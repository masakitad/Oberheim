#include "OB8LookAndFeel.h"

namespace ob8 {

OB8LookAndFeel::OB8LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, panelBlue());
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxTextColourId,         panelCream());
    setColour (juce::Label::textColourId,                 panelCream());
    setColour (juce::ComboBox::backgroundColourId,        panelDark());
    setColour (juce::ComboBox::textColourId,              panelCream());
    setColour (juce::ComboBox::arrowColourId,             panelAccent());
    setColour (juce::ComboBox::outlineColourId,           panelCream().withAlpha (0.4f));
    setColour (juce::PopupMenu::backgroundColourId,       panelDark());
    setColour (juce::PopupMenu::textColourId,             panelCream());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, panelOrange());
    setColour (juce::ToggleButton::textColourId,          panelCream());
    setColour (juce::ToggleButton::tickColourId,          panelAccent());
}

void OB8LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                       float pos, float startAng, float endAng,
                                       juce::Slider&)
{
    const float r        = juce::jmin (w, h) * 0.5f - 6.0f;
    const float cx       = x + w * 0.5f;
    const float cy       = y + h * 0.5f;
    const float angle    = startAng + pos * (endAng - startAng);

    // Outer black ring (engraved into the panel)
    g.setColour (panelDark());
    g.fillEllipse (cx - r - 2, cy - r - 2, (r + 2) * 2, (r + 2) * 2);

    // Knob cap: brushed metal gradient
    const juce::ColourGradient cap (
        panelCream().brighter (0.4f), cx, cy - r,
        panelCream().darker (0.6f),   cx, cy + r,
        false);
    g.setGradientFill (cap);
    g.fillEllipse (cx - r, cy - r, r * 2, r * 2);

    // Subtle inner shadow
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawEllipse (cx - r + 1, cy - r + 1, r * 2 - 2, r * 2 - 2, 1.0f);

    // Pointer
    juce::Path p;
    const float pl = r * 0.92f;
    const float pw = 2.5f;
    p.addRectangle (-pw * 0.5f, -pl, pw, pl * 0.55f);
    g.setColour (juce::Colours::black);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (cx, cy));

    // Index dot
    const float dotR  = r + 4.5f;
    const float dotX  = cx + std::sin (angle) * dotR;
    const float dotY  = cy - std::cos (angle) * dotR;
    g.setColour (panelAccent());
    g.fillEllipse (dotX - 2.0f, dotY - 2.0f, 4.0f, 4.0f);
}

void OB8LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                       bool /*highlighted*/, bool /*down*/)
{
    auto bounds = b.getLocalBounds().toFloat();
    auto sq     = bounds.removeFromLeft (bounds.getHeight()).reduced (4.0f);

    g.setColour (panelDark());
    g.fillRoundedRectangle (sq, 2.0f);
    g.setColour (panelCream().withAlpha (0.6f));
    g.drawRoundedRectangle (sq, 2.0f, 1.0f);

    if (b.getToggleState())
    {
        g.setColour (panelOrange());
        g.fillRoundedRectangle (sq.reduced (3.0f), 1.5f);
    }

    g.setColour (panelCream());
    g.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    g.drawText (b.getButtonText(), bounds.toNearestInt(),
                juce::Justification::centredLeft, false);
}

void OB8LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isDown*/,
                                   int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                   juce::ComboBox& cb)
{
    auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);
    g.setColour (panelDark());
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (panelCream().withAlpha (0.4f));
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    juce::Path arrow;
    const float ax = width - 12.0f, ay = height * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (panelAccent());
    g.fillPath (arrow);

    juce::ignoreUnused (cb);
}

juce::Font OB8LookAndFeel::getLabelFont (juce::Label& l)
{
    return juce::Font (juce::FontOptions (l.getFont().getHeight()).withStyle ("Bold"));
}

juce::Font OB8LookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (13.0f).withStyle ("Bold"));
}

} // namespace ob8
