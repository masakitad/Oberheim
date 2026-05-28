#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "OB8LookAndFeel.h"

namespace ob8 {

/*
    ModChip (handoff §6.4).

    A small chip with an accent-red outline and label, typically rendered
    next to a knob's value to indicate that the knob's parameter is being
    modulated by another source (E1/E2 envelopes, L1/L2 LFOs, V1/V2
    velocity, P1/P2 page-2 routings, MW mod wheel, AT aftertouch).

    Spec:
      height       12 px
      padding      0 4 px
      border       1 px solid accent (#a23a1a), corner-radius 1 px
      font         IBM Plex Mono 7.5 px / 500, letter-spacing 0.05em
      colour       accent text on transparent background

    The component sizes itself off the string width; place several of them
    horizontally in a row using a juce::FlexBox or hand layout.
*/
class ModChip : public juce::Component
{
public:
    explicit ModChip (const juce::String& labelText) : text (labelText)
    {
        setInterceptsMouseClicks (false, false);
    }

    void setLabelText (const juce::String& t) { text = t; repaint(); }

    void paint (juce::Graphics& g) override
    {
        using LF = OB8LookAndFeel;
        const auto b = getLocalBounds().toFloat().reduced (0.5f);

        // 1 px accent outline with a 1 px corner radius
        g.setColour (LF::accent());
        g.drawRoundedRectangle (b, 1.0f, 1.0f);

        // Label
        g.setFont (LF::monoBold (7.5f).withExtraKerningFactor (0.05f));
        g.drawText (text.toUpperCase(), getLocalBounds(),
                    juce::Justification::centred, false);
    }

    /*  Compute the natural width for the chip given its label text. */
    int preferredWidth() const
    {
        auto f = OB8LookAndFeel::monoBold (7.5f).withExtraKerningFactor (0.05f);
        return f.getStringWidth (text.toUpperCase()) + 8;  // 4 px padding both sides
    }

    static constexpr int kHeight = 12;

private:
    juce::String text;
};

} // namespace ob8
