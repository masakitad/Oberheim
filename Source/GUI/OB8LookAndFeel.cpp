#include "OB8LookAndFeel.h"

#if OB8_HAS_BUNDLED_FONT
 #include "BinaryData.h"
#endif

namespace ob8 {

namespace {

juce::Typeface::Ptr loadSystemMono (bool bold, bool italic)
{
    juce::String name = juce::Font::getDefaultMonospacedFontName();
    if (name.isEmpty()) name = "Menlo";

    int flags = juce::Font::plain;
    if (bold)   flags |= juce::Font::bold;
    if (italic) flags |= juce::Font::italic;
    return juce::Typeface::createSystemTypefaceFor (juce::Font (name, 12.0f, flags));
}

juce::Typeface::Ptr loadSystemSerif (bool bold)
{
    juce::String name = "Georgia";
    int flags = bold ? juce::Font::bold : juce::Font::plain;
    return juce::Typeface::createSystemTypefaceFor (juce::Font (name, 12.0f, flags));
}

juce::Typeface::Ptr tryBundled (const char* data, int size,
                                std::function<juce::Typeface::Ptr()> systemFallback)
{
    if (data != nullptr && size > 0)
    {
        if (auto tf = juce::Typeface::createSystemTypefaceFor (
                data, static_cast<size_t> (size)))
            return tf;
    }
    return systemFallback();
}

} // namespace

juce::Typeface::Ptr OB8LookAndFeel::getMonoRegular()
{
    static juce::Typeface::Ptr tf =
       #if OB8_HAS_BUNDLED_FONT
        tryBundled (BinaryData::IBMPlexMonoRegular_ttf,
                    BinaryData::IBMPlexMonoRegular_ttfSize,
                    [] { return loadSystemMono (false, false); });
       #else
        loadSystemMono (false, false);
       #endif
    return tf;
}

juce::Typeface::Ptr OB8LookAndFeel::getMonoBold()
{
    static juce::Typeface::Ptr tf =
       #if OB8_HAS_BUNDLED_FONT
        tryBundled (BinaryData::IBMPlexMonoBold_ttf,
                    BinaryData::IBMPlexMonoBold_ttfSize,
                    [] { return loadSystemMono (true, false); });
       #else
        loadSystemMono (true, false);
       #endif
    return tf;
}

juce::Typeface::Ptr OB8LookAndFeel::getMonoItalic()
{
    static juce::Typeface::Ptr tf =
       #if OB8_HAS_BUNDLED_FONT
        tryBundled (BinaryData::IBMPlexMonoItalic_ttf,
                    BinaryData::IBMPlexMonoItalic_ttfSize,
                    [] { return loadSystemMono (false, true); });
       #else
        loadSystemMono (false, true);
       #endif
    return tf;
}

juce::Typeface::Ptr OB8LookAndFeel::getSerifSemiBold()
{
    static juce::Typeface::Ptr tf =
       #if OB8_HAS_BUNDLED_FONT
        tryBundled (BinaryData::FrauncesSemiBold_ttf,
                    BinaryData::FrauncesSemiBold_ttfSize,
                    [] { return loadSystemSerif (true); });
       #else
        loadSystemSerif (true);
       #endif
    return tf;
}

juce::Font OB8LookAndFeel::monoRegular (float h)
{
    if (auto tf = getMonoRegular()) return juce::Font (tf).withHeight (h);
    return juce::Font (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::plain);
}
juce::Font OB8LookAndFeel::monoBold (float h)
{
    if (auto tf = getMonoBold()) return juce::Font (tf).withHeight (h);
    return juce::Font (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::bold);
}
juce::Font OB8LookAndFeel::monoItalic (float h)
{
    if (auto tf = getMonoItalic()) return juce::Font (tf).withHeight (h);
    return juce::Font (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::italic);
}
juce::Font OB8LookAndFeel::serifSemiBold (float h)
{
    if (auto tf = getSerifSemiBold()) return juce::Font (tf).withHeight (h);
    return juce::Font ("Georgia", h, juce::Font::bold);
}

OB8LookAndFeel::OB8LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, paperBg());
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxTextColourId,         inkDim());
    setColour (juce::Slider::trackColourId,               ink());
    setColour (juce::Slider::backgroundColourId,          hairFine());
    setColour (juce::Slider::thumbColourId,               accent());
    setColour (juce::Label::textColourId,                 ink());
    setColour (juce::ComboBox::backgroundColourId,        paperBg());
    setColour (juce::ComboBox::textColourId,              ink());
    setColour (juce::ComboBox::arrowColourId,             inkDim());
    setColour (juce::ComboBox::outlineColourId,           hairline());
    setColour (juce::PopupMenu::backgroundColourId,       paperBg());
    setColour (juce::PopupMenu::textColourId,             ink());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent().withAlpha (0.15f));
    setColour (juce::PopupMenu::highlightedTextColourId,  ink());
    setColour (juce::ToggleButton::textColourId,          ink());
    setColour (juce::ToggleButton::tickColourId,          accent());
    setColour (juce::TextButton::buttonColourId,          paperBg());
    setColour (juce::TextButton::textColourOnId,          accent());
    setColour (juce::TextButton::textColourOffId,         ink());
    setColour (juce::TextEditor::backgroundColourId,      paperBg());
    setColour (juce::TextEditor::textColourId,            ink());
    setColour (juce::TextEditor::outlineColourId,         hairline());

    if (auto tf = getMonoRegular())
        setDefaultSansSerifTypeface (tf);
}

