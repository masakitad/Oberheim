#include "OB8Knob.h"
#include "OB8LookAndFeel.h"

namespace ob8 {

OB8Knob::OB8Knob (juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramID,
                  const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 12);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f,
                                true);
    slider.setVelocityBasedMode (false);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (OB8LookAndFeel::monoRegular (9.0f).withExtraKerningFactor (0.18f));
    label.setColour (juce::Label::textColourId, OB8LookAndFeel::ink());
    addAndMakeVisible (label);

    attachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, paramID, slider));
}

void OB8Knob::resized()
{
    auto bounds = getLocalBounds();
    label.setBounds (bounds.removeFromTop (14));

    // Reserve a row at the bottom for ModChips. The chips sit BELOW the
    // slider's TextBoxBelow so the value readout is never covered.
    constexpr int kChipRowH = ModChip::kHeight;
    constexpr int kChipRowGap = 2;
    if (! chips.empty())
    {
        auto chipRow = bounds.removeFromBottom (kChipRowH);
        bounds.removeFromBottom (kChipRowGap);

        // Allocate width per chip using its preferred width, but shrink
        // proportionally if the row is too tight. Hard floor at 14 px so
        // even a 1-char chip remains readable.
        const int n = static_cast<int> (chips.size());
        int totalPreferred = 0;
        for (auto& c : chips) totalPreferred += c->preferredWidth();
        const int interGap = (n > 1) ? 2 : 0;
        const int totalGap = interGap * (n - 1);
        const int avail = std::max (0, chipRow.getWidth() - totalGap);

        const float scale = (totalPreferred > avail && totalPreferred > 0)
            ? static_cast<float> (avail) / static_cast<float> (totalPreferred)
            : 1.0f;

        int x = chipRow.getX();
        for (int i = 0; i < n; ++i)
        {
            int w = static_cast<int> (std::round (chips[i]->preferredWidth() * scale));
            w = std::max (14, w);
            const int actualW = std::min (w, chipRow.getRight() - x);
            chips[i]->setBounds (x, chipRow.getY(), actualW, chipRow.getHeight());
            x += actualW + interGap;
        }
    }
    slider.setBounds (bounds);
}

void OB8Knob::paint (juce::Graphics&) {}

void OB8Knob::setChipLabels (const std::vector<juce::String>& labels)
{
    chips.clear();
    for (const auto& t : labels)
    {
        auto chip = std::make_unique<ModChip> (t);
        addAndMakeVisible (*chip);
        chips.push_back (std::move (chip));
    }
    resized();
}

// ---------- Toggle -----------------------------------------------------

OB8Toggle::OB8Toggle (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& paramID,
                     const juce::String& labelText)
{
    button.setButtonText (labelText);
    addAndMakeVisible (button);
    attachment.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (apvts, paramID, button));
}

void OB8Toggle::resized() { button.setBounds (getLocalBounds()); }

// ---------- Choice -----------------------------------------------------

OB8Choice::OB8Choice (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& paramID,
                     const juce::String& labelText)
{
    addAndMakeVisible (combo);
    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (OB8LookAndFeel::monoRegular (9.0f).withExtraKerningFactor (0.18f));
    label.setColour (juce::Label::textColourId, OB8LookAndFeel::ink());
    addAndMakeVisible (label);

    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramID)))
    {
        int id = 1;
        for (const auto& choice : p->choices)
            combo.addItem (choice, id++);
    }
    attachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (apvts, paramID, combo));
}

void OB8Choice::resized()
{
    auto bounds = getLocalBounds();
    label.setBounds (bounds.removeFromTop (14));
    bounds.removeFromBottom (bounds.getHeight() / 2);  // sit combo near the top
    combo.setBounds (bounds.reduced (4, 2));
}

void OB8Choice::paint (juce::Graphics&) {}

} // namespace ob8
