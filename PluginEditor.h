
#pragma once

#include "PluginProcessor.h"

//==============================================================================
// Estética neón azul
//==============================================================================
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static inline const juce::Colour neon     { 0xff168cff };
    static inline const juce::Colour neonCore { 0xffbfe6ff };
    static inline const juce::Colour track    { 0xff102746 };

    NeonLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;
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

    SaichanProcessor& processor;
    NeonLookAndFeel laf;

    juce::Slider rateKnob;
    juce::Slider semitonesKnob;

    juce::Label rateLabel;
    juce::Label semitonesLabel;

    juce::ToggleButton syncButton { "SYNC" };

    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    using ButtonAttachment =
        juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> rateAttachment;
    std::unique_ptr<SliderAttachment> semitonesAttachment;

    std::unique_ptr<ButtonAttachment> syncAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaichanEditor)
};
