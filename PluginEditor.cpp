
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Look & Feel: azul neón
//==============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::Label::textColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff071426));
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

    // Halo exterior
    g.setColour (neon.withAlpha (0.08f));
    g.fillEllipse (juce::Rectangle<float> (centre.x - radius + 2.0f,
                                           centre.y - radius + 2.0f,
                                           radius * 2.0f - 4.0f,
                                           radius * 2.0f - 4.0f));

    // Pista
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centre.x, centre.y, radius, radius,
                                 0.0f, startAngle, endAngle, true);

    g.setColour (track);
    g.strokePath (backgroundArc,
                  juce::PathStrokeType (6.0f,
                                        juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded));

    // Arco activo
    juce::Path activeArc;
    activeArc.addCentredArc (centre.x, centre.y, radius, radius,
                             0.0f, startAngle, angle, true);

    for (int i = 4; i >= 1; --i)
    {
        g.setColour (neon.withAlpha (0.10f * (float) i));
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

    // Centro de la perilla
    const float knobRadius = radius * 0.68f;

    g.setColour (juce::Colour (0xff101d30));
    g.fillEllipse (centre.x - knobRadius, centre.y - knobRadius,
                   knobRadius * 2.0f, knobRadius * 2.0f);

    g.setColour (neon.withAlpha (0.8f));
    g.drawEllipse (centre.x - knobRadius, centre.y - knobRadius,
                   knobRadius * 2.0f, knobRadius * 2.0f, 1.5f);

    // Indicador
    const float innerRadius = radius * 0.28f;
    const float outerRadius = radius * 0.57f;

    const float px = centre.x + std::sin (angle) * innerRadius;
    const float py = centre.y - std::cos (angle) * innerRadius;
    const float qx = centre.x + std::sin (angle) * outerRadius;
    const float qy = centre.y - std::cos (angle) * outerRadius;

    g.setColour (juce::Colours::white);
    g.drawLine (px, py, qx, qy, 3.0f);
}

//==============================================================================
// Editor
//==============================================================================
SaichanEditor::SaichanEditor (SaichanProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&laf);
    setSize (760, 440);

    // RATE
    rateKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    rateKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);
    rateKnob.setRange (0.0, 4.0, 1.0);
    rateKnob.setDoubleClickReturnValue (true, 2.0);
    addAndMakeVisible (rateKnob);

    rateKnob.textFromValueFunction = [this] (double value)
    {
        const bool sync = processor.apvts.getRawParameterValue ("ratemode")->load() > 0.5f;

        if (sync)
        {
            static const char* divisions[] =
            {
                "1/1", "1/2", "1/4", "1/8", "1/16"
            };

            const int index = juce::jlimit (0, 4, juce::roundToInt (value));
            return juce::String (divisions[index]);
        }

        const float hz = 0.25f + ((float) value / 4.0f) * 7.75f;
        return juce::String (hz, 2) + " Hz";
    };

    // SEMITONES
    semitonesKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    semitonesKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);
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
    rateLabel.setFont (juce::Font (19.0f, juce::Font::bold));
    addAndMakeVisible (rateLabel);

    semitonesLabel.setText ("SEMITONES", juce::dontSendNotification);
    semitonesLabel.setJustificationType (juce::Justification::centred);
    semitonesLabel.setFont (juce::Font (19.0f, juce::Font::bold));
    addAndMakeVisible (semitonesLabel);

    // SYNC / HZ selector
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

        rateKnob.setRange (0.0, 4.0, sync ? 1.0 : 0.01);
        rateKnob.setValue (rateKnob.getValue(), juce::sendNotificationSync);
        syncButton.setButtonText (sync ? "SYNC" : "HZ");
        rateKnob.updateText();
    };

    syncButton.setButtonText (
        syncButton.getToggleState() ? "SYNC" : "HZ");

    rateKnob.setRange (
        0.0, 4.0,
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
// Background and graphics
//==============================================================================
void SaichanEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient background (
        juce::Colour (0xff020711), 0.0f, 0.0f,
        juce::Colour (0xff0b2850), 0.0f, bounds.getHeight(), false);

    g.setGradientFill (background);
    g.fillAll();

    // Decorative light
    g.setColour (NeonLookAndFeel::neon.withAlpha (0.07f));
    g.fillEllipse (getWidth() * 0.28f, 55.0f, 330.0f, 250.0f);

    // Outer frame
    g.setColour (NeonLookAndFeel::neon.withAlpha (0.7f));
    g.drawRoundedRectangle (bounds.reduced (7.0f), 16.0f, 2.0f);

    // Header
    g.setFont (juce::Font (27.0f, juce::Font::bold));
    g.setColour (NeonLookAndFeel::neon.withAlpha (0.35f));
    g.drawText ("EL MAESTRO SAICHAN",
                getLocalBounds().removeFromTop (58).translated (0, 2),
                juce::Justification::centred);

    g.setColour (juce::Colours::white);
    g.drawText ("EL MAESTRO SAICHAN",
                getLocalBounds().removeFromTop (58),
                juce::Justification::centred);

    // Character placeholder area
    auto characterArea = juce::Rectangle<float> (
        getWidth() * 0.5f - 65.0f, 72.0f, 130.0f, 112.0f);

    g.setColour (NeonLookAndFeel::neon.withAlpha (0.10f));
    g.fillRoundedRectangle (characterArea, 18.0f);

    g.setColour (NeonLookAndFeel::neon.withAlpha (0.65f));
    g.drawRoundedRectangle (characterArea, 18.0f, 1.5f);

    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawFittedText ("SAICHAN",
                      characterArea.toNearestInt(),
                      juce::Justification::centred,
                      1);

    // Pump meter
    auto meterArea = juce::Rectangle<float> (
        250.0f, (float) getHeight() - 30.0f, 260.0f, 8.0f);

    g.setColour (NeonLookAndFeel::track);
    g.fillRoundedRectangle (meterArea, 4.0f);

    const float level = juce::jlimit (0.0f, 1.0f, processor.getMeterLevel());

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

    rateLabel.setBounds (100, 205, 220, 28);
    semitonesLabel.setBounds (440, 205, 220, 28);

    rateKnob.setBounds (100, 230, 220, 165);
    semitonesKnob.setBounds (440, 230, 220, 165);

    syncButton.setBounds (centreX - 45, 175, 90, 28);
}
