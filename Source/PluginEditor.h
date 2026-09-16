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
class DeluxeDetuneAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    DeluxeDetuneAudioProcessorEditor (DeluxeDetuneAudioProcessor&);
    ~DeluxeDetuneAudioProcessorEditor() override;



    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
	DeluxeDetuneAudioProcessor& audioProcessor;
    juce::Slider detune;
    juce::Slider mix;

    juce::Label detuneLabel;
    juce::Label mixLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> detuneAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeluxeDetuneAudioProcessorEditor)
};
