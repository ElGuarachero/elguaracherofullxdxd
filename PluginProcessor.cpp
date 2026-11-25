#include "PluginProcessor.h"
#include "PluginEditor.h"

SaichanProcessor::SaichanProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
                        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    pDepth     = apvts.getRawParameterValue ("depth");
    pThreshold = apvts.getRawParameterValue ("threshold");
    pAttack    = apvts.getRawParameterValue ("attack");
    pHold      = apvts.getRawParameterValue ("hold");
    pRelease   = apvts.getRawParameterValue ("release");
    pMix       = apvts.getRawParameterValue ("mix");
    pUseSC     = apvts.getRawParameterValue ("usesc");
}

juce::AudioProcessorValueTreeState::ParameterLayout SaichanProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 80.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -30.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> (0.1f, 50.0f, 0.01f, 0.5f), 2.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "hold", 1 }, "Hold",
        juce::NormalisableRange<float> (0.0f, 500.0f, 0.1f), 120.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> (5.0f, 1000.0f, 0.1f, 0.5f), 200.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "usesc", 1 }, "External Sidechain", false));

    return { params.begin(), params.end() };
}

bool SaichanProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void SaichanProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    envelope = 0.0f;
    currentGain = 1.0f;
    holdRemaining = 0;
}

void SaichanProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    // ¿Usar el bus de sidechain como disparador? (solo si el host lo ha activado)
    auto* scBus = getBus (true, 1);
    const bool scActive = pUseSC->load() > 0.5f
                          && scBus != nullptr && scBus->isEnabled()
                          && buffer.getNumChannels() >= 4;

    const float depth     = pDepth->load() * 0.01f;
    const float thrLin    = juce::Decibels::decibelsToGain (pThreshold->load());
    const float mix       = pMix->load() * 0.01f;
    const int   holdSamps = juce::roundToInt (pHold->load() * 0.001 * sampleRate);

    const float attackCoef  = std::exp (-1.0f / (float) juce::jmax (1.0, sampleRate * pAttack->load()  * 0.001));
    const float releaseCoef = std::exp (-1.0f / (float) juce::jmax (1.0, sampleRate * pRelease->load() * 0.001));
    const float detectRelease = 0.995f; // ~ caída rápida del detector de pico

    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getWritePointer (1);

    const float* trigL = scActive ? buffer.getReadPointer (2) : outL;
    const float* trigR = scActive ? buffer.getReadPointer (3) : outR;

    for (int i = 0; i < numSamples; ++i)
    {
        // 1) Detector de pico del disparador
        const float peak = juce::jmax (std::abs (trigL[i]), std::abs (trigR[i]));
        envelope = juce::jmax (peak, envelope * detectRelease);

        // 2) Hold: mientras el kick esté por encima del umbral, se mantiene el ducking
        if (envelope > thrLin)
            holdRemaining = holdSamps;
        else if (holdRemaining > 0)
            --holdRemaining;

        // 3) Ganancia objetivo y suavizado (attack al bajar, release al subir)
        const float target = (holdRemaining > 0) ? (1.0f - depth) : 1.0f;
        const float coef   = (target < currentGain) ? attackCoef : releaseCoef;
        currentGain = coef * currentGain + (1.0f - coef) * target;

        // 4) Mezcla dry/wet
        const float wetL = outL[i] * currentGain;
        const float wetR = outR[i] * currentGain;
        outL[i] = outL[i] * (1.0f - mix) + wetL * mix;
        outR[i] = outR[i] * (1.0f - mix) + wetR * mix;
    }

    meter.store (1.0f - currentGain);
}

juce::AudioProcessorEditor* SaichanProcessor::createEditor()
{
    return new SaichanEditor (*this);
}

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

// Punto de entrada que JUCE usa para crear el plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SaichanProcessor();
}
