/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class DeluxeDetuneAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DeluxeDetuneAudioProcessor();
    ~DeluxeDetuneAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState& getAPVTS();

    

private:
    //==============================================================================
    juce::AudioBuffer<float> delayBuffer;
    juce::AudioProcessorValueTreeState apvts;
    int writeIndex = 0;

    // "phase" is a shared phase accumulator that drives the two read-head
    // positions (delayA/delayB) used for the detune/crossfade algorithm.
    // The accumulator advances proportionally to the resampling ratio so that
    // when a read-head wraps the corresponding crossfade gain is near zero.
    float phase = 0.0f;

    // baseDelaySamples is the center delay (in samples) for both taps. windowSamples
    // is the modulation window size (in samples) used to sweep each read-head
    // around the base delay; together they determine delayA/delayB below.
    float baseDelaySamples = 0.0f;
    float windowSamples = 0.0f;


    // Parameter smoothing objects: detuneSmoother smooths the detune (in cents)
    // so pitchRatio updates are continuous. mixSmoother smooths the wet/dry mix.
    juce::SmoothedValue<float> detuneSmoother;
    juce::SmoothedValue<float> mixSmoother;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeluxeDetuneAudioProcessor)
};
