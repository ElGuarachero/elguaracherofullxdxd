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
    pDepth     = apvts.getRawParameterValue ("depth");
    pGlide     = apvts.getRawParameterValue ("glide");
    pNoiseHighPass = apvts.getRawParameterValue ("noisehp");
    pNoiseLowPass  = apvts.getRawParameterValue ("noiselp");
    pOutput    = apvts.getRawParameterValue ("output");
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

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 },
        "VOLUME",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "glide", 1 }, "GLIDE",
        juce::NormalisableRange<float> (0.0f, 2000.0f, 1.0f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noisehp", 1 }, "NOISE HP",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.01f, 0.3f), 20.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noiselp", 1 }, "NOISE LP",
        juce::NormalisableRange<float> (200.0f, 20000.0f, 0.01f, 0.3f), 20000.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "OUTPUT",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f));

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
    lpFreq = juce::jlimit (20.0, limit, lpFreq);

    if (hpFreq != lastHpFreq)
    {
        lastHpFreq = hpFreq;

        for (int ch = 0; ch < 2; ++ch)
            for (int s = 0; s < filterStages; ++s)
                hpFilters[ch][s].setHighPass (sampleRate, hpFreq, butterworthQ[s]);
    }

    if (lpFreq != lastLpFreq)
    {
        lastLpFreq = lpFreq;

        for (int ch = 0; ch < 2; ++ch)
            for (int s = 0; s < filterStages; ++s)
                lpFilters[ch][s].setLowPass (sampleRate, lpFreq, butterworthQ[s]);
    }
}

