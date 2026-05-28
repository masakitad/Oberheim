#include "OB8LookAndFeel.h"

namespace ob8 {

OB8LookAndFeel::OB8LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, panelBlue());
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxTextColourId,         panelDark());
    setColour (juce::Slider::trackColourId,               panelDark());
    setColour (juce::Slider::backgroundColourId,          panelDark().withAlpha (0.20f));
    setColour (juce::Slider::thumbColourId,               panelAccent());
    setColour (juce::Label::textColourId,                 panelDark());
    setColour (juce::ComboBox::backgroundColourId,        panelCream());
    setColour (juce::ComboBox::textColourId,              panelDark());
    setColour (juce::ComboBox::arrowColourId,             panelAccent());
    setColour (juce::ComboBox::outlineColourId,           panelDark());
    setColour (juce::PopupMenu::backgroundColourId,       panelCream());
    setColour (juce::PopupMenu::textColourId,             panelDark());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, panelAccent().withAlpha (0.20f));
    setColour (juce::PopupMenu::highlightedTextColourId,  panelDark());
    setColour (juce::ToggleButton::textColourId,          panelDark());
    setColour (juce::ToggleButton::tickColourId,          panelAccent());
    setColour (juce::TextButton::buttonColourId,          panelCream());
    setColour (juce::TextButton::textColourOnId,          panelAccent());
    setColour (juce::TextButton::textColourOffId,         panelDark());
    setColour (juce::TextEditor::backgroundColourId,      panelCream());
    setColour (juce::TextEditor::textColourId,            panelDark());
    setColour (juce::TextEditor::outlineColourId,         panelDark());
}

/*  Rotary slider is no longer used (we draw vertical faders instead) but we
    keep this implementation so any leftover rotary callers fall back to the
    same hairline aesthetic. */
void OB8LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                       float pos, float startAng, float endAng,
                                       juce::Slider&)
{
    const float r     = juce::jmin (w, h) * 0.5f - 4.0f;
    const float cx    = x + w * 0.5f;
    const float cy    = y + h * 0.5f;
    const float angle = startAng + pos * (endAng - startAng);

    g.setColour (panelCream().darker (0.05f));
    g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    g.setColour (panelDark());
    g.drawEllipse (cx - r, cy - r, r * 2, r * 2, 1.0f);

    juce::Path p;
    p.addRectangle (-1.0f, -r * 0.92f, 2.0f, r * 0.50f);
    g.setColour (panelAccent());
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (cx, cy));
}

/*  Vertical linear slider drawing. Matches the "HAIRLINE" design language:
      - thin black track in the centre with 5 horizontal tick marks
      - red rectangular thumb
      - cream background
      - subtle hairline frame around the track area */
void OB8LookAndFeel::drawLinearSlider (juce::Graphics& g,
                                       int x, int y, int w, int h,
                                       float sliderPos,
                                       float minPos, float maxPos,
                                       const juce::Slider::SliderStyle style,
                                       juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical
        && style != juce::Slider::LinearBarVertical)
    {
        // Fallback to default for non-vertical orientations
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos,
                                          minPos, maxPos, style, slider);
        return;
    }

    const float cx = x + w * 0.5f;
    const float top    = y + 4.0f;
    const float bottom = y + h - 4.0f;

    // Track (hairline)
    g.setColour (panelDark());
    g.drawLine (cx, top, cx, bottom, 1.0f);

    // 5 tick marks evenly distributed -- top/middle/bottom emphasised
    const float tickWide = 8.0f;
    const float tickThin = 4.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float ty = top + (bottom - top) * static_cast<float> (i) / 4.0f;
        const float halfW = (i == 0 || i == 2 || i == 4) ? tickWide : tickThin;
        g.setColour (panelDark().withAlpha (0.55f));
        g.drawLine (cx - halfW, ty, cx + halfW, ty, 1.0f);
    }

    // Thumb: red rectangle marker.
    const float thumbH = 6.0f;
    const float thumbW = 22.0f;
    const float ty     = juce::jlimit (top, bottom, sliderPos);
    g.setColour (panelAccent());
    g.fillRect (cx - thumbW * 0.5f, ty - thumbH * 0.5f, thumbW, thumbH);
    g.setColour (panelDark());
    g.drawRect (cx - thumbW * 0.5f, ty - thumbH * 0.5f, thumbW, thumbH, 0.6f);

    juce::ignoreUnused (slider, minPos, maxPos);
}

void OB8LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                       bool /*highlighted*/, bool /*down*/)
{
    auto bounds = b.getLocalBounds().toFloat();
    const float size = juce::jmin (bounds.getHeight(), 18.0f);
    auto sq = juce::Rectangle<float> (bounds.getX(), bounds.getCentreY() - size * 0.5f,
                                      size, size).reduced (1.0f);

    g.setColour (panelCream());
    g.fillRect (sq);
    g.setColour (panelDark());
    g.drawRect (sq, 1.0f);

    if (b.getToggleState())
    {
        g.setColour (panelAccent());
        g.fillRect (sq.reduced (3.0f));
    }

    bounds.removeFromLeft (size + 6.0f);
    g.setColour (panelDark());
    g.setFont (juce::Font (juce::FontOptions (11.0f).withTypefaceStyle ("Bold")));
    g.drawText (b.getButtonText(), bounds.toNearestInt(),
                juce::Justification::centredLeft, false);
}

void OB8LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isDown*/,
                                   int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                   juce::ComboBox& cb)
{
    auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    g.setColour (panelCream());
    g.fillRect (bounds);
    g.setColour (panelDark());
    g.drawRect (bounds, 1.0f);

    juce::Path arrow;
    const float ax = width - 9.0f, ay = height * 0.5f;
    arrow.addTriangle (ax - 3, ay - 2, ax + 3, ay - 2, ax, ay + 2);
    g.setColour (panelAccent());
    g.fillPath (arrow);

    juce::ignoreUnused (cb);
}

void OB8LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                           const juce::Colour&,
                                           bool /*highlighted*/, bool down)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? panelAccent().withAlpha (0.25f) : panelCream());
    g.fillRect (bounds);
    g.setColour (panelDark());
    g.drawRect (bounds, 1.0f);
}

juce::Font OB8LookAndFeel::getLabelFont (juce::Label& l)
{
    return juce::Font (juce::FontOptions (l.getFont().getHeight()).withTypefaceStyle ("Plain"));
}

juce::Font OB8LookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (12.0f).withTypefaceStyle ("Plain"));
}

juce::Font OB8LookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return juce::Font (juce::FontOptions (11.0f).withTypefaceStyle ("Plain"));
}

} // namespace ob8
