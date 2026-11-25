
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Constructor
//==============================================================================
SaichanProcessor::SaichanProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    pRate      = apvts.getRawParameterValue ("rate");
    pRateMode  = apvts.getRawParameterValue ("ratemode");
    pSemitones = apvts.getRawParameterValue ("semitones");
}

//==============================================================================
// Parameters
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
SaichanProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "rate", 1 },
        "RATE",
        juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f),
        2.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "ratemode", 1 },
        "RATE MODE",
        true));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "semitones", 1 },
        "SEMITONES",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f),
        0.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
// Bus layout
//==============================================================================
bool SaichanProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
// Prepare
//==============================================================================
void SaichanProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    pumpPhase = 0.0;
    currentGain = 1.0f;
    pitchPhase = 0.0f;
    pitchWritePosition = 0;

    pitchDelayBuffer.setSize (2, juce::jmax (2048, (int) (sampleRate * 0.25)));
    pitchDelayBuffer.clear();

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto hostBpm = position->getBpm())
                bpm = *hostBpm;
        }
    }
}

//==============================================================================
// Audio processing
//==============================================================================
void SaichanProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    // Read parameters
    const float rate = pRate->load();
    const bool syncMode = pRateMode->load() > 0.5f;
    const float semitones = pSemitones->load();

    // Get BPM from FL Studio
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto hostBpm = position->getBpm())
                bpm = juce::jlimit (30.0, 300.0, *hostBpm);
        }
    }

    // RATE: musical divisions or free Hz
    double cycleSeconds = 0.5;

    if (syncMode)
    {
        static constexpr double beatMultipliers[] =
        {
            4.0, 2.0, 1.0, 0.5, 0.25
        };

        const int division = juce::jlimit (0, 4, juce::roundToInt (rate));
        cycleSeconds = beatMultipliers[division] * 60.0 / bpm;
    }
    else
    {
        const double frequency = 0.25 + (rate / 4.0) * 7.75;
        cycleSeconds = 1.0 / frequency;
    }

    const double phaseIncrement = 1.0 / (cycleSeconds * sampleRate);

    // Fixed classic pumping curve
    constexpr float minimumGain = 0.22f;

    // Pitch shifting preparation
    const float pitchRatio = std::pow (2.0f, semitones / 12.0f);
    const bool pitchEnabled = std::abs (semitones) > 0.01f;

    const int delaySize = pitchDelayBuffer.getNumSamples();
    const float windowSamples = (float) (sampleRate * 0.08);
    const float minimumDelay = (float) (sampleRate * 0.005);

    const float pitchPhaseIncrement =
        pitchEnabled
        ? std::abs (pitchRatio - 1.0f) / windowSamples
        : 0.0f;

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    float* delayL = pitchDelayBuffer.getWritePointer (0);
    float* delayR = pitchDelayBuffer.getWritePointer (1);

    auto readInterpolated = [] (const float* data, int size, float position)
    {
        while (position < 0.0f)
            position += (float) size;

        while (position >= (float) size)
            position -= (float) size;

        const int index1 = (int) position;
        const int index2 = (index1 + 1) % size;
        const float fraction = position - (float) index1;

        return data[index1] + fraction * (data[index2] - data[index1]);
    };

    for (int i = 0; i < numSamples; ++i)
    {
        // 1. Automatic rhythmic pumping
        const float phase = (float) pumpPhase;

        // Fast dip followed by a smooth recovery
        const float curve = phase * phase;
        const float targetGain = minimumGain + (1.0f - minimumGain) * curve;

        currentGain = targetGain;

        pumpPhase += phaseIncrement;

        if (pumpPhase >= 1.0)
            pumpPhase -= 1.0;

        // 2. Pitch processing
        float inputL = left[i];
        float inputR = right[i];

        if (pitchEnabled)
        {
            delayL[pitchWritePosition] = inputL;
            delayR[pitchWritePosition] = inputR;

            const float phaseA = (float) pitchPhase;
            const float phaseB = std::fmod (phaseA + 0.5f, 1.0f);

            const float direction = (semitones > 0.0f) ? -1.0f : 1.0f;

            const float delayA = minimumDelay
                + (direction > 0.0f ? phaseA : 1.0f - phaseA) * windowSamples;

            const float delayB = minimumDelay
                + (direction > 0.0f ? phaseB : 1.0f - phaseB) * windowSamples;

            const float readA = (float) pitchWritePosition - delayA;
            const float readB = (float) pitchWritePosition - delayB;

            const float fadeA = 0.5f - 0.5f * std::cos (phaseA * juce::MathConstants<float>::twoPi);
            const float fadeB = 1.0f - fadeA;

            const float shiftedL =
                readInterpolated (delayL, delaySize, readA) * fadeA
                + readInterpolated (delayL, delaySize, readB) * fadeB;

            const float shiftedR =
                readInterpolated (delayR, delaySize, readA) * fadeA
                + readInterpolated (delayR, delaySize, readB) * fadeB;

            inputL = shiftedL;
            inputR = shiftedR;

            pitchPhase += pitchPhaseIncrement;

            if (pitchPhase >= 1.0f)
                pitchPhase -= 1.0f;
        }

        // 3. Apply pumping
        left[i] = inputL * currentGain;
        right[i] = inputR * currentGain;

        // 4. Advance delay buffer
        pitchWritePosition = (pitchWritePosition + 1) % delaySize;
    }

    meter.store (1.0f - currentGain);
}

//==============================================================================
// Editor
//==============================================================================
juce::AudioProcessorEditor* SaichanProcessor::createEditor()
{
    return new SaichanEditor (*this);
}

//==============================================================================
// Save and restore
//==============================================================================
void SaichanProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SaichanProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// Plugin entry point
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SaichanProcessor();
}
