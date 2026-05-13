#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <climits>

namespace ob8 {

OB8Processor::OB8Processor()
    : juce::AudioProcessor (BusesProperties()
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

void OB8Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double internalSr = sampleRate * (1 << oversampleFactor);

    // Oversampling
    oversampler.reset (new juce::dsp::Oversampling<float> (
        2,                                   // stereo
        oversampleFactor,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true,
        false));
    oversampler->initProcessing (static_cast<size_t> (samplesPerBlock));
    oversampler->reset();
    setLatencySamples (juce::roundToInt (oversampler->getLatencyInSamples()));

    // Per-voice prepare
    uint32_t seed = 0xA5A5A5A5u;
    for (auto& v : voices)
    {
        v.prepare (internalSr, seed);
        v.reset();
        seed = seed * 1664525u + 1013904223u;
    }

    lfo.prepare (sampleRate);    // LFO runs at host rate (modulation only)

    const int osBlock = samplesPerBlock * (1 << oversampleFactor);
    mixBuffer.setSize        (1, osBlock, false, true, true);
    oversampleBuffer.setSize (2, osBlock, false, true, true);
    lfoBuffer.assign (static_cast<size_t> (osBlock), 0.0f);

    currentBendSemis = 0.0;
}

void OB8Processor::releaseResources()
{
    if (oversampler) oversampler->reset();
    for (auto& v : voices) v.reset();
}

bool OB8Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

dsp::Voice::PerVoiceParams OB8Processor::snapshotParams() const
{
    auto get = [&](auto id) { return apvts.getRawParameterValue (id)->load(); };

    dsp::Voice::PerVoiceParams p;

    p.vco1Octave    = static_cast<int> (get (ParamID::vco1Octave)) - 2;
    p.vco2Octave    = static_cast<int> (get (ParamID::vco2Octave)) - 2;
    p.vco1Wave      = static_cast<int> (get (ParamID::vco1Wave))
                       == 0 ? dsp::PolyBLEPOscillator::Wave::Saw
                            : dsp::PolyBLEPOscillator::Wave::Pulse;
    p.vco2Wave      = static_cast<int> (get (ParamID::vco2Wave))
                       == 0 ? dsp::PolyBLEPOscillator::Wave::Saw
                            : dsp::PolyBLEPOscillator::Wave::Pulse;
    p.pulseWidth1   = get (ParamID::vco1Pw);
    p.pulseWidth2   = get (ParamID::vco2Pw);
    p.vco2DetuneSemis = get (ParamID::vco2Detune);

    p.xModAmount    = get (ParamID::xMod);
    p.sync          = get (ParamID::sync) > 0.5f;

    p.levelVco1     = get (ParamID::mixVco1);
    p.levelVco2     = get (ParamID::mixVco2);
    p.levelNoise    = get (ParamID::mixNoise);

    p.cutoffHz      = get (ParamID::cutoff);
    p.resonance     = get (ParamID::resonance);
    p.envAmount     = get (ParamID::envAmount);
    p.lfoToVcf      = get (ParamID::lfoToVcf);
    p.kbdTrack      = get (ParamID::kbdTrack);
    p.filterSlope   = static_cast<int> (get (ParamID::slope))
                       == 0 ? dsp::StateVariableFilter::Slope::TwoPole
                            : dsp::StateVariableFilter::Slope::FourPole;

    p.filtA = get (ParamID::filtA);
    p.filtD = get (ParamID::filtD);
    p.filtS = get (ParamID::filtS);
    p.filtR = get (ParamID::filtR);

    p.ampA  = get (ParamID::ampA);
    p.ampD  = get (ParamID::ampD);
    p.ampS  = get (ParamID::ampS);
    p.ampR  = get (ParamID::ampR);

    p.lfoToVco1Pitch = get (ParamID::lfoToVco1);
    p.lfoToVco2Pitch = get (ParamID::lfoToVco2);
    p.lfoToPwm       = get (ParamID::lfoToPwm);

    p.velToVca = get (ParamID::velToVca);
    p.velToVcf = get (ParamID::velToVcf);

    p.pitchBendSemis = currentBendSemis + get (ParamID::masterTune) * 0.01;

    return p;
}

void OB8Processor::handleMidiEvent (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())          noteOn  (msg.getNoteNumber(), msg.getFloatVelocity());
    else if (msg.isNoteOff())    noteOff (msg.getNoteNumber());
    else if (msg.isAllNotesOff() || msg.isAllSoundOff()) allNotesOff();
    else if (msg.isPitchWheel())
    {
        const float v = (msg.getPitchWheelValue() - 8192) / 8192.0f;
        const float range = apvts.getRawParameterValue (ParamID::bendRange)->load();
        currentBendSemis = v * range;
    }
}