juce::Typeface::Ptr OB8LookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    juce::Typeface::Ptr tf;
    if      (f.getStyleFlags() & juce::Font::italic) tf = getMonoItalic();
    else if (f.getStyleFlags() & juce::Font::bold)   tf = getMonoBold();
    else                                             tf = getMonoRegular();
    if (tf) return tf;
    return LookAndFeel_V4::getTypefaceForFont (f);
}

/*
    HAIRLINE-VIII RefinedKnob (handoff §6.1).

    SVG viewBox 0..100, scaled to (w, h). Sweep is -135° .. +135° (270°
    range). Components:
        1. outer sweep arc (hairFine, 1 px)
        2. two end ticks (hairline, 1.1 px) reaching r=40 -> r=46
        3. centre tick at the top (50,6 -> 50,12)
        4. body circle r=30 (transparent fill, ink stroke 1.4 at alpha 0.88)
        5. indicator from (50,50) to (50,22), accent 2.2 px round-cap
        6. centre dot r=2.2 at (50,50), accent fill
*/
void OB8LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                       float pos, float startAng, float endAng,
                                       juce::Slider&)
{
    juce::ignoreUnused (startAng, endAng);

    const float side = static_cast<float> (juce::jmin (w, h));
    const float cx   = x + w * 0.5f;
    const float cy   = y + h * 0.5f;
    const float s    = side / 100.0f;     // svg -> px scale factor

    // -135° to +135° in canvas space (0 = pointing up)
    const float a0 = juce::degreesToRadians (-135.0f);
    const float a1 = juce::degreesToRadians ( 135.0f);
    const float a  = a0 + pos * (a1 - a0);

    auto svgPos = [&] (float xv, float yv) -> juce::Point<float>
    {
        return { cx + (xv - 50.0f) * s, cy + (yv - 50.0f) * s };
    };

    // 1. Outer sweep arc -- approximate with juce::Path::addCentredArc
    {
        juce::Path arc;
        arc.addCentredArc (cx, cy,
                           40.0f * s, 40.0f * s,
                           0.0f,
                           a0, a1,
                           true);
        g.setColour (hairFine());
        g.strokePath (arc, juce::PathStrokeType (1.0f));
    }

    // 2. End ticks at r=40 to r=46 in svg space
    auto endTickAt = [&] (float angle)
    {
        const float dx = std::sin (angle);
        const float dy = -std::cos (angle);
        const juce::Point<float> p1 { cx + dx * 40.0f * s, cy + dy * 40.0f * s };
        const juce::Point<float> p2 { cx + dx * 46.0f * s, cy + dy * 46.0f * s };
        g.setColour (hairline());
        g.drawLine ({ p1, p2 }, 1.1f);
    };
    endTickAt (a0);
    endTickAt (a1);

    // 3. Centre tick at the top (svg 50,6 -> 50,12)
    {
        const auto t1 = svgPos (50.0f, 6.0f);
        const auto t2 = svgPos (50.0f, 12.0f);
        g.setColour (hairline());
        g.drawLine ({ t1, t2 }, 1.1f);
    }

    // 4. Body circle r=30 (transparent fill)
    {
        const float r = 30.0f * s;
        g.setColour (ink().withAlpha (0.88f));
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.4f);
    }

    // 5. Indicator line (50,50) -> (50,22) rotated to a
    {
        const float r0 = 0.0f;
        const float r1 = 28.0f * s;
        const float dx = std::sin (a);
        const float dy = -std::cos (a);
        const juce::Point<float> p0 { cx, cy };
        const juce::Point<float> p1 { cx + dx * r1, cy + dy * r1 };
        juce::ignoreUnused (r0);
        juce::Path line;
        line.startNewSubPath (p0);
        line.lineTo (p1);
        g.setColour (accent());
        g.strokePath (line, juce::PathStrokeType (2.2f,
                            juce::PathStrokeType::curved,
                            juce::PathStrokeType::rounded));
    }

    // 6. Centre dot r=2.2 in svg units
    {
        const float r = 2.2f * s;
        g.setColour (accent());
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    }
}

