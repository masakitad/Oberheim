#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace ob8 {

/*
    HAIRLINE-VIII WaveformPreview (handoff §6.5).

    Replaces a juce::AudioParameterChoice picker with a visual SVG-style
    waveform thumbnail and two cycle arrows. The owning component supplies
    a mapping from the parameter's choice index to one of the five canonical
    waveform shapes; clicks on the left/right arrows decrement/increment
    the underlying choice parameter.

    Layout (same shape as OB8Knob/OB8Choice so they can share section grids):
        row 1  -- label  (~14 px)
        row 2+ -- arrow [  waveform  ] arrow   (fills remainder)
*/
class WaveformPreview : public juce::Component,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    enum class Kind { Sine, Saw, Square, Tri, Noise };

    WaveformPreview (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& paramID,
                     const juce::String& labelText,
                     std::vector<Kind> choiceToKind);

    ~WaveformPreview() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void parameterChanged (const juce::String& id, float newValue) override;

    int currentIndex() const noexcept;
    Kind currentKind() const noexcept;
    void cycle (int delta);

    juce::Label label;
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId;
    std::vector<Kind> kinds;

    juce::Rectangle<int> leftArrowBox, rightArrowBox, previewBox;
};

} // namespace ob8
