#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

namespace
{
    constexpr int windowSize = 800;
    constexpr int knobW = 110;
    constexpr int knobGap = 10;
    constexpr int rowX0 = (windowSize - (6 * knobW + 5 * knobGap)) / 2;
    constexpr int labelY = 596;
    constexpr int knobY = 618;
    constexpr int knobH = 124;
    constexpr int buttonY = 748;

    int cellX (int index)
    {
        return rowX0 + index * (knobW + knobGap);
    }

    // Puntos de la cara en proporción de la imagen (0 a 1). Ajustables.
    struct FacePoint
    {
        float x, y, radius;
    };

    constexpr FacePoint eyeLeft  { 0.478f, 0.195f, 0.045f };   // RATE
    constexpr FacePoint eyeRight { 0.570f, 0.195f, 0.045f };   // RATE
    constexpr FacePoint nose     { 0.522f, 0.245f, 0.035f };   // NOISE
    constexpr FacePoint mouth    { 0.522f, 0.300f, 0.050f };   // NOISE
    constexpr FacePoint earLeft  { 0.410f, 0.255f, 0.040f };   // HIGH PASS
    constexpr FacePoint earRight { 0.626f, 0.255f, 0.040f };   // LOW PASS

    float proportion (const juce::Slider& s)
    {
        return (float) s.valueToProportionOfLength (s.getValue());
    }

    juce::String formatFrequency (double hz)
    {
        if (hz >= 1000.0)
            return juce::String (hz / 1000.0, 2) + " kHz";

        return juce::String (juce::roundToInt (hz)) + " Hz";
    }

    // "hinchar" una zona: dibuja la foto agrandada dentro de un círculo
    void drawBulge (juce::Graphics& g, const juce::Image& photo,
                    const FacePoint& spot, float amount)
    {
        if (amount < 0.01f)
            return;

        const float cx = spot.x * windowSize;
        const float cy = spot.y * windowSize;
        const float r  = spot.radius * windowSize;

        {
            juce::Graphics::ScopedSaveState state (g);

            juce::Path mask;
            mask.addEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
            g.reduceClipRegion (mask);

            const float s = 1.0f + 0.35f * amount;
            g.addTransform (juce::AffineTransform::scale (s, s, cx, cy));
            g.drawImageAt (photo, 0, 0);
        }

        g.setColour (NeonLookAndFeel::sky.withAlpha (0.6f * amount));
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);
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
        (float) x, (float) y, (float) width, (float) height).reduced (12.0f);

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

    // Arco activo con glow neón
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

    // Cuerpo de la perilla con borde neón
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

    // Puntero
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
// Kickstart graph
//==============================================================================
KickstartGraph::KickstartGraph (SaichanProcessor& p,
                                juce::Slider& depth,
                                juce::Slider& rate)
    : processor (p), depthKnob (depth), rateKnob (rate)
{
}

void KickstartGraph::paint (juce::Graphics& g)
{
    const auto box = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xc8020811));
    g.fillRoundedRectangle (box, 12.0f);

    g.setColour (NeonLookAndFeel::sky.withAlpha (0.5f));
    g.drawRoundedRectangle (box.reduced (0.5f), 12.0f, 1.2f);

    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.setColour (NeonLookAndFeel::skyCore);
    g.drawText ("KICKSTART", box.withHeight (26.0f).reduced (12.0f, 4.0f),
                juce::Justification::left, false);

    const auto plot = box.reduced (14.0f, 10.0f).withTrimmedTop (24.0f);

    const float depth = (float) depthKnob.getValue();
    const float minGain = 1.0f - depth * (1.0f - 0.22f);
    const bool active = rateKnob.getValue() > 0.01;

    auto gainAt = [minGain, active] (float x)
    {
        return active ? minGain + (1.0f - minGain) * x * x : 1.0f;
    };

    auto toY = [plot] (float gain)
    {
        return plot.getBottom() - (gain - 0.22f) / 0.78f * plot.getHeight();
    };

    juce::Path curve;

    for (int i = 0; i <= 64; ++i)
    {
        const float x = (float) i / 64.0f;
        const juce::Point<float> p (plot.getX() + x * plot.getWidth(),
                                    toY (gainAt (x)));

        if (i == 0)
            curve.startNewSubPath (p);
        else
            curve.lineTo (p);
    }

    for (int i = 4; i >= 1; --i)
    {
        g.setColour (NeonLookAndFeel::sky.withAlpha (0.12f * (float) i));
        g.strokePath (curve, juce::PathStrokeType ((float) i * 3.0f));
    }

    g.setColour (NeonLookAndFeel::skyCore);
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    if (active)
    {
        const float phase = processor.getVisualPhase();
        const float dotX = plot.getX() + phase * plot.getWidth();
        const float dotY = toY (gainAt (phase));

        g.setColour (NeonLookAndFeel::sky.withAlpha (0.35f));
        g.fillEllipse (dotX - 10.0f, dotY - 10.0f, 20.0f, 20.0f);

        g.setColour (NeonLookAndFeel::skyCore);
        g.fillEllipse (dotX - 5.0f, dotY - 5.0f, 10.0f, 10.0f);
    }
}

