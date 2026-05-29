#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "ModChip.h"

#include <memory>
#include <vector>

namespace ob8 {

/*
    A labelled rotary knob bound to an APVTS parameter.

    Layout (per handoff §6.3 KnobUnit):
        - label (top, 14 px)
        - rotary slider + value text below (fills remaining height)
        - optional ModChip row at the bottom (12 px), positioned BELOW the
          value text box so the chips never overlap the numeric readout.

    Use setChipLabels(...) once after construction to attach modulation
    indicator chips (e.g. {"E2", "L1", "V1"} for cutoff). Chips shrink
    horizontally if they don't fit in the available width so they never
    spill outside the knob's bounds.
*/
class OB8Knob : public juce::Component
{
public:
    OB8Knob (juce::AudioProcessorValueTreeState& apvts,
             const juce::String& paramID,
             const juce::String& labelText);

    void resized() override;
    void paint (juce::Graphics&) override;

    /*  Attach modulation-source chips to this knob. Pass the short
        identifier strings (E1/E2/L1/L2/V1/V2/MW/AT/...). Chips render in
        a horizontal row directly below the slider's value text. */
    void setChipLabels (const std::vector<juce::String>& labels);

    juce::Slider slider;

private:
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    std::vector<std::unique_ptr<ModChip>> chips;
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
