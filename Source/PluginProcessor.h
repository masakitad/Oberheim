#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "DSP/Voice.h"
#include "DSP/LFO.h"
#include "DSP/Oversampler.h"

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

    // Number of polyphonic voices. The OB-8 has 8.
    static constexpr int kNumVoices = 8;

private:
    void handleMidiEvent (const juce::MidiMessage&);
    void noteOn  (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();

    dsp::Voice::PerVoiceParams snapshotParams() const;

    // Voice management
    std::array<dsp::Voice, kNumVoices> voices;
    int noteOnCounter = 0;

    // Global modulator
    dsp::LFO lfo;

    // Pitch bend
    double currentBendSemis = 0.0;

    // Oversampling
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int oversampleFactor = 2; // 4x

    // Scratch buffers
    juce::AudioBuffer<float> mixBuffer;
    juce::AudioBuffer<float> oversampleBuffer;
    std::vector<float>       lfoBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OB8Processor)
};

} // namespace ob8
