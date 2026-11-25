
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Look & Feel
//==============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::Label::textColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff18080c));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    setColour (juce::ToggleButton::tickColourId, neon);
    setColour (juce::ToggleButton::tickDisabledColourId, track);
}

void NeonLookAndFeel::drawRotarySlider (
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float startAngle,
    float endAngle,
    juce::Slider&)
{
    auto bounds = juce::Rectangle<float> (
        (float) x, (float) y, (float) width, (float) height).reduced (12.0f);

    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    // Halo
    g.setColour (neon.withAlpha (0.12f));
    g.fillEllipse (juce::Rectangle<float> (
        centre.x - radius + 2.0f,
        centre.y - radius + 2.0f,
        radius * 2.0f - 4.0f,
        radius * 2.0f - 4.0f));

    // Track
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (
        centre.x, centre.y, radius, radius,
        0.0f, startAngle, endAngle, true);

    g.setColour (track);
    g.strokePath (backgroundArc,
        juce::PathStrokeType (7.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Active arc
    juce::Path activeArc;
    activeArc.addCentredArc (
        centre.x, centre.y, radius, radius,
        0.0f, startAngle, angle, true);

    for (int i = 4; i >= 1; --i)
    {
        g.setColour (neon.withAlpha (0.12f * (float) i));
        g.strokePath (activeArc,
            juce::PathStrokeType (6.0f + (float) i * 3.0f,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
    }

    g.setColour (neonCore);
    g.strokePath (activeArc,
        juce::PathStrokeType (4.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Knob body
    const float knobRadius = radius * 0.68f;

    g.setColour (juce::Colour (0xff180d12));
    g.fillEllipse (
        centre.x - knobRadius, centre.y - knobRadius,
        knobRadius * 2.0f, knobRadius * 2.0f);

    g.setColour (neon.withAlpha (0.9f));
    g.drawEllipse (
        centre.x - knobRadius, centre.y - knobRadius,
        knobRadius * 2.0f, knobRadius * 2.0f, 1.8f);

    // Pointer
    const float innerRadius = radius * 0.28f;
    const float outerRadius = radius * 0.57f;

    const float px = centre.x + std::sin (angle) * innerRadius;
    const float py = centre.y - std::cos (angle) * innerRadius;
    const float qx = centre.x + std::sin (angle) * outerRadius;
    const float qy = centre.y - std::cos (angle) * outerRadius;

    g.setColour (juce::Colours::white);
    g.drawLine (px, py, qx, qy, 3.5f);
}

//==============================================================================
// Editor
//==============================================================================
SaichanEditor::SaichanEditor (SaichanProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&laf);
    setSize (760, 460);

    // RATE
    rateKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    rateKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 24);
    rateKnob.setRange (0.0, 16.0, 1.0);
    rateKnob.setDoubleClickReturnValue (true, 4.0);

    rateKnob.textFromValueFunction = [this] (double value)
    {
        if (value < 0.01)
            return juce::String ("OFF");

        const bool sync = processor.apvts
            .getRawParameterValue ("ratemode")->load() > 0.5f;

        if (sync)
        {
            static const char* divisions[16] =
            {
                "1/1", "1/2", "1/4", "1/8",
                "1/16", "1/32",
                "1/4T", "1/8T", "1/16T", "1/32T",
                "1/1D", "1/2D", "1/4D",
                "1/8D", "1/16D", "1/32D"
            };

            const int index = juce::jlimit (
                1, 16, juce::roundToInt (value));

            return juce::String (divisions[index - 1]);
        }

        return juce::String (value, 2) + " Hz";
    };

    addAndMakeVisible (rateKnob);

    // SEMITONES
    semitonesKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    semitonesKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 24);
    semitonesKnob.setRange (-12.0, 12.0, 0.01);
    semitonesKnob.setDoubleClickReturnValue (true, 0.0);

    semitonesKnob.textFromValueFunction = [] (double value)
    {
        return juce::String (value, 1) + " st";
    };

    addAndMakeVisible (semitonesKnob);

    // Labels
    rateLabel.setText ("RATE", juce::dontSendNotification);
    rateLabel.setJustificationType (juce::Justification::centred);
    rateLabel.setFont (juce::Font (21.0f, juce::Font::bold));
    addAndMakeVisible (rateLabel);

    semitonesLabel.setText ("SEMITONES", juce::dontSendNotification);
    semitonesLabel.setJustificationType (juce::Justification::centred);
    semitonesLabel.setFont (juce::Font (21.0f, juce::Font::bold));
    addAndMakeVisible (semitonesLabel);

    // SYNC / HZ
    syncButton.setClickingTogglesState (true);
    addAndMakeVisible (syncButton);

    // Attachments
    rateAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "rate", rateKnob);

    semitonesAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "semitones", semitonesKnob);

    syncAttachment = std::make_unique<ButtonAttachment> (
        processor.apvts, "ratemode", syncButton);

    syncButton.onClick = [this]
    {
        const bool sync = syncButton.getToggleState();

        rateKnob.setRange (0.0, 16.0, sync ? 1.0 : 0.01);
        syncButton.setButtonText (sync ? "SYNC" : "HZ");

        rateKnob.setValue (
            rateKnob.getValue(), juce::sendNotificationSync);

        rateKnob.repaint();
    };

    syncButton.setButtonText (
        syncButton.getToggleState() ? "SYNC" : "HZ");

    rateKnob.setRange (
        0.0, 16.0,
        syncButton.getToggleState() ? 1.0 : 0.01);

    startTimerHz (30);
}

SaichanEditor::~SaichanEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SaichanEditor::timerCallback()
{
    repaint();
}

//==============================================================================
// Background
//==============================================================================
void SaichanEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient background (
        juce::Colour (0xff090307), 0.0f, 0.0f,
        juce::Colour (0xff300711), 0.0f, bounds.getHeight(), false);

    g.setGradientFill (background);
    g.fillAll();

    // Red ambient light
    g.setColour (NeonLookAndFeel::neon.withAlpha (0.10f));
    g.fillEllipse (getWidth() * 0.25f, 75.0f, 380.0f, 280.0f);

    // Outer frame
    g.setColour (NeonLookAndFeel::neon.withAlpha (0.8f));
    g.drawRoundedRectangle (bounds.reduced (7.0f), 17.0f, 2.0f);

    // Brand
    auto headerArea = getLocalBounds().removeFromTop (75);

    g.setFont (juce::Font (27.0f, juce::Font::bold));
    g.setColour (NeonLookAndFeel::neon.withAlpha (0.35f));
    g.drawText ("FEDE RODRIGUEZ",
        headerArea.translated (0, 2),
        juce::Justification::centred);

    g.setColour (juce::Colours::white);
    g.drawText ("FEDE RODRIGUEZ",
        headerArea,
        juce::Justification::centred);

    // Plugin title
    g.setFont (juce::Font (19.0f, juce::Font::bold));
    g.setColour (NeonLookAndFeel::neonCore);
    g.drawText ("MAESTRO SAICHAN VST",
        getLocalBounds().withTrimmedTop (65).removeFromTop (32),
        juce::Justification::centred);

    // Character / logo placeholder
    auto logoArea = juce::Rectangle<float> (
        getWidth() * 0.5f - 66.0f, 105.0f, 132.0f, 65.0f);

    g.setColour (NeonLookAndFeel::neon.withAlpha (0.12f));
    g.fillRoundedRectangle (logoArea, 12.0f);

    g.setColour (NeonLookAndFeel::neon.withAlpha (0.8f));
    g.drawRoundedRectangle (logoArea, 12.0f, 1.5f);

    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawFittedText ("FEDE RODRIGUEZ",
        logoArea.toNearestInt(),
        juce::Justification::centred, 1);

    // Pump meter
    auto meterArea = juce::Rectangle<float> (
        250.0f, (float) getHeight() - 25.0f, 260.0f, 7.0f);

    g.setColour (NeonLookAndFeel::track);
    g.fillRoundedRectangle (meterArea, 4.0f);

    const float level = juce::jlimit (
        0.0f, 1.0f, processor.getMeterLevel());

    g.setColour (NeonLookAndFeel::neon);
    g.fillRoundedRectangle (
        meterArea.withWidth (meterArea.getWidth() * level), 4.0f);
}

//==============================================================================
// Layout
//==============================================================================
void SaichanEditor::resized()
{
    const int centreX = getWidth() / 2;

    rateLabel.setBounds (95, 190, 230, 30);
    semitonesLabel.setBounds (435, 190, 230, 30);

    rateKnob.setBounds (95, 220, 230, 175);
    semitonesKnob.setBounds (435, 220, 230, 175);

    syncButton.setBounds (centreX - 48, 165, 96, 28);
}
