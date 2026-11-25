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

    struct FacePoint
    {
        float x, y, radius;
    };

    constexpr FacePoint eyeLeft  { 0.478f, 0.195f, 0.045f };
    constexpr FacePoint eyeRight { 0.570f, 0.195f, 0.045f };
    constexpr FacePoint nose     { 0.522f, 0.245f, 0.035f };
    constexpr FacePoint mouth    { 0.522f, 0.300f, 0.050f };
    constexpr FacePoint earLeft  { 0.410f, 0.255f, 0.040f };
    constexpr FacePoint earRight { 0.626f, 0.255f, 0.040f };

    float proportion (const juce::Slider& s)
    {
        const double range = s.getMaximum() - s.getMinimum();

        if (range <= 0.0)
            return 0.0f;

        return (float) ((s.getValue() - s.getMinimum()) / range);
    }

    juce::String formatFrequency (double hz)
    {
        if (hz >= 1000.0)
            return juce::String (hz / 1000.0, 2) + " kHz";

        return juce::String (juce::roundToInt (hz)) + " Hz";
    }

    void drawBulge (juce::Graphics& g, const juce::Image& photo,
                    const FacePoint& spot, float amount)
    {
        if (amount < 0.01f || ! photo.isValid())
            return;

        const float cx = spot.x * windowSize;
        const float cy = spot.y * windowSize;
        const float r  = spot.radius * windowSize;
        const float strength = 0.5f * amount;

        const int x0 = juce::jmax (0, (int) std::floor (cx - r));
        const int y0 = juce::jmax (0, (int) std::floor (cy - r));
        const int x1 = juce::jmin (photo.getWidth(),  (int) std::ceil (cx + r));
        const int y1 = juce::jmin (photo.getHeight(), (int) std::ceil (cy + r));

        const int w = x1 - x0;
        const int h = y1 - y0;

        if (w <= 0 || h <= 0)
            return;

        juce::Image bulged (juce::Image::ARGB, w, h, true);

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const float dx = (float) (x0 + x) + 0.5f - cx;
                const float dy = (float) (y0 + y) + 0.5f - cy;
                const float d  = std::sqrt (dx * dx + dy * dy);

                if (d >= r)


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

    styleKnob (volumeKnob);
    volumeKnob.setRange (0.0, 1.0, 0.01);
    volumeKnob.setDoubleClickReturnValue (true, 1.0);
    volumeKnob.textFromValueFunction = [] (double value)
    {
        return juce::String (juce::roundToInt (value * 100.0)) + " %";
    };
    addAndMakeVisible (volumeKnob);

    styleKnob (semitonesKnob);
    semitonesKnob.setRange (-12.0, 12.0, 0.01);
    semitonesKnob.setDoubleClickReturnValue (true, 0.0);
    semitonesKnob.textFromValueFunction = [] (double value)
    {
        return juce::String (value, 1) + " st";
    };
    addAndMakeVisible (semitonesKnob);

    styleKnob (highPassKnob);
    highPassKnob.setNormalisableRange (juce::NormalisableRange<double> (20.0, 2000.0, 0.01, 0.3));
    highPassKnob.setDoubleClickReturnValue (true, 20.0);
    highPassKnob.textFromValueFunction = [] (double value) { return formatFrequency (value); };
    addAndMakeVisible (highPassKnob);

    styleKnob (lowPassKnob);
    lowPassKnob.setNormalisableRange (juce::NormalisableRange<double> (200.0, 20000.0, 0.01, 0.3));
    lowPassKnob.setDoubleClickReturnValue (true, 20000.0);
    lowPassKnob.textFromValueFunction = [] (double value) { return formatFrequency (value); };
    addAndMakeVisible (lowPassKnob);

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

    setupLabel (rateLabel, "RATE");
    setupLabel (volumeLabel, "VOLUME");
    setupLabel (semitonesLabel, "SEMITONES");
    setupLabel (highPassLabel, "HIGH PASS");
    setupLabel (lowPassLabel, "LOW PASS");
    setupLabel (noiseLabel, "NOISE");

    syncButton.setClickingTogglesState (true);
    noiseSyncButton.setClickingTogglesState (true);
    addAndMakeVisible (syncButton);
    addAndMakeVisible (noiseSyncButton);

    addAndMakeVisible (kickstart);

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

    if (background.isValid())
        g.drawImage (background, bounds);
    else
        g.fillAll (juce::Colour (0xff050a14));

    drawBulge (g, background, eyeLeft,  proportion (rateKnob));
    drawBulge (g, background, eyeRight, proportion (rateKnob));
    drawBulge (g, background, mouth,    proportion (noiseKnob));
    drawBulge (g, background, nose,     proportion (noiseKnob));
    drawBulge (g, background, earLeft,  proportion (highPassKnob));
    drawBulge (g, background, earRight, proportion (lowPassKnob));

    if (logo.isValid())
        g.drawImageWithin (logo, 280, 280, 240, 240,
                           juce::RectanglePlacement::centred);

    g.setColour (NeonLookAndFeel::sky.withAlpha (0.7f));
    g.drawRoundedRectangle (bounds.reduced (7.0f), 17.0f, 2.0f);

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
