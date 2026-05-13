#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ob8 {

/*
    Visual scheme mirroring the OB-8 front panel:
      - Deep midnight-blue background
      - Off-white legend / engraving
      - Brass/yellow accent for active controls and pointer indicators
      - Rotary knobs with a flat-top metal cap, single black pointer line.
*/
class OB8LookAndFeel : public juce::LookAndFeel_V4
{
public:
    OB8LookAndFeel();

    static juce::Colour panelBlue()    { return juce::Colour::fromRGB (12, 22, 48); }
    static juce::Colour panelDark()    { return juce::Colour::fromRGB (8, 14, 32); }
    static juce::Colour panelAccent()  { return juce::Colour::fromRGB (215, 175, 60); }
    static juce::Colour panelCream()   { return juce::Colour::fromRGB (224, 215, 195); }
    static juce::Colour panelOrange()  { return juce::Colour::fromRGB (210, 120, 40); }

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawAsHighlighted, bool shouldDrawAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    juce::Font getLabelFont   (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
};

} // namespace ob8
