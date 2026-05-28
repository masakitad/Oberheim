#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ob8 {

/*
    HAIRLINE-VIII look-and-feel.

    Drafting ink on aged paper -- monospace UI text, serif display title,
    1 px hairline strokes and a single vermillion-red annotation accent.
    Faithful to the design handoff bundled in design_handoff_hairline_viii/.

    Tokens (from §4 Design Tokens):
      paperBg   #efe6cc   warm cream paper
      ink       #1a2538   deep navy-brown for text and lines
      inkDim    rgba(ink, 0.58)
      inkFaint  rgba(ink, 0.20)
      hairline  rgba(ink, 0.42)
      hairFine  rgba(ink, 0.20)
      accent    #a23a1a   red annotation pen

    Typography:
      Display = Fraunces (serif)        -- the "HAIRLINE-VIII" wordmark
      UI      = IBM Plex Mono (mono)    -- everything else
*/
class OB8LookAndFeel : public juce::LookAndFeel_V4
{
public:
    OB8LookAndFeel();

    // ---- Design tokens ----------------------------------------------------
    static juce::Colour paperBg()  { return juce::Colour::fromRGB (239, 230, 204); }
    static juce::Colour ink()      { return juce::Colour::fromRGB (26,  37,  56);  }
    static juce::Colour inkDim()   { return ink().withAlpha (0.58f); }
    static juce::Colour inkFaint() { return ink().withAlpha (0.20f); }
    static juce::Colour hairline() { return ink().withAlpha (0.42f); }
    static juce::Colour hairFine() { return ink().withAlpha (0.20f); }
    static juce::Colour accent()   { return juce::Colour::fromRGB (162, 58, 26); }

    // Legacy aliases so older call sites keep compiling
    static juce::Colour panelCream()  { return paperBg(); }
    static juce::Colour panelBlue()   { return paperBg(); }
    static juce::Colour panelDark()   { return ink(); }
    static juce::Colour panelMute()   { return inkDim(); }
    static juce::Colour panelAccent() { return accent(); }
    static juce::Colour panelOrange() { return accent(); }

    // ---- Typefaces --------------------------------------------------------
    static juce::Typeface::Ptr getMonoRegular();
    static juce::Typeface::Ptr getMonoBold();
    static juce::Typeface::Ptr getMonoItalic();
    static juce::Typeface::Ptr getSerifSemiBold();

    static juce::Font monoRegular  (float heightPx);
    static juce::Font monoBold     (float heightPx);
    static juce::Font monoItalic   (float heightPx);
    static juce::Font serifSemiBold (float heightPx);

    // ---- Component drawing ------------------------------------------------
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawAsHighlighted, bool shouldDrawAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont      (juce::Label&) override;
    juce::Font getComboBoxFont   (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getPopupMenuFont() override;

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
};

} // namespace ob8
