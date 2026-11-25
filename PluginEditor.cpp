#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

namespace
{
    constexpr int windowWidth  = 980;
    constexpr int windowHeight = 500;
    constexpr int cellWidth    = 160;
    constexpr int cellGap      = 30;
    constexpr int plateTop     = 110;
    constexpr int plateHeight  = 345;

    int cellX (int index)
    {
        return 30 + index * (cellWidth + cellGap);
    }

    juce::String formatFrequency (double hz)
    {
        if (hz >= 1000.0)
            return juce::String (hz / 1000.0, 2) + " kHz";

        return juce::String (juce::roundToInt (hz)) + " Hz";
    }
}

//==============================================================================
// Look & Feel
//==============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::Label::textColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff050c16));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    setColour (juce::ToggleButton::tickColourId, sky);
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
        (float) x, (float) y, (float) width, (float) height).reduced (14.0f);

    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    // Track
    juce::Path trackArc;
    trackArc.addCentredArc (centre.x, centre.y, radius, radius,
                            0.0f, startAngle, endAngle, true);

    g.setColour (track);
    g.strokePath (trackArc,
        juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                              juce::PathStrokeType::rounded));

    // Active arc with neon glow
    juce::Path activeArc;
    activeArc.addCentredArc (centre.x, centre.y, radius, radius,
                             0.0f, startAngle, angle, true);

    for (int i = 4; i >= 1; --i)
    {
        g.setColour (sky.withAlpha (0.10f * (float) i));
        g.strokePath (activeArc,
            juce::PathStrokeType (5.0f + (float) i * 3.0f,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
    }

    g.setColour (skyCore);
    g.strokePath (activeArc,
        juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                              juce::PathStrokeType::rounded));

    // Knob body with neon border
    const float knobRadius = radius * 0.68f;

    auto knobArea = juce::Rectangle<float> (
        centre.x - knobRadius, centre.y - knobRadius,
        knobRadius * 2.0f, knobRadius * 2.0f);

    for (int i = 3; i >= 1; --i)
    {
        g.setColour (sky.withAlpha (0.08f * (float) i));
        g.drawEllipse (knobArea.expanded ((float) i * 2.0f), 2.0f);
    }

    g.setColour (body);
    g.fillEllipse (knobArea);

    g.setColour (sky);
    g.drawEllipse (knobArea, 2.0f);

    // Pointer
    const float innerRadius = radius * 0.28f;
    const float outerRadius = radius * 0.57f;

    const float px = centre.x + std::sin (angle) * innerRadius;
    const float py = centre.y - std::cos (angle) * innerRadius;
    const float qx = centre.x + std::sin (angle) * outerRadius;
    const float qy = centre.y - std::cos (angle) * outerRadius;

    g.setColour (skyCore);
    g.drawLine (px, py, qx, qy, 3.5f);
}

