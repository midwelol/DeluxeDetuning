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
    setSize (200, 200);
    // slider object parameters
    detune.setSliderStyle(juce::Slider::LinearBarVertical);
    detune.setRange(-50.0f, 50.0f, 1.0f);
    detune.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    detune.setPopupDisplayEnabled(true, false, this);
    detune.setTextValueSuffix(" Detune");
    detune.setValue(0.0f);

    // add listener to slider
    detune.addListener(this);
    //add to editor
    addAndMakeVisible(&detune);
}

DeluxeDetuneAudioProcessorEditor::~DeluxeDetuneAudioProcessorEditor()
{
}

//==============================================================================
void DeluxeDetuneAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Detune", getLocalBounds(), juce::Justification::centred, 1);
}

void DeluxeDetuneAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    detune.setBounds(40, 30, 20, getHeight() - 60);
}
