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
    int delaySamples = static_cast<int>(sampleRate * 0.01);

    delayBuffer.setSize(getTotalNumInputChannels(), maxDelaySamples);
    delayBuffer.clear();

    writeIndex = 0;
    readPosition = static_cast<float>(delaySamples);

 
    readPosition2 = std::fmod(readPosition - (delaySamples / 2.0f), static_cast<float>(maxDelaySamples));
    if (readPosition2 < 0.0f)
        readPosition2 += maxDelaySamples;

    activePosition = 0;
    mixHead = 0.0f;
    crossfading = false;

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

    int delaySamples = static_cast<int>(getSampleRate() * 0.01);

    int crossfadeSamples = static_cast<int>(getSampleRate() * 0.01);
    float crossfadeIncrement = 1.0f / crossfadeSamples;

    float windowSamples = 2.0f * delaySamples;

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
       
        float distance = std::fmod(writeIndex - readPosition, windowSamples);
        if (distance < 0.0f)
            distance += windowSamples;

        float distance2 = std::fmod(writeIndex - readPosition2, windowSamples);
        if (distance2 < 0.0f)
            distance2 += windowSamples;

        if (activePosition == 0 && distance < crossfadeSamples && crossfading == false)
        {
            crossfading = true;
        }
        else if (activePosition == 1 && distance2 < crossfadeSamples && crossfading == false)
        {
            crossfading = true;
        }

        if (activePosition == 0 && crossfading == true && mixHead < 1.0f)
        {
            mixHead += crossfadeIncrement;
            mixHead = std::clamp(mixHead, 0.0f, 1.0f);

            if (mixHead >= 1.0f)
            {
                activePosition = 1;
                crossfading = false;

                readPosition = writeIndex - delaySamples;
                if (readPosition < 0.0f)
                {
                    readPosition += bufferSize;
                }
            }
        }
        else if (activePosition == 1 && crossfading == true && mixHead > 0.0f)
        {
            mixHead -= crossfadeIncrement;
            mixHead = std::clamp(mixHead, 0.0f, 1.0f);

            if (mixHead <= 0.0f)
            {
                activePosition = 0;
                crossfading = false;

                readPosition2 = writeIndex - delaySamples;
                if (readPosition2 < 0.0f)
                {
                    readPosition2 += bufferSize;
                }
            }
        }

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* delayChannel = delayBuffer.getWritePointer(channel);

            // ..do something to the data...
            int index1 = static_cast<int>(readPosition);
            float fractional = readPosition - index1;
            int index2 = (index1 + 1) % bufferSize;
            int index3 = static_cast<int>(readPosition2);
            float fractional2 = readPosition2 - index3;
            int index4 = (index3 + 1) % bufferSize;

            float dry = channelData[sample];
            float wet = delayChannel[index1] + fractional * (delayChannel[index2] - delayChannel[index1]);
            float wet2 = delayChannel[index3] + fractional2 * (delayChannel[index4] - delayChannel[index3]);
            float blendedWet = wet * (1.0f - mixHead) + wet2 * mixHead;

            delayChannel[writeIndex] = channelData[sample];

            channelData[sample] = (1.0f - smoothedMix) * dry + smoothedMix * blendedWet;
        }
        writeIndex = (writeIndex + 1) % delayBuffer.getNumSamples();
        readPosition = std::fmod(readPosition + pitchRatio, bufferSize);
        readPosition2 = std::fmod(readPosition2 + pitchRatio, bufferSize);
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

