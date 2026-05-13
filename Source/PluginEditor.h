#pragma once

#include "PluginProcessor.h"
#include "GUI/OB8LookAndFeel.h"
#include "GUI/OB8Knob.h"

#include <juce_audio_utils/juce_audio_utils.h>   // MidiKeyboardComponent

namespace ob8 {

class OB8Editor : public juce::AudioProcessorEditor
{
public:
    explicit OB8Editor (OB8Processor&);
    ~OB8Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

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
    std::unique_ptr<OB8Choice> vco1Oct, vco1Wave;
    std::unique_ptr<OB8Knob>   vco1Pw;

    // VCO 2
    std::unique_ptr<OB8Choice> vco2Oct, vco2Wave;
    std::unique_ptr<OB8Knob>   vco2Pw, vco2Detune;

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
    std::unique_ptr<OB8Knob>   lfoRate, lfoToVco1, lfoToVco2, lfoToPwm;
    std::unique_ptr<OB8Choice> lfoShape;

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

    // On-screen MIDI keyboard. Accepts mouse input and (when focused) PC
    // keyboard input mapped to chromatic notes (A,S,D,F,... for naturals,
    // W,E,T,Y,U for sharps, Z/X to change octave). Constructed in the .cpp
    // initialiser list with the processor's MidiKeyboardState.
    juce::MidiKeyboardComponent keyboard;

    std::vector<Section> sections;

    void layoutSection (Section& s, juce::Rectangle<int> bounds, int cols);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OB8Editor)
};

} // namespace ob8
