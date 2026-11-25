#pragma once

#include "PluginProcessor.h"
#include <array>

// Estética neón azul sobre fondo oscuro, inspirada en la imagen de referencia.
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static inline const juce::Colour neon { 0xff1e90ff };
    static inline const juce::Colour neonCore { 0xffbfe6ff };
    static inline const juce::Colour track { 0xff0b2350 };

    NeonLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;
};

class SaichanEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit SaichanEditor (SaichanProcessor&);
    ~SaichanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); }

    SaichanProcessor& processor;
    NeonLookAndFeel laf;

    static constexpr int numKnobs = 6;
    std::array<juce::Slider, numKnobs> knobs;
    std::array<juce::Label,  numKnobs> knobLabels;

    juce::ToggleButton scButton { "EXT SIDECHAIN" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::unique_ptr<ButtonAttachment> scAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaichanEditor)
};
