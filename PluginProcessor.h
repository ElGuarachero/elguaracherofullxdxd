#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class SaichanProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numSyncDivisions = 24;
    static const char* const syncNames[numSyncDivisions];
    static const double syncBeats[numSyncDivisions];   // duración en negras

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

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateFilters (double hpFreq, double lpFreq);

    // 8th order = 4 biquads per filter (Butterworth Q values)
    static constexpr int filterStages = 4;
    static constexpr float butterworthQ[filterStages] = { 0.5098f, 0.6013f, 0.8999f, 2.5629f };

    std::atomic<float>* pRate = nullptr;
    std::atomic<float>* pRateMode = nullptr;
    std::atomic<float>* pSemitones = nullptr;
    std::atomic<float>* pHighPass = nullptr;
    std::atomic<float>* pLowPass = nullptr;
    std::atomic<float>* pNoise = nullptr;
    std::atomic<float>* pNoiseSync = nullptr;

    std::atomic<float> meter { 0.0f };

    double sampleRate = 44100.0;
    double bpm = 120.0;

    double pumpPhase = 0.0;
    float currentGain = 1.0f;

    juce::AudioBuffer<float> pitchDelayBuffer;
    int pitchWritePosition = 0;
    float pitchPhase = 0.0f;

    juce::dsp::IIR::Filter<float> hpFilters[2][filterStages];   // [channel][stage]
    juce::dsp::IIR::Filter<float> lpFilters[2][filterStages];
    double lastHpFreq = -1.0;
    double lastLpFreq = -1.0;

    juce::Random noiseRandom;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaichanProcessor)
};
