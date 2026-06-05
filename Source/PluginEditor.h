#pragma once

#include "PluginProcessor.h"
#include "GUI/OB8LookAndFeel.h"
#include "GUI/OB8Knob.h"
#include "GUI/ModChip.h"
#include "GUI/WaveformPreview.h"

#include <juce_audio_utils/juce_audio_utils.h>   // MidiKeyboardComponent

namespace ob8 {

/*
    JUCE's MidiKeyboardComponent only handles PC keyboard input while it
    itself has focus -- so clicking a knob or combo box makes the on-screen
    keyboard stop responding. We subclass it just to bring its protected
    keyPressed / keyStateChanged overrides up to public access so the
    editor can forward unhandled key events from any focused descendant.
*/
class FocusableKeyboard : public juce::MidiKeyboardComponent
{
public:
    using juce::MidiKeyboardComponent::MidiKeyboardComponent;
    using juce::MidiKeyboardComponent::keyPressed;
    using juce::MidiKeyboardComponent::keyStateChanged;

    /*  Per HAIRLINE-VIII §6.12: C white keys carry a small octave label
        (C1, C2, ...) at their bottom edge. Other white keys render unchanged. */
    void drawWhiteNote (int midiNoteNumber, juce::Graphics& g,
                        juce::Rectangle<float> area,
                        bool isDown, bool isOver,
                        juce::Colour lineColour, juce::Colour textColour) override;
};

class OB8Editor : public juce::AudioProcessorEditor,
                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit OB8Editor (OB8Processor&);
    ~OB8Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;

private:
    void parameterChanged (const juce::String& paramID, float newValue) override;

public:

private:
    struct Section
    {
        juce::String title;
        juce::Rectangle<int> bounds;
        std::vector<juce::Component*> children;
    };

    OB8Processor& processorRef;
    OB8LookAndFeel laf;

    // ---- Page 1 controls ----------------------------------------------------
    // VCO 1
    std::unique_ptr<OB8Choice>       vco1Oct;
    std::unique_ptr<WaveformPreview> vco1Wave;
    std::unique_ptr<OB8Knob>         vco1Pw;

    // VCO 2
    std::unique_ptr<OB8Choice>       vco2Oct;
    std::unique_ptr<WaveformPreview> vco2Wave;
    std::unique_ptr<OB8Knob>         vco2Pw, vco2Detune;

    // X-MOD / sync
    std::unique_ptr<OB8Knob>   xMod;
    std::unique_ptr<OB8Toggle> sync;

    // Mixer
    std::unique_ptr<OB8Knob> mixVco1, mixVco2, mixNoise;

    // Filter
    std::unique_ptr<OB8Knob>   cutoff, resonance, envAmount, lfoToVcf, kbdTrack;
    std::unique_ptr<OB8Choice> slope;

    // Filter env
    std::unique_ptr<OB8Knob> filtA, filtD, filtS, filtR;

    // Amp env
    std::unique_ptr<OB8Knob> ampA, ampD, ampS, ampR;

    // LFO
    std::unique_ptr<OB8Knob>         lfoRate, lfoToVco1, lfoToVco2, lfoToPwm;
    std::unique_ptr<WaveformPreview> lfoShape;

    // Velocity
    std::unique_ptr<OB8Knob> velToVca, velToVcf;

    // Global
    std::unique_ptr<OB8Choice> polyMode;
    std::unique_ptr<OB8Knob>   unisonDetune, driftDepth, masterGain, masterTune, bendRange;

    // ---- Page 2 controls ----------------------------------------------------
    std::unique_ptr<OB8Knob>   envToVco1, envToVco2, envToPwm;
    std::unique_ptr<OB8Knob>   atToVcf, atToLfo, atToVca;
    std::unique_ptr<OB8Knob>   mwToVcf, mwToLfo, mwToVibrato;
    std::unique_ptr<OB8Toggle> lfoKeySync;

    // Split / Double
    std::unique_ptr<OB8Knob>   splitPoint, splitDetune, doubleDetune;
    std::unique_ptr<OB8Choice> splitOctave;

    // Performance
    std::unique_ptr<OB8Knob>   glide;
    std::unique_ptr<OB8Toggle> hold;
    std::unique_ptr<OB8Toggle> ampReleaseInf;

    // Delay
    std::unique_ptr<OB8Knob> dlyTimeL, dlyTimeR, dlyFb, dlyCross, dlyDamp, dlyMix;

    // Reverb
    std::unique_ptr<OB8Knob> rvbSize, rvbDecay, rvbDamp, rvbPre, rvbMod, rvbWidth, rvbMix;

    // Granular delay
    std::unique_ptr<OB8Knob> grSize, grDensity, grScatter, grPitch, grSpread, grFeedback, grMix;

    // Keyboard octave shift UI
    juce::TextButton  octDownBtn { "<" }, octUpBtn { ">" };
    juce::Label       octaveLabel { {}, "C3" };
    int               pcKeyboardBaseOctave { 4 };  // makes 'A' = MIDI 48 = C3
    void updateOctaveLabel();

    // FILTER mod chips now live inside the cutoff / resonance OB8Knobs
    // themselves (see OB8Knob::setChipLabels) so they can never overlap
    // the knob's value text.

    // Cached paper-texture overlay -- built once and blitted by paint().
    juce::Image paperTexture;
    void buildPaperTexture();

    // SIMPLE-view macros + the view-mode selector itself
    std::unique_ptr<OB8Knob>   macroTone, macroMotion, macroSpace;
    juce::ComboBox             viewModeCombo;
    juce::Label                viewModeLabel { {}, "VIEW" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
                               viewModeAttach;
    void applyViewMode();

    // ---- Patch management ---------------------------------------------------
    juce::ComboBox bankCombo, programCombo;
    juce::Label    bankLabel { {}, "BANK" }, programLabel { {}, "PROGRAM" }, patchNameLabel { {}, "NAME" };
    juce::TextEditor patchNameEdit;
    juce::TextButton storeBtn { "STORE" }, recallBtn { "RECALL" },
                    saveBankBtn { "SAVE BANK..." }, loadBankBtn { "LOAD BANK..." };

    void populateBankCombo();
    void populateProgramCombo();
    void storeCurrentPatch();
    void recallSelectedPatch();
    void chooseSaveBank();
    void chooseLoadBank();

    std::unique_ptr<juce::FileChooser> fileChooser;

    // On-screen MIDI keyboard. Accepts mouse input and (via key forwarding
    // from the editor) PC keyboard input mapped to chromatic notes
    // (A,S,D,F,... for naturals, W,E,T,Y,U for sharps, Z/X to change
    // octave). Constructed in the .cpp initialiser list with the
    // processor's MidiKeyboardState.
    FocusableKeyboard keyboard;

    // Dimension line drawn below the keyboard (HAIRLINE-VIII §6.12). Set in
    // resized(), rendered in paint().
    juce::Rectangle<int> keyboardDimBounds;

    std::vector<Section> sections;

    void layoutSection (Section& s, juce::Rectangle<int> bounds, int cols);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OB8Editor)
};

} // namespace ob8