//==============================================================================
// Editor
//==============================================================================
SaichanEditor::SaichanEditor (SaichanProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      background (juce::ImageFileFormat::loadFrom (BinaryData::fede_rodriguez_jpg,
                                                   BinaryData::fede_rodriguez_jpgSize)),
      logo (juce::ImageFileFormat::loadFrom (BinaryData::logo_png,
                                             BinaryData::logo_pngSize)),
      kickstart (p, volumeKnob, rateKnob)
{
    setLookAndFeel (&laf);
    setSize (windowSize, windowSize);

    auto styleKnob = [] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, knobW, 20);
    };

    auto setupLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (14.0f, juce::Font::bold));
        addAndMakeVisible (label);
    };

    // RATE
    styleKnob (rateKnob);
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

    // VOLUME (cuánto se aplica el efecto)
    styleKnob (volumeKnob);
    volumeKnob.setRange (0.0, 1.0, 0.01);
    volumeKnob.setDoubleClickReturnValue (true, 1.0);
    volumeKnob.textFromValueFunction = [] (double value)
    {
        return juce::String (juce::roundToInt (value * 100.0)) + " %";
    };
    addAndMakeVisible (volumeKnob);

    // SEMITONES
    styleKnob (semitonesKnob);
    semitonesKnob.setRange (-12.0, 12.0, 0.01);
    semitonesKnob.setDoubleClickReturnValue (true, 0.0);
    semitonesKnob.textFromValueFunction = [] (double value)
    {
        return juce::String (value, 1) + " st";
    };
    addAndMakeVisible (semitonesKnob);

    // HIGH PASS
    styleKnob (highPassKnob);
    highPassKnob.setNormalisableRange (juce::NormalisableRange<double> (20.0, 2000.0, 0.01, 0.3));
    highPassKnob.setDoubleClickReturnValue (true, 20.0);
    highPassKnob.textFromValueFunction = [] (double value) { return formatFrequency (value); };
    addAndMakeVisible (highPassKnob);

    // LOW PASS
    styleKnob (lowPassKnob);
    lowPassKnob.setNormalisableRange (juce::NormalisableRange<double> (200.0, 20000.0, 0.01, 0.3));
    lowPassKnob.setDoubleClickReturnValue (true, 20000.0);
    lowPassKnob.textFromValueFunction = [] (double value) { return formatFrequency (value); };
    addAndMakeVisible (lowPassKnob);

    // NOISE
    styleKnob (noiseKnob);
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
    setupLabel (volumeLabel, "VOLUME");
    setupLabel (semitonesLabel, "SEMITONES");
    setupLabel (highPassLabel, "HIGH PASS");
    setupLabel (lowPassLabel, "LOW PASS");
    setupLabel (noiseLabel, "NOISE");

    // Buttons
    syncButton.setClickingTogglesState (true);
    noiseSyncButton.setClickingTogglesState (true);
    addAndMakeVisible (syncButton);
    addAndMakeVisible (noiseSyncButton);

    addAndMakeVisible (kickstart);

    // Attachments
    rateAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "rate", rateKnob);

    volumeAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "depth", volumeKnob);

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
// Paint
//==============================================================================
void SaichanEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Fondo fijo: la escena completa
    if (background.isValid())
        g.drawImage (background, bounds);
    else
        g.fillAll (juce::Colour (0xff050a14));

    // Partes de la cara que se hinchan
    drawBulge (g, background, eyeLeft,  proportion (rateKnob));
    drawBulge (g, background, eyeRight, proportion (rateKnob));
    drawBulge (g, background, mouth,    proportion (noiseKnob));
    drawBulge (g, background, nose,     proportion (noiseKnob));
    drawBulge (g, background, earLeft,  proportion (highPassKnob));
    drawBulge (g, background, earRight, proportion (lowPassKnob));

    // Logo centrado en todo el plugin
    if (logo.isValid())
        g.drawImageWithin (logo, 280, 280, 240, 240,
                           juce::RectanglePlacement::centred);

    // Marco neón
    g.setColour (NeonLookAndFeel::sky.withAlpha (0.7f));
    g.drawRoundedRectangle (bounds.reduced (7.0f), 17.0f, 2.0f);

    // Placa oscura detrás de los controles
    auto plate = juce::Rectangle<float> ((float) rowX0 - 14.0f, 584.0f,
                                         (float) (6 * knobW + 5 * knobGap + 28), 200.0f);

    g.setColour (juce::Colour (0xc4020811));
    g.fillRoundedRectangle (plate, 16.0f);

    g.setColour (NeonLookAndFeel::sky.withAlpha (0.45f));
    g.drawRoundedRectangle (plate.reduced (0.5f), 16.0f, 1.2f);
}

//==============================================================================
// Layout
//==============================================================================
void SaichanEditor::resized()
{
    kickstart.setBounds (24, 24, 260, 118);

    rateLabel.setBounds (cellX (0), labelY, knobW, 20);
    rateKnob.setBounds (cellX (0), knobY, knobW, knobH);
    syncButton.setBounds (cellX (0) + 8, buttonY, 94, 24);

    volumeLabel.setBounds (cellX (1), labelY, knobW, 20);
    volumeKnob.setBounds (cellX (1), knobY, knobW, knobH);

    semitonesLabel.setBounds (cellX (2), labelY, knobW, 20);
    semitonesKnob.setBounds (cellX (2), knobY, knobW, knobH);

    highPassLabel.setBounds (cellX (3), labelY, knobW, 20);
    highPassKnob.setBounds (cellX (3), knobY, knobW, knobH);

    lowPassLabel.setBounds (cellX (4), labelY, knobW, 20);
    lowPassKnob.setBounds (cellX (4), knobY, knobW, knobH);

    noiseLabel.setBounds (cellX (5), labelY, knobW, 20);
    noiseKnob.setBounds (cellX (5), knobY, knobW, knobH);
    noiseSyncButton.setBounds (cellX (5) + 2, buttonY, 106, 24);
}