//==============================================================================
// Editor
//==============================================================================
SaichanEditor::SaichanEditor (SaichanProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      background (juce::ImageFileFormat::loadFrom (BinaryData::fede_rodriguez_jpg,
                                                   BinaryData::fede_rodriguez_jpgSize))
{
    setLookAndFeel (&laf);
    setSize (windowWidth, windowHeight);

    auto setupLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (19.0f, juce::Font::bold));
        addAndMakeVisible (label);
    };

    // RATE
    rateKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    rateKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 110, 24);
    rateKnob.setDoubleClickReturnValue (true, 8.0);

    rateKnob.textFromValueFunction = [this] (double value)
    {
        if (value < 0.01)
            return juce::String ("OFF");

        const bool sync = processor.apvts
            .getRawParameterValue ("ratemode")->load() > 0.5f;

        if (sync)
        {
            const int index = juce::jlimit (
                1, SaichanProcessor::numSyncDivisions, juce::roundToInt (value));

            return juce::String (SaichanProcessor::syncNames[index - 1]);
        }

        return juce::String (value, 2) + " Hz";
    };

    addAndMakeVisible (rateKnob);

    // SEMITONES
    semitonesKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    semitonesKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 110, 24);
    semitonesKnob.setRange (-12.0, 12.0, 0.01);
    semitonesKnob.setDoubleClickReturnValue (true, 0.0);
    semitonesKnob.textFromValueFunction = [] (double value)
    {
        return juce::String (value, 1) + " st";
    };
    addAndMakeVisible (semitonesKnob);

    // HIGH PASS
    highPassKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    highPassKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 110, 24);
    highPassKnob.setNormalisableRange (juce::NormalisableRange<double> (20.0, 2000.0, 0.01, 0.3));
    highPassKnob.setDoubleClickReturnValue (true, 20.0);
    highPassKnob.textFromValueFunction = [] (double value) { return formatFrequency (value); };
    addAndMakeVisible (highPassKnob);

    // LOW PASS
    lowPassKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lowPassKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 110, 24);
    lowPassKnob.setNormalisableRange (juce::NormalisableRange<double> (200.0, 20000.0, 0.01, 0.3));
    lowPassKnob.setDoubleClickReturnValue (true, 20000.0);
    lowPassKnob.textFromValueFunction = [] (double value) { return formatFrequency (value); };
    addAndMakeVisible (lowPassKnob);

    // NOISE
    noiseKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    noiseKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 110, 24);
    noiseKnob.setRange (0.0, 1.0, 0.001);
    noiseKnob.setDoubleClickReturnValue (true, 0.0);
    noiseKnob.textFromValueFunction = [] (double value)
    {
        if (value < 0.001)
            return juce::String ("OFF");

        return juce::String (juce::roundToInt (value * 100.0)) + " %";
    };
    addAndMakeVisible (noiseKnob);

    // Labels
    setupLabel (rateLabel, "RATE");
    setupLabel (semitonesLabel, "SEMITONES");
    setupLabel (highPassLabel, "HIGH PASS");
    setupLabel (lowPassLabel, "LOW PASS");
    setupLabel (noiseLabel, "NOISE");

    // Buttons
    syncButton.setClickingTogglesState (true);
    noiseSyncButton.setClickingTogglesState (true);
    addAndMakeVisible (syncButton);
    addAndMakeVisible (noiseSyncButton);

    // Attachments
    rateAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "rate", rateKnob);

    semitonesAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "semitones", semitonesKnob);

    highPassAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "highpass", highPassKnob);

    lowPassAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "lowpass", lowPassKnob);

    noiseAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "noise", noiseKnob);

    syncAttachment = std::make_unique<ButtonAttachment> (
        processor.apvts, "ratemode", syncButton);

    noiseSyncAttachment = std::make_unique<ButtonAttachment> (
        processor.apvts, "noisesync", noiseSyncButton);

    // Ranges depend on SYNC / HZ. The timer keeps them updated, including
    // when the state changes from a preset or the DAW.
    appliedSync = syncButton.getToggleState();
    applyRateRange (appliedSync);

    startTimerHz (30);
}

SaichanEditor::~SaichanEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SaichanEditor::applyRateRange (bool sync)
{
    appliedSync = sync;

    if (sync)
        rateKnob.setRange (0.0, (double) SaichanProcessor::numSyncDivisions, 1.0);
    else
        rateKnob.setRange (0.0, 32.0, 0.01);

    syncButton.setButtonText (sync ? "SYNC" : "HZ");

    rateKnob.setValue (rateKnob.getValue(), juce::sendNotificationSync);
    rateKnob.repaint();
}

void SaichanEditor::timerCallback()
{
    const bool sync = syncButton.getToggleState();

    if (sync != appliedSync)
        applyRateRange (sync);

    repaint();
}

