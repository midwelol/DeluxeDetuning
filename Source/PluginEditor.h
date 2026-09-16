/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class DeluxeDetuneAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          private juce::Slider::Listener
{
public:
    DeluxeDetuneAudioProcessorEditor (DeluxeDetuneAudioProcessor&);
    ~DeluxeDetuneAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void sliderValueChanged(juce::Slider* slider) override;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DeluxeDetuneAudioProcessor& audioProcessor;
    juce::Slider detune;
    //juce::Slider mix;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeluxeDetuneAudioProcessorEditor)
};
