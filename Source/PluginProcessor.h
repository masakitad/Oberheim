#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "MacroBridge.h"
#include "DSP/Voice.h"
#include "DSP/LFO.h"
#include "DSP/Oversampler.h"
#include "DSP/Delay.h"
#include "DSP/Reverb.h"
#include "DSP/Granular.h"

namespace ob8 {

class OB8Processor : public juce::AudioProcessor
{
public:
    OB8Processor();
    ~OB8Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "OB-8 Native"; }
    bool acceptsMidi()  const override { return true;  }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // Shared keyboard state used by the on-screen virtual keyboard and any
    // PC-keyboard input.
    juce::MidiKeyboardState keyboardState;

    // Number of polyphonic voices. The OB-8 has 8.
    static constexpr int kNumVoices = 8;

    // Patch / bank management. Banks of 10 patches, 12 banks = 120 patches.
    static constexpr int kPatchesPerBank = 10;
    static constexpr int kNumBanks       = 12;

    bool loadBankFromFile (const juce::File&);
    bool saveBankToFile   (const juce::File&) const;
    bool loadCurrentPatchFromXml (const juce::XmlElement&);
    void saveCurrentPatchToXml   (juce::XmlElement&, const juce::String& patchName);

    // Currently selected patch slot (0..119); just metadata, no audio impact
    int   currentBank    = 0;
    int   currentProgram = 0;
    juce::String currentPatchName { "Init Patch" };
    juce::ValueTree bankState { "BANK" };

private:
    void handleMidiEvent (const juce::MidiMessage&);
    void noteOn  (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();
    void resetLfoIfKeySync (bool wasIdle);
    bool isAnyVoiceActive() const noexcept;

    dsp::Voice::PerVoiceParams snapshotParams() const;

    // Voice management
    std::array<dsp::Voice, kNumVoices> voices;
    int noteOnCounter = 0;

    // Global modulator
    dsp::LFO lfo;

    // Live MIDI state
    double currentBendSemis = 0.0;
    double currentModWheel  = 0.0;   // 0..1
    double currentAfterT    = 0.0;   // 0..1 (channel aftertouch)
    bool   sustainPedalDown = false;
    juce::SortedSet<int> sustainedNotes;
    juce::Array<int>     polyAfterTouch; // 128 entries indexed by note

    // Oversampling
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int oversampleFactor = 2; // 4x

    // Post effects (run at host sample rate, after downsampling)
    dsp::StereoDelay   delay;
    dsp::FDNReverb     reverb;
    dsp::GranularDelay granular;

    // Simple-view macro-to-parameter bridge. Constructed after apvts.
    MacroBridge macros { apvts };

    // Scratch buffers
    juce::AudioBuffer<float> mixBuffer;
    juce::AudioBuffer<float> oversampleBuffer;
    std::vector<float>       lfoBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OB8Processor)
};

} // namespace ob8
