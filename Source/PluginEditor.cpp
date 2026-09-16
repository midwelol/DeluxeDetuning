/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DeluxeDetuneAudioProcessorEditor::DeluxeDetuneAudioProcessorEditor (DeluxeDetuneAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (500, 675);


    // Detune
    detune.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    detune.setRange(-50.0f, 50.0f, 1.0f);
    detune.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    detune.setPopupDisplayEnabled(true, false, this);
    detune.setTextValueSuffix(" Detune");
    detune.setValue(0.0f);

	detuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "detune", detune);

    // Label
	detuneLabel.setText("Detune", juce::dontSendNotification);
    detuneLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(&detune);
    addAndMakeVisible(&detuneLabel);

    // Mix / Level
    mix.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mix.setRange(0.0f, 1.0f, 0.01f);
    mix.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    mix.setPopupDisplayEnabled(true, false, this);
    mix.setTextValueSuffix(" Mix");
    mix.setValue(0.5f);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "mix", mix);

    // Label
	mixLabel.setText("Mix", juce::dontSendNotification);
	mixLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(&mix);
    addAndMakeVisible(&mixLabel);


}

DeluxeDetuneAudioProcessorEditor::~DeluxeDetuneAudioProcessorEditor()
{
}

//==============================================================================
void DeluxeDetuneAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::black);
}

void DeluxeDetuneAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    detune.setBounds(40, 50, 150, 150);
    mix.setBounds(310, 50, 150, 150);
    detuneLabel.setBounds(40, 200, 150, 30);
    mixLabel.setBounds(310, 200, 150, 30);
}

