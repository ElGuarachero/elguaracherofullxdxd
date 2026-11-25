#include "PluginEditor.h"

//==============================================================================
// Look & feel neón
//==============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::Label::textColourId, juce::Colours::white);
    setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    setColour (juce::ToggleButton::tickColourId, neon);
    setColour (juce::ToggleButton::tickDisabledColourId, track);
}

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (8.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre  = bounds.getCentre();
    const float angle  = startAngle + sliderPos * (endAngle - startAngle);

    juce::PathStrokeType stroke (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    // Pista de fondo
    juce::Path bgArc;
    bgArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (track);
    g.strokePath (bgArc, stroke);

    // Arco de valor con halo neón
    juce::Path valArc;
    valArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (neon.withAlpha (0.12f * (float) i));
        g.strokePath (valArc, juce::PathStrokeType (4.0f + (float) i * 4.0f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }
    g.setColour (neonCore);
    g.strokePath (valArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    // Puntero
    const float px = centre.x + std::sin (angle) * radius * 0.25f;
    const float py = centre.y - std::cos (angle) * radius * 0.25f;
    const float qx = centre.x + std::sin (angle) * radius * 0.85f;
    const float qy = centre.y - std::cos (angle) * radius * 0.85f;
    g.setColour (juce::Colours::white);
    g.drawLine (px, py, qx, qy, 2.5f);
}

//==============================================================================
// Editor
//==============================================================================
namespace
{
    constexpr std::array<const char*, 6> knobNames { "DEPTH", "THRESH", "ATTACK", "HOLD", "RELEASE", "MIX" };
    constexpr std::array<const char*, 6> knobIds   { "depth", "threshold", "attack", "hold", "release", "mix" };

    void drawNeonRect (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float corner)
    {
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (c.withAlpha (0.15f * (float) i));
            g.drawRoundedRectangle (r, corner, 2.0f + (float) i * 2.0f);
        }
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (r, corner, 1.5f);
    }
}

SaichanEditor::SaichanEditor (SaichanProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&laf);
    setSize (800, 380);

    for (int i = 0; i < numKnobs; ++i)
    {
        auto& knob = knobs[(size_t) i];
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
        knob.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        knob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        knob.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (knob);

        auto& label = knobLabels[(size_t) i];
        label.setText (knobNames[(size_t) i], juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (13.0f, juce::Font::bold));
        addAndMakeVisible (label);

        sliderAttachments.push_back (std::make_unique<SliderAttachment> (
            processor.apvts, knobIds[(size_t) i], knob));
    }

    addAndMakeVisible (scButton);
    scAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "usesc", scButton);

    startTimerHz (30);
}

SaichanEditor::~SaichanEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SaichanEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Fondo: degradado azul marino oscuro
    juce::ColourGradient bg (juce::Colour (0xff020b1f), 0.0f, 0.0f,
                             juce::Colour (0xff0b3a8a), 0.0f, bounds.getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    // Marco neón
    drawNeonRect (g, bounds.reduced (6.0f), NeonLookAndFeel::neon, 14.0f);

    // Título con halo
    auto titleArea = juce::Rectangle<float> (0.0f, 14.0f, bounds.getWidth(), 40.0f);
    g.setFont (juce::Font (28.0f, juce::Font::bold));
    for (float off : { -2.0f, 2.0f })
    {
        g.setColour (NeonLookAndFeel::neon.withAlpha (0.35f));
        g.drawText ("EL MAESTRO SAICHAN", titleArea.translated (off, 0.0f), juce::Justification::centred, false);
    }
    g.setColour (juce::Colours::white);
    g.drawText ("EL MAESTRO SAICHAN", titleArea, juce::Justification::centred, false);

    // Medidor de reducción de ganancia (GR)
    auto meterArea = juce::Rectangle<float> (230.0f, bounds.getHeight() - 42.0f,
                                             bounds.getWidth() - 260.0f, 14.0f);
    g.setColour (NeonLookAndFeel::track);
    g.fillRoundedRectangle (meterArea, 4.0f);

    const float gr = juce::jlimit (0.0f, 1.0f, processor.getMeterLevel());
    if (gr > 0.0f)
    {
        g.setColour (NeonLookAndFeel::neon);
        g.fillRoundedRectangle (meterArea.withWidth (meterArea.getWidth() * gr), 4.0f);
    }

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawText ("GAIN REDUCTION", meterArea.translated (0.0f, -16.0f).withHeight (14.0f),
                juce::Justification::left, false);
}

void SaichanEditor::resized()
{
    auto area = getLocalBounds().reduced (24);
    area.removeFromTop (50); // espacio para el título

    auto bottom = area.removeFromBottom (44);
    scButton.setBounds (bottom.removeFromLeft (190).reduced (0, 6));

    const int colWidth = area.getWidth() / numKnobs;
    for (int i = 0; i < numKnobs; ++i)
    {
        auto col = area.removeFromLeft (colWidth);
        knobLabels[(size_t) i].setBounds (col.removeFromTop (20));
        knobs[(size_t) i].setBounds (col.reduced (4));
    }
}
