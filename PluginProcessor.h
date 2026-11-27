#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cmath>

//==============================================================================
// Filtro biquad simple (un tramo de filtro de 2.º orden)
//==============================================================================
struct SaichanBiquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void setHighPass (double sampleRate, double freq, double q)
    {
        const double w0 = juce::MathConstants<double>::twoPi * freq / sampleRate;
        const double cw = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * q);
        const double a0 = 1.0 + alpha;

        b0 = (float) ((1.0 + cw) * 0.5 / a0);
        b1 = (float) (-(1.0 + cw) / a0);
        b2 = (float) ((1.0 + cw) * 0.5 / a0);
        a1 = (float) (-2.0 * cw / a0);
        a2 = (float) ((1.0 - alpha) / a0);
    }

    void setLowPass (double sampleRate, double freq, double q)
    {
        const double w0 = juce::MathConstants<double>::twoPi * freq / sampleRate;
        const double cw = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * q);
        const double a0 = 1.0 + alpha;

        b0 = (float) ((1.0 - cw) * 0.5 / a0);
        b1 = (float) ((1.0 - cw) / a0);
        b2 = (float) ((1.0 - cw) * 0.5 / a0);
        a1 = (float) (-2.0 * cw / a0);
        a2 = (float) ((1.0 - alpha) / a0);
    }

    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

//==============================================================================
// Processor
//==============================================================================
class SaichanProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numSyncDivisions = 24;
    static const char* const syncNames[numSyncDivisions];
    static const double syncBeats[numSyncDivisions];

    SaichanProcessor();
    ~SaichanProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

      juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override
    {
        return "EL MAESTRO SAICHAN";
    }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getMeterLevel() const noexcept
    {
        return meter.load();
    }

    float getVisualPhase() const noexcept
    {
        return visualPhase.load();
    }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateFilters (double hpFreq, double lpFreq);
    void updateNoiseFilters (double hpFreq, double lpFreq);

    // 8.º orden = 4 biquads por filtro (valores Q de Butterworth)
    static constexpr int filterStages = 4;
    static constexpr float butterworthQ[filterStages] = { 0.5098f, 0.6013f, 0.8999f, 2.5629f };

    std::atomic<float>* pRate = nullptr;
    std::atomic<float>* pRateMode = nullptr;
    std::atomic<float>* pSemitones = nullptr;
    std::atomic<float>* pHighPass = nullptr;
    std::atomic<float>* pLowPass = nullptr;
    std::atomic<float>* pNoise = nullptr;
    std::atomic<float>* pNoiseSync = nullptr;
    std::atomic<float>* pDepth = nullptr;
    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pNoiseHighPass = nullptr;
    std::atomic<float>* pNoiseLowPass = nullptr;
    std::atomic<float>* pOutput = nullptr;

    std::atomic<float> meter { 0.0f };
    std::atomic<float> visualPhase { 0.0f };

    double sampleRate = 44100.0;
    double bpm = 120.0;

    double pumpPhase = 0.0;
    float currentGain = 1.0f;

    juce::AudioBuffer<float> pitchDelayBuffer;
    int pitchWritePosition = 0;
    float pitchPhase = 0.0f;

    SaichanBiquad hpFilters[2][filterStages];   // [canal][etapa]
    SaichanBiquad lpFilters[2][filterStages];
    SaichanBiquad noiseHpFilters[2];
    SaichanBiquad noiseLpFilters[2];
    double lastHpFreq = -1.0;
    double lastLpFreq = -1.0;
    double lastNoiseHpFreq = -1.0;
    double lastNoiseLpFreq = -1.0;

    // Parámetros suavizados para evitar saltos audibles
    float smoothedRateHz = 2.0f;
    float smoothedSemitones = 0.0f;

    // Detector de entrada para que el NOISE solo aparezca cuando hay señal.
    float noiseInputEnvelope = 0.0f;

    juce::Random noiseRandom;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaichanProcessor)
};
