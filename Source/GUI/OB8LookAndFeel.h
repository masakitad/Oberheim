#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ob8 {

/*
    "HAIRLINE" look-and-feel: technical-drawing-style cream paper background
    with red accents and 1-px hairline framing. Replaces the earlier
    midnight-blue + brass palette.

    Palette:
      - panelCream  : background paper colour
      - panelDark   : near-black text / hairline strokes
      - panelAccent : red accent (sliders, ticks, active indicators)
      - panelMute   : muted grey for ticks / disabled
*/
class OB8LookAndFeel : public juce::LookAndFeel_V4
{
public:
    OB8LookAndFeel();

    static juce::Colour panelCream()   { return juce::Colour::fromRGB (236, 226, 200); }
    static juce::Colour panelBlue()    { return panelCream(); }            // legacy aliases
    static juce::Colour panelDark()    { return juce::Colour::fromRGB (22,  20,  18);  }
    static juce::Colour panelAccent()  { return juce::Colour::fromRGB (194, 57,  43);  }
    static juce::Colour panelOrange()  { return panelAccent(); }
    static juce::Colour panelMute()    { return juce::Colour::fromRGB (140, 132, 118); }

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

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont    (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

} // namespace ob8
