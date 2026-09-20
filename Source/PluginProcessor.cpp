/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DeluxeDetuneAudioProcessor::DeluxeDetuneAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    apvts(*this, nullptr, "Parameters", createParameterLayout())

#endif
{
}

DeluxeDetuneAudioProcessor::~DeluxeDetuneAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout DeluxeDetuneAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "detune",
        "Detune",
        -50.0f,
        50.0f,
        0.0f
    ));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix",
        "Mix",
        0.0f,
        1.0f,
        0.5f
    ));
    return layout;
}

juce::AudioProcessorValueTreeState& DeluxeDetuneAudioProcessor::getAPVTS()
{
    return apvts;
}

const juce::String DeluxeDetuneAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DeluxeDetuneAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool DeluxeDetuneAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool DeluxeDetuneAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double DeluxeDetuneAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DeluxeDetuneAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int DeluxeDetuneAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DeluxeDetuneAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String DeluxeDetuneAudioProcessor::getProgramName(int index)
{
    return {};
}

void DeluxeDetuneAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void DeluxeDetuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    const double maxDelaySeconds = 0.1;
    const int maxDelaySamples = static_cast<int>(sampleRate * maxDelaySeconds);

    delayBuffer.setSize(getTotalNumInputChannels(), maxDelaySamples);
    delayBuffer.clear();

    writeIndex = 0;
    phase = 0.0f;

    // Initialize the base delay and window sizes used by the phase-driven
    // dual-read-head detune algorithm. baseDelaySamples is the center delay
    // (in samples) around which both read heads sweep. windowSamples defines
    // the sweep span so that the two heads are half a window apart.
    baseDelaySamples = static_cast<float>(sampleRate * 0.015);
    windowSamples = static_cast<float>(sampleRate * 0.01);

    detuneSmoother.setCurrentAndTargetValue(0.0f);
    detuneSmoother.reset(sampleRate, 0.01);

    mixSmoother.setCurrentAndTargetValue(0.5f);
    mixSmoother.reset(sampleRate, 0.01);
}

void DeluxeDetuneAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DeluxeDetuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void DeluxeDetuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int bufferSize = delayBuffer.getNumSamples();

    float cents = apvts.getRawParameterValue("detune")->load();
    float mix = apvts.getRawParameterValue("mix")->load();
    detuneSmoother.setTargetValue(cents);
    mixSmoother.setTargetValue(mix);

    // Note: detuneSmoother and mixSmoother are used to smooth user parameter
    // changes. detuneSmoother smooths the detune (cents) so pitchRatio changes
    // continuously; mixSmoother smooths the wet/dry mix to avoid zipper noise.

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float smoothedCents = detuneSmoother.getNextValue();
        float pitchRatio = std::pow(2.0f, (smoothedCents / 1200.0f));
		float smoothedMix = mixSmoother.getNextValue();

        // phaseRate: how quickly the shared phase accumulator advances this sample.
        // It is derived from the resampling pitchRatio so that the read-head
        // sweep speed matches the desired pitch shift.
        float phaseRate = pitchRatio - 1.0f;

        // Advance the shared phase accumulator and wrap it to [0, windowSamples).
        // The shared 'phase' controls both read heads so that they are 180deg
        // out of phase (half a window apart). This guarantees that when a head
        // wraps the other's gain is near zero, avoiding discontinuities.
        phase = std::fmod(phase + phaseRate, windowSamples);
        if (phase < 0.0f)
            phase += windowSamples;

        // phaseB is the phase for the second head; offset by half the window
        // so the two heads move oppositely and crossfade smoothly.
        float phaseB = std::fmod(phase + windowSamples * 0.5f, windowSamples);

        // delayA and delayB are the instantaneous delay lengths (in samples)
        // for the two read heads. They are computed from baseDelaySamples plus
        // a phase-dependent offset so the heads sweep around the base delay.
        float delayA = baseDelaySamples + (windowSamples * 0.5f - phase);
        float delayB = baseDelaySamples + (windowSamples * 0.5f - phaseB);

        // Compute read positions in the circular delay buffer. The read index is
        // writeIndex - delay (in samples); wrap using fmod and correct negative
        // values so indices are always in buffer range.
        float readPosA = std::fmod(writeIndex - delayA, static_cast<float>(bufferSize));
        if (readPosA < 0.0f) readPosA += bufferSize;

        float readPosB = std::fmod(writeIndex - delayB, static_cast<float>(bufferSize));
        if (readPosB < 0.0f) readPosB += bufferSize;

        // gainA/gainB form a crossfade window between the two read heads. The
        // formula below implements a Hann window (raised cosine) for each
        // head: gainA = 0.5 * (1 - cos(2*pi*phase/window)). Using a Hann window
        // produces smooth amplitude ramps with zero derivatives at the edges,
        // which avoids clicks when heads cross or wrap.
        float gainA = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * phase / windowSamples);
        float gainB = 1.0f - gainA;

        if (sample == 0)
        {
            DBG("phase=" << phase << " delayA=" << delayA << " delayB=" << delayB
                << " gainA=" << gainA << " ratio=" << pitchRatio);
        }
        

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* delayChannel = delayBuffer.getWritePointer(channel);

            // ..do something to the data...

            int idxA1 = static_cast<int>(readPosA);
            float fracA = readPosA - idxA1;
            int idxA2 = (idxA1 + 1) % bufferSize;

            int idxB1 = static_cast<int>(readPosB);
            float fracB = readPosB - idxB1;
            int idxB2 = (idxB1 + 1) % bufferSize;


            float dry = channelData[sample];
            float wetA = delayChannel[idxA1] + fracA * (delayChannel[idxA2] - delayChannel[idxA1]);
            float wetB = delayChannel[idxB1] + fracB * (delayChannel[idxB2] - delayChannel[idxB1]);
            float blendedWet = wetA * gainA + wetB * gainB;


            delayChannel[writeIndex] = dry;
            channelData[sample] = (1.0f - smoothedMix) * dry + smoothedMix * blendedWet;
        }
        writeIndex = (writeIndex + 1) % bufferSize;
    }
}

//==============================================================================
bool DeluxeDetuneAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DeluxeDetuneAudioProcessor::createEditor()
{
    return new DeluxeDetuneAudioProcessorEditor(*this);
}

//==============================================================================
void DeluxeDetuneAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DeluxeDetuneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DeluxeDetuneAudioProcessor();
}

