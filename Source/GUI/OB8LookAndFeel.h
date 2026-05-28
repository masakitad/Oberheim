#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ob8 {

/*
    "HAIRLINE" look-and-feel: technical-drawing-style cream paper background
    with red accents and 1-px hairline framing.

    Typography: bundled IBM Plex Mono (OFL 1.1). Three weights -- Regular,
    Bold, Italic -- are baked into the binary at build time so the
    appearance is identical on every install.

    Palette:
      - panelCream  : warm paper colour
      - panelDark   : near-black text / hairline strokes
      - panelAccent : vermillion red (sliders, numbers, active indicators)
      - panelMute   : muted grey for ticks / field captions
*/
class OB8LookAndFeel : public juce::LookAndFeel_V4
{
public:
    OB8LookAndFeel();

    static juce::Colour panelCream()  { return juce::Colour::fromRGB (236, 226, 200); }
    static juce::Colour panelBlue()   { return panelCream(); }     // legacy alias
    static juce::Colour panelDark()   { return juce::Colour::fromRGB (22,  20,  18);  }
    static juce::Colour panelAccent() { return juce::Colour::fromRGB (194, 57,  43);  }
    static juce::Colour panelOrange() { return panelAccent(); }    // legacy alias
    static juce::Colour panelMute()   { return juce::Colour::fromRGB (140, 132, 118); }

    // Typeface accessors (used by the editor and embedded controls)
    static juce::Typeface::Ptr getMonoRegular();
    static juce::Typeface::Ptr getMonoBold();
    static juce::Typeface::Ptr getMonoItalic();

    static juce::Font monoRegular (float heightPx);
    static juce::Font monoBold    (float heightPx);
    static juce::Font monoItalic  (float heightPx);

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
    juce::Font getTextEditorFont (juce::TextEditor&);

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
};

} // namespace ob8
