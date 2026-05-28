#include "OB8LookAndFeel.h"

#if OB8_HAS_BUNDLED_FONT
 #include "BinaryData.h"
#endif

namespace ob8 {

namespace {
juce::Typeface::Ptr loadBundled (const void* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, static_cast<size_t> (size));
}

/*  Fallback when the bundled IBM Plex Mono download didn't succeed at
    configure time. We grab whichever system monospace face is available
    so the editor still renders cleanly. */
juce::Typeface::Ptr loadSystemMono (juce::Font::FontStyleFlags style)
{
    juce::Font::FontStyleFlags resolved = style;
    juce::Font f (juce::FontOptions ("Menlo", 12.0f, resolved));
    if (f.getTypefaceName().isEmpty())
        f = juce::Font (juce::FontOptions ("Monaco", 12.0f, resolved));
    return juce::Typeface::createSystemTypefaceFor (f);
}
} // namespace

juce::Typeface::Ptr OB8LookAndFeel::getMonoRegular()
{
   #if OB8_HAS_BUNDLED_FONT
    static juce::Typeface::Ptr tf = loadBundled (
        BinaryData::IBMPlexMonoRegular_ttf, BinaryData::IBMPlexMonoRegular_ttfSize);
   #else
    static juce::Typeface::Ptr tf = loadSystemMono (juce::Font::plain);
   #endif
    return tf;
}

juce::Typeface::Ptr OB8LookAndFeel::getMonoBold()
{
   #if OB8_HAS_BUNDLED_FONT
    static juce::Typeface::Ptr tf = loadBundled (
        BinaryData::IBMPlexMonoBold_ttf, BinaryData::IBMPlexMonoBold_ttfSize);
   #else
    static juce::Typeface::Ptr tf = loadSystemMono (juce::Font::bold);
   #endif
    return tf;
}

juce::Typeface::Ptr OB8LookAndFeel::getMonoItalic()
{
   #if OB8_HAS_BUNDLED_FONT
    static juce::Typeface::Ptr tf = loadBundled (
        BinaryData::IBMPlexMonoItalic_ttf, BinaryData::IBMPlexMonoItalic_ttfSize);
   #else
    static juce::Typeface::Ptr tf = loadSystemMono (juce::Font::italic);
   #endif
    return tf;
}

juce::Font OB8LookAndFeel::monoRegular (float h) { return juce::Font (getMonoRegular()).withHeight (h); }
juce::Font OB8LookAndFeel::monoBold    (float h) { return juce::Font (getMonoBold())   .withHeight (h); }
juce::Font OB8LookAndFeel::monoItalic  (float h) { return juce::Font (getMonoItalic()) .withHeight (h); }

OB8LookAndFeel::OB8LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, panelCream());
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxTextColourId,         panelDark());
    setColour (juce::Slider::trackColourId,               panelDark());
    setColour (juce::Slider::backgroundColourId,          panelDark().withAlpha (0.18f));
    setColour (juce::Slider::thumbColourId,               panelAccent());
    setColour (juce::Label::textColourId,                 panelDark());
    setColour (juce::ComboBox::backgroundColourId,        panelCream());
    setColour (juce::ComboBox::textColourId,              panelDark());
    setColour (juce::ComboBox::arrowColourId,             panelAccent());
    setColour (juce::ComboBox::outlineColourId,           panelDark());
    setColour (juce::PopupMenu::backgroundColourId,       panelCream());
    setColour (juce::PopupMenu::textColourId,             panelDark());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, panelAccent().withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId,  panelDark());
    setColour (juce::ToggleButton::textColourId,          panelDark());
    setColour (juce::ToggleButton::tickColourId,          panelAccent());
    setColour (juce::TextButton::buttonColourId,          panelCream());
    setColour (juce::TextButton::textColourOnId,          panelAccent());
    setColour (juce::TextButton::textColourOffId,         panelDark());
    setColour (juce::TextEditor::backgroundColourId,      panelCream());
    setColour (juce::TextEditor::textColourId,            panelDark());
    setColour (juce::TextEditor::outlineColourId,         panelDark());

    setDefaultSansSerifTypeface (getMonoRegular());
}

juce::Typeface::Ptr OB8LookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    if (f.getStyleFlags() & juce::Font::italic) return getMonoItalic();
    if (f.getStyleFlags() & juce::Font::bold  ) return getMonoBold();
    return getMonoRegular();
}

/*  Rotary slider -- kept as a fallback for any legacy rotary caller. */
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