//==============================================================================
// Background and decoration
//==============================================================================
void SaichanEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float level = juce::jlimit (0.0f, 1.0f, processor.getMeterLevel());

    // Background: the character scene, pulsing slightly with the pump
    {
        juce::Graphics::ScopedSaveState state (g);

        const float zoom = 1.0f + 0.012f * level;
        g.addTransform (juce::AffineTransform::scale (
            zoom, zoom, bounds.getCentreX(), bounds.getCentreY()));

        if (backgroundCache.isValid())
            g.drawImageAt (backgroundCache, 0, 0);
        else
            g.fillAll (juce::Colour (0xff050a14));
    }

    // Neon frame, brighter with each pump
    g.setColour (NeonLookAndFeel::sky.withAlpha (0.55f + 0.45f * level));
    g.drawRoundedRectangle (bounds.reduced (7.0f), 17.0f, 2.0f);

    // Dark plates behind each control group
    for (int i = 0; i < 5; ++i)
    {
        auto plate = juce::Rectangle<float> (
            (float) cellX (i), (float) plateTop,
            (float) cellWidth, (float) plateHeight);

        g.setColour (juce::Colour (0xb8020811));
        g.fillRoundedRectangle (plate, 14.0f);

        g.setColour (NeonLookAndFeel::sky.withAlpha (0.45f));
        g.drawRoundedRectangle (plate.reduced (0.5f), 14.0f, 1.2f);
    }

    // Brand
    g.setFont (juce::Font (28.0f, juce::Font::bold));

    g.setColour (NeonLookAndFeel::sky.withAlpha (0.35f));
    g.drawText ("FEDE RODRIGUEZ",
                juce::Rectangle<int> (0, 14, getWidth(), 34),
                juce::Justification::centred);

    g.setColour (juce::Colours::white);
    g.drawText ("FEDE RODRIGUEZ",
                juce::Rectangle<int> (0, 12, getWidth(), 34),
                juce::Justification::centred);

    // Plugin title
    g.setFont (juce::Font (19.0f, juce::Font::bold));
    g.setColour (NeonLookAndFeel::skyCore);
    g.drawText ("MAESTRO SAICHAN VST",
                juce::Rectangle<int> (0, 50, getWidth(), 30),
                juce::Justification::centred);

    // Pump meter
    auto meterArea = juce::Rectangle<float> (
        (windowWidth - 260) * 0.5f, (float) getHeight() - 25.0f, 260.0f, 7.0f);

    g.setColour (NeonLookAndFeel::track);
    g.fillRoundedRectangle (meterArea, 4.0f);

    g.setColour (NeonLookAndFeel::sky);
    g.fillRoundedRectangle (meterArea.withWidth (meterArea.getWidth() * level), 4.0f);
}

//==============================================================================
// Layout
//==============================================================================
void SaichanEditor::resized()
{
    const auto bounds = getLocalBounds();

    // Scale the photo to cover the window, centred, darkened for legibility
    if (background.isValid())
    {
        backgroundCache = juce::Image (juce::Image::ARGB,
                                       bounds.getWidth(), bounds.getHeight(), true);

        juce::Graphics bg (backgroundCache);

        const float scale = juce::jmax (
            (float) bounds.getWidth()  / (float) background.getWidth(),
            (float) bounds.getHeight() / (float) background.getHeight());

        const float w = background.getWidth() * scale;
        const float h = background.getHeight() * scale;

        bg.drawImage (background, juce::Rectangle<float> (
            (bounds.getWidth() - w) * 0.5f, (bounds.getHeight() - h) * 0.5f, w, h));

        bg.setColour (juce::Colour (0x66020611));
        bg.fillAll();
    }

    rateLabel.setBounds (cellX (0), 120, cellWidth, 26);
    rateKnob.setBounds (cellX (0), 150, cellWidth, 230);
    syncButton.setBounds (cellX (0) + 15, 400, 130, 28);

    semitonesLabel.setBounds (cellX (1), 120, cellWidth, 26);
    semitonesKnob.setBounds (cellX (1), 150, cellWidth, 230);

    highPassLabel.setBounds (cellX (2), 120, cellWidth, 26);
    highPassKnob.setBounds (cellX (2), 150, cellWidth, 230);

    lowPassLabel.setBounds (cellX (3), 120, cellWidth, 26);
    lowPassKnob.setBounds (cellX (3), 150, cellWidth, 230);

    noiseLabel.setBounds (cellX (4), 120, cellWidth, 26);
    noiseKnob.setBounds (cellX (4), 150, cellWidth, 230);
    noiseSyncButton.setBounds (cellX (4) + 15, 400, 130, 28);
}