/*
    HAIRLINE-VIII RefinedFader (handoff §6.2).

    SVG width 26 x height 48. Scaled to (w, h).
        1. five paired tick rows (left + right) at 0, .25, .5, .75, 1
        2. central 4 px slot, hairline-stroked
        3. horizontal half-line at slot mid (hairFine)
        4. accent thumb 20 x 4 px, with subtle bottom shadow line
*/
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
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, minPos, maxPos, style, slider);
        return;
    }

    const float sx = static_cast<float> (w) / 26.0f;
    const float sy = static_cast<float> (h) / 48.0f;
    const float cx = x + w * 0.5f;
    auto sv = [&] (float xv, float yv) -> juce::Point<float>
    {
        return { x + xv * sx, y + yv * sy };
    };

    // 1. Paired tick rows
    const float yTickPositions[5] = { 4.0f, 14.0f, 24.0f, 34.0f, 44.0f };
    const bool  major[5] = { true, false, true, false, true };
    for (int i = 0; i < 5; ++i)
    {
        const float yv = yTickPositions[i];
        const float left1  = major[i] ? 2.0f  : 4.5f;
        const float left2  = major[i] ? 6.0f  : 6.0f;
        const float right1 = major[i] ? 26.0f - 2.0f  : 26.0f - 4.5f;
        const float right2 = major[i] ? 26.0f - 6.0f  : 26.0f - 6.0f;
        g.setColour (hairline());
        g.drawLine ({ sv (left1,  yv), sv (left2,  yv) }, 1.0f);
        g.drawLine ({ sv (right1, yv), sv (right2, yv) }, 1.0f);
    }

    // 2. Central slot 4 px wide, height 40 (y=4..44)
    {
        const auto p1 = sv (11.0f, 4.0f);
        const auto p2 = sv (15.0f, 44.0f);
        juce::Rectangle<float> slot (p1.x, p1.y, p2.x - p1.x, p2.y - p1.y);
        g.setColour (ink().withAlpha (0.88f));
        g.drawRect (slot, 1.0f);
    }

    // 3. Half-line across at y = 24 (mid)
    {
        g.setColour (hairFine());
        g.drawLine ({ sv (8.0f, 24.0f), sv (18.0f, 24.0f) }, 1.0f);
    }

    // 4. Thumb: positioned by sliderPos (sliderPos is in pixel coords already)
    {
        // Use sliderPos directly -- JUCE passes the y in component coords
        const float thumbH = 4.0f * sy;
        const float thumbW = 20.0f * sx;
        const float ty = juce::jlimit (y + thumbH * 0.5f,
                                       y + h - thumbH * 0.5f,
                                       sliderPos);
        juce::Rectangle<float> thumb (cx - thumbW * 0.5f,
                                      ty - thumbH * 0.5f,
                                      thumbW, thumbH);
        g.setColour (accent());
        g.fillRect (thumb);
        // Subtle shadow line under the thumb
        g.setColour (juce::Colour::fromRGBA (0, 0, 0, 90));
        g.drawLine ({ thumb.getX(), thumb.getBottom(),
                      thumb.getRight(), thumb.getBottom() }, 1.0f);
    }
}

void OB8LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                       bool /*highlighted*/, bool /*down*/)
{
    auto bounds = b.getLocalBounds().toFloat();
    const float size = juce::jmin (bounds.getHeight(), 14.0f);
    auto sq = juce::Rectangle<float> (bounds.getX(),
                                      bounds.getCentreY() - size * 0.5f,
                                      size, size).reduced (0.5f);

    g.setColour (paperBg());
    g.fillRect (sq);
    g.setColour (hairline());
    g.drawRect (sq, 1.0f);

    if (b.getToggleState())
    {
        g.setColour (accent());
        g.fillRect (sq.reduced (2.5f));
    }

    bounds.removeFromLeft (size + 5.0f);
    g.setColour (ink());
    g.setFont (monoRegular (9.0f).withExtraKerningFactor (0.14f));
    g.drawText (b.getButtonText().toUpperCase(), bounds.toNearestInt(),
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
    g.setColour (paperBg());
    g.fillRect (bounds);
    g.setColour (hairline());
    g.drawRect (bounds, 1.0f);

    g.setColour (inkDim());
    g.setFont (monoRegular (8.0f));
    g.drawText (juce::String::charToString (0x25BE), // ▾
                static_cast<int> (bounds.getRight() - 12),
                static_cast<int> (bounds.getY()),
                12, height, juce::Justification::centred, false);

    juce::ignoreUnused (cb);
}

void OB8LookAndFeel::positionComboBoxText (juce::ComboBox& cb, juce::Label& l)
{
    l.setBounds (4, 0, cb.getWidth() - 14, cb.getHeight());
    l.setFont   (monoRegular (8.5f).withExtraKerningFactor (0.04f));
    l.setColour (juce::Label::textColourId, ink());
    l.setMinimumHorizontalScale (0.6f);  // allow shrinking before truncating
}

void OB8LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                           const juce::Colour&,
                                           bool /*highlighted*/, bool down)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? accent().withAlpha (0.12f) : paperBg());
    g.fillRect (bounds);
    g.setColour (hairline());
    g.drawRect (bounds, 1.0f);
}

juce::Font OB8LookAndFeel::getLabelFont      (juce::Label& l)        { return monoRegular (l.getFont().getHeight()); }
juce::Font OB8LookAndFeel::getComboBoxFont   (juce::ComboBox&)       { return monoRegular (9.5f); }
juce::Font OB8LookAndFeel::getTextButtonFont (juce::TextButton&, int){ return monoRegular (9.5f).withExtraKerningFactor (0.18f); }
juce::Font OB8LookAndFeel::getPopupMenuFont()                        { return monoRegular (10.5f); }

} // namespace ob8
