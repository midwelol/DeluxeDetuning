# DeluxeDetune

A JUCE-based audio plugin implementing a delay-line pitch-shifting algorithm to recreate the detuned guitar effect of the DigiTech Luxe pedal.

## Overview

DeluxeDetuning is a C++ audio plugin built with JUCE that recreates the continuously detuned, doubled-guitar character of the DigiTech Luxe pedal.

This project was created as a way for me to learn audio plugin development in JUCE, real-time audio processing, circular delay buffers, fractional-delay interpolation, pitch shifting, parameter smoothing, and crossfading between multiple delay read heads through hands-on development.

The plugin processes incoming audio in real time and combines the original dry signal with a pitch-shifted wet signal. The pitch-shifting alogrithm uses a circular delay buffer with two moving read heads. The read heads are offset in phase and crossfaded to reduce discontinuities as they move through the delay buffer.

The current implementation uses two parameters:
- Detune: Ranging from -50 to +50 cents
- Mix: ranging from 0 to 1

Both parameters are smoothed to avoid abrupt parameter changes and zipper noise.

## Feautres

- Real-time stereo audio processing
- JUCE-based VST3/audio-plugin architecture
- Adjustable detune from -50 to +50 cents
- Adjustable dry/wet mix
- Circular delay buffer
- Two moving delay read heads
- Fractional-delay interpolation
- Crossfading between read heads
- Parameter smoothing
- Stereo input/output support
- GUI controls using JUCE sliders
- AudioProcessorValueTreeState parameter management
- Visual Studio project/build configuration

## Controls

### Detune

The Detune parameter controls the pitch offset of the wet signal.

Range:

- -50 cents
- 0 cents
- +50 cents

The pitch ratio is calculated from cents using:

pitchRatio = 2^(cents / 1200)

This converts the musical pitch interval in cents into the resampling ratio needed by the delay-based pitch-shifting algorithm.
### Mix

The Mix parameter controls the balance between the original dry signal and the processed wet signal.

Range:

- 0.0 = fully dry
- 1.0 = fully wet

The output is calculated as:

output = (1 - mix) * dry + mix * wet

The current default is 0.5.

## DSP Concepts

The plugin represents the desired pitch shift in cents.

A semitone is 100 cents, and an octave is 1200 cents.

The frequency ratio corresponding to a pitch shift in cents is:

pitchRatio = 2^(cents / 1200)

The processor uses this relationship to convert the Detune parameter into a pitch ratio.

## Delay-Based Pitch Shifting

The core pitch-shifting algorithm uses a circular delay buffer.

Instead of simply delaying the incoming signal by a fixed amount, the processor changes the delay experienced by the read head over time.

A moving read position effectively changes the playback rate of the delayed signal:

- Moving the read position faster relative to the incoming samples produces an upward pitch shift.
- Moving it slower produces a downward pitch shift.

The current implementation stores incoming samples in delayBuffer and reads from two moving positions within that buffer.

## Circular Delay Buffer

The processor uses a circular buffer to store recent samples.

The buffer has a fixed size, and the write position advances through it:

writeIndex = (writeIndex + 1) % bufferSize

When the write position reaches the end of the buffer, it wraps back to the beginning.

This allows the plugin to continuously reuse a fixed amount of memory instead of allocating new audio storage during real-time processing.

The delay buffer is initialized in prepareToPlay() based on the current sample rate, with a maximum delay of 0.1 seconds.

## Dual Read Heads

A single moving read head can create discontinuities when its delay trajectory wraps around.

To make the transition smoother, the processor uses two read heads.

The read heads are offset by half of the modulation window:

phaseB = phase + windowSamples * 0.5

The two heads therefore operate at different points in their delay trajectories.

When one read head approaches a problematic transition, the other is positioned to provide the replacement signal.

## Crossfading Between Read Heads

The two read heads are combined using complementary gain values.

The processor calculates a Hann-style gain for one read head:

gainA = 0.5 - 0.5 * cos(2π * phase / window)

and uses:

gainB = 1 - gainA

The two interpolated signals are then combined:

blendedWet = wetA * gainA + wetB * gainB

This creates a smooth transition between the two read heads instead of abruptly switching from one to the other.

## Fractional-Delay Interpolation

The desired read positions are floating-point values rather than integer sample indices.

For example:

readPos = 125.37

The processor separates the integer index from the fractional component:

idx = 125
frac = 0.37

It then linearly interpolates between adjacent samples:

interpolated = sample1 + frac * (sample2 - sample1)

This allows the read head to move continuously between samples instead of being restricted to integer-sample delay positions.

The current implementation performs this interpolation independently for both read heads. 

## Parameter Smoothing

The processor uses JUCE SmoothedValue objects for both the Detune and Mix parameters.

The detune parameter is smoothed before calculating the pitch ratio, while the mix parameter is smoothed before calculating the dry/wet output.

This prevents sudden parameter changes from creating audible zipper noise or abrupt changes in the pitch-shifting trajectory.

The smoothers are initialized with a 10 ms ramp time.

### Why this matters

Parameter smoothing is a real-world audio-plugin concern.

A GUI control can change by a large amount between audio callbacks.

If the DSP immediately uses that new value, the resulting discontinuity can be audible.

Smoothing turns a sudden parameter jump into a short continuous transition.

## Real-Time Processing

The plugin performs its DSP inside JUCE's processBlock() callback.

For each incoming sample, the processor:

1. Retrieves the current smoothed detune value.
2. Converts cents to a pitch ratio.
3. Advances the shared phase accumulator.
4. Calculates the two read-head positions.
5. Interpolates the samples at both read positions.
6. Calculates the crossfade gains.
7. Blends the two wet signals.
8. Writes the dry input into the circular delay buffer.
9. Mixes the dry and wet signals.
10. Advances the circular-buffer write index.

The current implementation performs sample processing inside the buffer loop and processes channels using the same read-head state.

## Design Goal

The goal of the project was to recreate the characteristic detuned guitar effect associated with the DigiTech Luxe pedal while learning how real-time pitch-shifting algorithms can be constructed from basic DSP building blocks.

Rather than treating the effect as a black box, the project focuses on understanding the underlying concepts:

- Delay lines
- Variable delay
- Read-head movement
- Fractional delays
- Interpolation
- Crossfading
- Pitch ratios
- Parameter smoothing
- Real-time audio buffers
