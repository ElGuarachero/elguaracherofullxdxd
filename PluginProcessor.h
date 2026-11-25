#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

// EL MAESTRO SAICHAN
// Ducking/pumping tipo sidechain disparado por el kick.
// Entrada principal = audio a procesar. Sidechain (opcional) = señal de disparo (kick).
class SaichanProcessor : public juce::AudioProcessor
{
public:
    SaichanProcessor();
    ~SaichanProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "EL MAESTRO SAICHAN"; }
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

    // Usado por el editor para el medidor de reducción de ganancia (0..1)
    float getMeterLevel() const noexcept { return meter.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<float>* pDepth     = nullptr;
    std::atomic<float>* pThreshold = nullptr;
    std::atomic<float>* pAttack    = nullptr;
    std::atomic<float>* pHold      = nullptr;
    std::atomic<float>* pRelease   = nullptr;
    std::atomic<float>* pMix       = nullptr;
    std::atomic<float>* pUseSC     = nullptr;

    std::atomic<float> meter { 0.0f };

    double sampleRate = 44100.0;
    float  envelope = 0.0f;     // detector de pico del disparador
    float  currentGain = 1.0f;  // ganancia suavizada aplicada
    int    holdRemaining = 0;   // muestras restantes de hold
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaichanProcessor)
};
