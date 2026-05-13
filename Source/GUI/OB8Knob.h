#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace ob8 {

/*
    A labelled rotary knob bound to an APVTS parameter.
    Compact layout, OB-8 style: top label, knob, value below on hover.
*/
class OB8Knob : public juce::Component
{
public:
    OB8Knob (juce::AudioProcessorValueTreeState& apvts,
             const juce::String& paramID,
             const juce::String& labelText);

    void resized() override;
    void paint (juce::Graphics&) override;

    juce::Slider slider;

private:
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

/*  Toggle bound to a bool parameter. */
class OB8Toggle : public juce::Component
{
public:
    OB8Toggle (juce::AudioProcessorValueTreeState& apvts,
               const juce::String& paramID,
               const juce::String& labelText);

    void resized() override;

    juce::ToggleButton button;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

/*  Choice bound to a choice parameter. */
class OB8Choice : public juce::Component
{
public:
    OB8Choice (juce::AudioProcessorValueTreeState& apvts,
               const juce::String& paramID,
               const juce::String& labelText);

    void resized() override;
    void paint (juce::Graphics&) override;

    juce::ComboBox combo;
private:
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

} // namespace ob8