void SaichanProcessor::updateNoiseFilters (double hpFreq, double lpFreq)
{
    const double limit = sampleRate * 0.45;
    hpFreq = juce::jlimit (20.0, limit, hpFreq);
    lpFreq = juce::jlimit (20.0, limit, lpFreq);

    if (hpFreq != lastNoiseHpFreq)
    {
        lastNoiseHpFreq = hpFreq;
        for (int ch = 0; ch < 2; ++ch)
            noiseHpFilters[ch].setHighPass (sampleRate, hpFreq, 0.70710678);
    }

    if (lpFreq != lastNoiseLpFreq)
    {
        lastNoiseLpFreq = lpFreq;
        for (int ch = 0; ch < 2; ++ch)
            noiseLpFilters[ch].setLowPass (sampleRate, lpFreq, 0.70710678);
    }
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
    smoothedRateHz = 2.0f;
    smoothedSemitones = 0.0f;

    pitchDelayBuffer.setSize (2, juce::jmax (2048, (int) (sampleRate * 0.25)));
    pitchDelayBuffer.clear();

    for (int ch = 0; ch < 2; ++ch)
        for (int s = 0; s < filterStages; ++s)
        {
            hpFilters[ch][s].reset();
            lpFilters[ch][s].reset();
        }

    lastHpFreq = -1.0;
    lastLpFreq = -1.0;
    lastNoiseHpFreq = -1.0;
    lastNoiseLpFreq = -1.0;

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

    const float requestedRate = pRate->load();
    const bool syncMode = pRateMode->load() > 0.5f;
    const float requestedSemitones = pSemitones->load();
    const float glideMs = pGlide->load();
    const float outputDb = pOutput->load();
    const float noiseLevel = pNoise->load();
    const bool noiseFollowsRate = pNoiseSync->load() > 0.5f;

    // Cuánto se aplica el efecto de pumping (VOLUME): 0 = nada, 1 = completo
    const float depth = juce::jlimit (0.0f, 1.0f, pDepth->load());
    const float minimumGain = 1.0f - depth * (1.0f - 0.22f);

    // Curva cuadrática: el volumen sube de forma suave con la perilla
    const float noiseGain = noiseLevel * noiseLevel * 0.25f;

    // BPM desde el host
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto hostBpm = position->getBpm())
                bpm = juce::jlimit (30.0, 300.0, *hostBpm);
        }
    }

    updateFilters (pHighPass->load(), pLowPass->load());
    updateNoiseFilters (pNoiseHighPass->load(), pNoiseLowPass->load());

    // Convertir RATE a frecuencia objetivo; el glide suaviza también cambios de SYNC.
    const bool requestedPumpEnabled = requestedRate > 0.001f;
    double targetCycleSeconds = 0.5;
    if (requestedPumpEnabled)
    {
        if (syncMode)
        {
            const int division = juce::jlimit (1, numSyncDivisions,
                                                juce::roundToInt (requestedRate));
            targetCycleSeconds = syncBeats[division - 1] * 60.0 / bpm;
        }
        else
        {
            const double frequency = juce::jlimit (0.01, 32.0, (double) requestedRate);
            targetCycleSeconds = 1.0 / frequency;
        }
    }
    const float targetRateHz = requestedPumpEnabled
        ? (float) (1.0 / targetCycleSeconds) : 0.0f;

    const float glideSeconds = glideMs * 0.001f;
    const float glideCoefficient = glideSeconds <= 0.0001f ? 1.0f
        : 1.0f - std::exp (-1.0f / (float) (glideSeconds * sampleRate));

    const float targetForSmoothing = targetRateHz;
    const float semitoneCoefficient = glideCoefficient;

    // El pitch y el rate siguen el tiempo de glide elegido.
    const float outputGain = juce::Decibels::decibelsToGain (outputDb);

    const int delaySize = pitchDelayBuffer.getNumSamples();
    const float windowSamples = (float) (sampleRate * 0.08);
    const float minimumDelay = (float) (sampleRate * 0.005);


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
        smoothedRateHz += (targetForSmoothing - smoothedRateHz) * glideCoefficient;
        smoothedSemitones += (requestedSemitones - smoothedSemitones) * semitoneCoefficient;

        const bool pumpEnabled = smoothedRateHz > 0.001f;
        const double phaseIncrement = pumpEnabled ? (double) smoothedRateHz / sampleRate : 0.0;
        const float pitchRatio = std::pow (2.0f, smoothedSemitones / 12.0f);
        const bool pitchEnabled = std::abs (smoothedSemitones) > 0.01f;
        const float pitchPhaseIncrement = pitchEnabled
            ? std::abs (pitchRatio - 1.0f) / windowSamples : 0.0f;

        // 1. Pumping rítmico
        float pumpShape = 0.0f;

        if (pumpEnabled)
        {
            const float phase = (float) pumpPhase;

            const float curve = phase * phase;
            currentGain = minimumGain + (1.0f - minimumGain) * curve;
            pumpShape = 1.0f - curve;

            pumpPhase += phaseIncrement;

            if (pumpPhase >= 1.0)
                pumpPhase -= 1.0;
        }
        else
        {
            currentGain = 1.0f;
        }

        // 2. Pitch
        float inputL = left[i];
        float inputR = right[i];

        if (pitchEnabled)
        {
            delayL[pitchWritePosition] = inputL;
            delayR[pitchWritePosition] = inputR;

            const float phaseA = (float) pitchPhase;
            const float phaseB = std::fmod (phaseA + 0.5f, 1.0f);

            const float direction = (smoothedSemitones > 0.0f) ? -1.0f : 1.0f;

            const float delayA = minimumDelay
                + (direction > 0.0f ? phaseA : 1.0f - phaseA) * windowSamples;

            const float delayB = minimumDelay
                + (direction > 0.0f ? phaseB : 1.0f - phaseB) * windowSamples;

            const float readA = (float) pitchWritePosition - delayA;
            const float readB = (float) pitchWritePosition - delayB;

            const float fadeA = 0.5f - 0.5f * std::cos (
                phaseA * juce::MathConstants<float>::twoPi);

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
        else
        {
            delayL[pitchWritePosition] = inputL;
            delayR[pitchWritePosition] = inputR;
        }

        // 3. Aplicar pumping
        float outL = inputL * currentGain;
        float outR = inputR * currentGain;

        // 4. High pass y low pass, 8.º orden cada uno
        for (int s = 0; s < filterStages; ++s)
        {
            outL = hpFilters[0][s].process (outL);
            outR = hpFilters[1][s].process (outR);
        }

        for (int s = 0; s < filterStages; ++s)
        {
            outL = lpFilters[0][s].process (outL);
            outR = lpFilters[1][s].process (outR);
        }

        // 5. Noise
        const float noiseEnvelope = noiseFollowsRate
            ? (pumpEnabled ? pumpShape : 0.0f)
            : 1.0f;

        const float amp = noiseGain * noiseEnvelope;
        float noiseL = (noiseRandom.nextFloat() * 2.0f - 1.0f) * amp;
        float noiseR = (noiseRandom.nextFloat() * 2.0f - 1.0f) * amp;

        noiseL = noiseHpFilters[0].process (noiseL);
        noiseR = noiseHpFilters[1].process (noiseR);
        noiseL = noiseLpFilters[0].process (noiseL);
        noiseR = noiseLpFilters[1].process (noiseR);

        outL = (outL + noiseL) * outputGain;
        outR = (outR + noiseR) * outputGain;

        left[i] = outL;
        right[i] = outR;

        // 6. Avanzar el buffer de delay
        pitchWritePosition = (pitchWritePosition + 1) % delaySize;
    }

    meter.store (1.0f - currentGain);
    visualPhase.store ((float) pumpPhase);
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
