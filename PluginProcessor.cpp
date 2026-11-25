#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Sync table (24 divisions, in quarter-note beats)
//==============================================================================
const char* const SaichanProcessor::syncNames[SaichanProcessor::numSyncDivisions] =
{
    "1/1",  "1/2",  "1/4",  "1/8",  "1/16",  "1/32",  "1/64",  "1/128",
    "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T", "1/64T", "1/128T",
    "1/1D", "1/2D", "1/4D", "1/8D", "1/16D", "1/32D", "1/64D", "1/128D"
};

const double SaichanProcessor::syncBeats[SaichanProcessor::numSyncDivisions] =
{
    4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125,
    8.0 / 3.0, 4.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0,
    1.0 / 6.0, 1.0 / 12.0, 1.0 / 24.0, 1.0 / 48.0,
    6.0, 3.0, 1.5, 0.75, 0.375, 0.1875, 0.09375, 0.046875
};

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
    pHighPass  = apvts.getRawParameterValue ("highpass");
    pLowPass   = apvts.getRawParameterValue ("lowpass");
    pNoise     = apvts.getRawParameterValue ("noise");
    pNoiseSync = apvts.getRawParameterValue ("noisesync");
}

//==============================================================================
// Parameters
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
SaichanProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rate", 1 },
        "RATE",
        juce::NormalisableRange<float> (0.0f, 32.0f, 0.01f),
        8.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "ratemode", 1 },
        "RATE MODE",
        true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "semitones", 1 },
        "SEMITONES",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "highpass", 1 },
        "HIGH PASS",
        juce::NormalisableRange<float> (20.0f, 2000.0f, 0.01f, 0.3f),
        20.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lowpass", 1 },
        "LOW PASS",
        juce::NormalisableRange<float> (200.0f, 20000.0f, 0.01f, 0.3f),
        20000.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise", 1 },
        "NOISE",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "noisesync", 1 },
        "NOISE SYNC",
        true));

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
// Filters
//==============================================================================
void SaichanProcessor::updateFilters (double hpFreq, double lpFreq)
{
    const double limit = sampleRate * 0.45;
    hpFreq = juce::jlimit (20.0, limit, hpFreq);