/*  HAIRLINE vertical fader:
      * rectangular outer "channel" (hairline outline)
      * thin centre track
      * tick marks (rows) on both sides of the track running the channel
        width, evenly distributed, with the half-way row drawn fully
        across as an emphasised "centre" rule
      * red horizontal thumb that spans the channel width */
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
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos,
                                          minPos, maxPos, style, slider);
        return;
    }

    // Compute the channel bounds: a centred narrow column inside the slider
    const float channelW = juce::jmin (24.0f, static_cast<float> (w) * 0.5f);
    const float cx       = x + w * 0.5f;
    const float top      = y + 2.0f;
    const float bottom   = y + h - 2.0f;
    juce::Rectangle<float> channel (cx - channelW * 0.5f, top, channelW, bottom - top);

    // Outer channel outline
    g.setColour (panelDark());
    g.drawRect (channel, 0.7f);

    // Centre track (thin vertical line through the channel)
    g.drawLine (cx, top + 1.0f, cx, bottom - 1.0f, 0.7f);

    // Tick marks. 11 evenly distributed rows; the middle one (row 5) and
    // the ends (0, 10) get a slightly wider stroke to mark thirds.
    constexpr int kNumTicks = 11;
    const float tickInset   = 2.0f;
    g.setColour (panelDark().withAlpha (0.55f));
    for (int i = 0; i < kNumTicks; ++i)
    {
        const float ty   = top + (bottom - top) * static_cast<float> (i)
                                                / (kNumTicks - 1);
        const bool  emph = (i == 0 || i == 5 || i == 10);
        const float extra = emph ? 2.0f : 0.0f;
        const float x0 = channel.getX() - extra;
        const float x1 = channel.getX() + tickInset;
        const float x2 = channel.getRight() - tickInset;
        const float x3 = channel.getRight() + extra;
        g.drawLine (x0, ty, x1, ty, emph ? 0.7f : 0.5f);
        g.drawLine (x2, ty, x3, ty, emph ? 0.7f : 0.5f);
    }

    // Red horizontal thumb across the channel width
    const float thumbH = 5.0f;
    const float thumbY = juce::jlimit (top + thumbH * 0.5f,
                                       bottom - thumbH * 0.5f,
                                       sliderPos);
    juce::Rectangle<float> thumb (channel.getX() - 1.0f,
                                  thumbY - thumbH * 0.5f,
                                  channel.getWidth() + 2.0f,
                                  thumbH);
    g.setColour (panelAccent());
    g.fillRect (thumb);
    g.setColour (panelDark());
    g.drawRect (thumb, 0.6f);

    juce::ignoreUnused (slider, minPos, maxPos);
}

void OB8LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                       bool /*highlighted*/, bool /*down*/)
{
    auto bounds = b.getLocalBounds().toFloat();
    const float size = juce::jmin (bounds.getHeight(), 16.0f);
    auto sq = juce::Rectangle<float> (bounds.getX(),
                                      bounds.getCentreY() - size * 0.5f,
                                      size, size).reduced (1.0f);

    g.setColour (panelCream());
    g.fillRect (sq);
    g.setColour (panelDark());
    g.drawRect (sq, 0.8f);

    if (b.getToggleState())
    {
        g.setColour (panelAccent());
        g.fillRect (sq.reduced (2.5f));
    }

    bounds.removeFromLeft (size + 5.0f);
    g.setColour (panelDark());
    g.setFont (monoRegular (10.5f));
    g.drawText (b.getButtonText(), bounds.toNearestInt(),
                juce::Justification::centredLeft, false);
}

void OB8LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                   bool /*isDown*/,
                                   int /*buttonX*/, int /*buttonY*/,
                                   int /*buttonW*/, int /*buttonH*/,
                                   juce::ComboBox& cb)
{
    auto bounds = juce::Rectangle<float> (0, 0,
                                          static_cast<float> (width),
                                          static_cast<float> (height))
                      .reduced (0.5f);
    g.setColour (panelCream());
    g.fillRect (bounds);
    g.setColour (panelDark());
    g.drawRect (bounds, 0.7f);

    juce::Path arrow;
    const float ax = width - 9.0f;
    const float ay = height * 0.5f;
    arrow.addTriangle (ax - 3, ay - 2, ax + 3, ay - 2, ax, ay + 2);
    g.setColour (panelAccent());
    g.fillPath (arrow);

    juce::ignoreUnused (cb);
}

void OB8LookAndFeel::positionComboBoxText (juce::ComboBox& cb, juce::Label& l)
{
    l.setBounds (4, 0, cb.getWidth() - 16, cb.getHeight());
    l.setFont   (monoRegular (10.5f));
    l.setColour (juce::Label::textColourId, panelDark());
}

void OB8LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                           const juce::Colour&,
                                           bool /*highlighted*/, bool down)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? panelAccent().withAlpha (0.20f) : panelCream());
    g.fillRect (bounds);
    g.setColour (panelDark());
    g.drawRect (bounds, 0.7f);
}

juce::Font OB8LookAndFeel::getLabelFont      (juce::Label& l)       { return monoRegular (l.getFont().getHeight()); }
juce::Font OB8LookAndFeel::getComboBoxFont   (juce::ComboBox&)      { return monoRegular (10.5f); }
juce::Font OB8LookAndFeel::getTextButtonFont (juce::TextButton&, int){ return monoRegular (10.5f); }
juce::Font OB8LookAndFeel::getPopupMenuFont()                       { return monoRegular (11.0f); }
juce::Font OB8LookAndFeel::getTextEditorFont (juce::TextEditor&)    { return monoRegular (10.5f); }

} // namespace ob8
