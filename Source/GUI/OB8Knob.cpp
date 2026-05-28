#include "OB8Knob.h"
#include "OB8LookAndFeel.h"

namespace ob8 {

OB8Knob::OB8Knob (juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramID,
                  const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 14);
    slider.setVelocityBasedMode (false);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (OB8LookAndFeel::monoBold (10.0f).withExtraKerningFactor (0.05f));
    label.setColour (juce::Label::textColourId, OB8LookAndFeel::panelDark());
    addAndMakeVisible (label);

    attachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, paramID, slider));
}

void OB8Knob::resized()
{
    auto bounds = getLocalBounds();
    label .setBounds (bounds.removeFromTop (14));
    slider.setBounds (bounds);
}

void OB8Knob::paint (juce::Graphics&) {}

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
    label.setFont (OB8LookAndFeel::monoBold (10.0f).withExtraKerningFactor (0.05f));
    label.setColour (juce::Label::textColourId, OB8LookAndFeel::panelDark());
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
