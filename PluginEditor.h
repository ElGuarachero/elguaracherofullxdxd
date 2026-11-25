#pragma once

#include "PluginProcessor.h"

//==============================================================================
// Estética azul celeste con neón
//==============================================================================
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static inline const juce::Colour sky     { 0xff7fd9ff };
    static inline const juce::Colour skyCore { 0xffd9f5ff };
    static inline const juce::Colour track   { 0xff0c2033 };
    static inline const juce::Colour body    { 0xf0060c16 };

    NeonLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;
};

//==============================================================================
// Gráfico Kickstart: muestra la curva del sidechain y el punto actual
//==============================================================================
class KickstartGraph : public juce::Component
{
public:
    KickstartGraph (SaichanProcessor& p, juce::Slider& depth, juce::Slider& rate);
    void paint (juce::Graphics&) override;

private:
    SaichanProcessor& processor;
    juce::Slider& depthKnob;
    juce::Slider& rateKnob;
};

//==============================================================================
// Plugin Editor
//==============================================================================
class SaichanEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit SaichanEditor (SaichanProcessor&);
    ~SaichanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void applyRateRange (bool sync);

    SaichanProcessor& processor;
    NeonLookAndFeel laf;

    juce::Image background;
    juce::Image logo;

    juce::Slider rateKnob;
    juce::Slider volumeKnob;
    juce::Slider semitonesKnob;
    juce::Slider highPassKnob;
    juce::Slider lowPassKnob;
    juce::Slider noiseKnob;

    juce::Label rateLabel;
    juce::Label volumeLabel;
    juce::Label semitonesLabel;
    juce::Label highPassLabel;
    juce::Label lowPassLabel;
    juce::Label noiseLabel;

    juce::ToggleButton syncButton { "SYNC" };
    juce::ToggleButton noiseSyncButton { "NOISE SYNC" };

    KickstartGraph kickstart;

    bool appliedSync = false;

    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    using ButtonAttachment =
        juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> rateAttachment;
    std::unique_ptr<SliderAttachment> volumeAttachment;
    std::unique_ptr<SliderAttachment> semitonesAttachment;
    std::unique_ptr<SliderAttachment> highPassAttachment;
    std::unique_ptr<SliderAttachment> lowPassAttachment;
    std::unique_ptr<SliderAttachment> noiseAttachment;

    std::unique_ptr<ButtonAttachment> syncAttachment;
    std::unique_ptr<ButtonAttachment> noiseSyncAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaichanEditor)
};