void OB8Processor::noteOn (int midiNote, float velocity)
{
    const int mode = static_cast<int> (apvts.getRawParameterValue (ParamID::polyMode)->load());
    const auto p   = snapshotParams();
    ++noteOnCounter;

    if (mode == 1)        // Unison
    {
        const double detune = apvts.getRawParameterValue (ParamID::unisonDetune)->load();
        // Spread all 8 voices around the requested note.
        for (size_t i = 0; i < voices.size(); ++i)
        {
            const double n = static_cast<double> (i) - (voices.size() - 1) * 0.5;
            auto pp = p;
            pp.pitchBendSemis += n * detune;
            voices[i].startNote (midiNote, velocity, noteOnCounter, pp);
        }
        return;
    }

    if (mode == 2)        // Mono (single highest-priority voice with legato-ish)
    {
        // Steal voice 0 each time
        voices[0].startNote (midiNote, velocity, noteOnCounter, p);
        return;
    }

    // Poly: find a free voice; otherwise steal the oldest.
    int target = -1;
    for (int i = 0; i < kNumVoices; ++i)
    {
        if (! voices[i].isActive()) { target = i; break; }
    }
    if (target < 0)
    {
        int oldest = INT_MAX;
        for (int i = 0; i < kNumVoices; ++i)
        {
            if (voices[i].getNoteOnOrder() < oldest)
            {
                oldest = voices[i].getNoteOnOrder();
                target = i;
            }
        }
    }
    if (target >= 0)
        voices[target].startNote (midiNote, velocity, noteOnCounter, p);
}

void OB8Processor::noteOff (int midiNote)
{
    const int mode = static_cast<int> (apvts.getRawParameterValue (ParamID::polyMode)->load());

    if (mode == 1)   // Unison: release all
    {
        for (auto& v : voices)
            if (v.getMidiNote() == midiNote) v.stopNote();
        return;
    }
    if (mode == 2)   // Mono: release voice 0 if it matches
    {
        if (voices[0].getMidiNote() == midiNote) voices[0].stopNote();
        return;
    }

    for (auto& v : voices)
        if (v.getMidiNote() == midiNote) v.stopNote();
}

void OB8Processor::allNotesOff()
{
    for (auto& v : voices) v.stopNote();
}

void OB8Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals _;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    // Update LFO settings (block-rate)
    {
        const int shapeIdx = static_cast<int> (apvts.getRawParameterValue (ParamID::lfoShape)->load());
        lfo.setShape (static_cast<dsp::LFO::Shape> (shapeIdx));
        lfo.setRate  (apvts.getRawParameterValue (ParamID::lfoRate)->load());
    }

    // Apply MIDI events at the head of the block. Sample-accurate handling
    // would split the rendering loop at each event; not critical for panel
    // OB-8 behaviours.
    for (const auto md : midi)
        handleMidiEvent (md.getMessage());

    // ---- Upsample silence to obtain a properly sized OS-rate block.
    // We then overwrite that block with our synthesised audio and ask the
    // oversampler to decimate it back into the host buffer.
    juce::dsp::AudioBlock<float> hostBlock (buffer);
    auto osBlock      = oversampler->processSamplesUp (hostBlock);
    const int osSamples = static_cast<int> (osBlock.getNumSamples());

    // Generate the LFO buffer at oversampled rate (one float per sample)
    for (int i = 0; i < osSamples; ++i)
        lfoBuffer[static_cast<size_t> (i)] = static_cast<float> (lfo.processSample());

    // Render voices into a mono mix bus at OS rate
    mixBuffer.setSize (1, osSamples, false, false, true);
    mixBuffer.clear();
    float* mixPtr = mixBuffer.getWritePointer (0);

    const auto params = snapshotParams();

    for (auto& v : voices)
    {
        if (! v.isActive()) continue;
        v.renderAdd (mixPtr, osSamples, params, lfoBuffer.data());
    }

    // Master gain
    const float gainDb = apvts.getRawParameterValue (ParamID::masterGain)->load();
    const float gain   = juce::Decibels::decibelsToGain (gainDb);
    juce::FloatVectorOperations::multiply (mixPtr, gain, osSamples);

    // Copy the mono OS-rate mix into every channel of the OS block
    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        std::memcpy (osBlock.getChannelPointer (ch),
                     mixPtr,
                     sizeof (float) * static_cast<size_t> (osSamples));
    }

    // Decimate back to host rate, writing into the host buffer
    oversampler->processSamplesDown (hostBlock);
}

juce::AudioProcessorEditor* OB8Processor::createEditor()
{
    return new OB8Editor (*this);
}

void OB8Processor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void OB8Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

} // namespace ob8

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ob8::OB8Processor();
}
