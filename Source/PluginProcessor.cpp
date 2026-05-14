#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <climits>

namespace ob8 {

OB8Processor::OB8Processor()
    : juce::AudioProcessor (BusesProperties()
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    polyAfterTouch.resize (128);
    for (auto& v : polyAfterTouch) v = 0;

    // Initialise an empty bank tree with 120 patch slots
    bankState = juce::ValueTree ("BANK");
    bankState.setProperty ("name", "User Bank", nullptr);
    for (int b = 0; b < kNumBanks; ++b)
    {
        juce::ValueTree bank ("Bank");
        bank.setProperty ("index", b, nullptr);
        for (int p = 0; p < kPatchesPerBank; ++p)
        {
            juce::ValueTree patch ("Patch");
            patch.setProperty ("name", juce::String ("Init ") + juce::String (b * kPatchesPerBank + p + 1), nullptr);
            bank.appendChild (patch, nullptr);
        }
        bankState.appendChild (bank, nullptr);
    }
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

    // Page 2 routings
    p.envToVco1Semis    = get (ParamID::envToVco1);
    p.envToVco2Semis    = get (ParamID::envToVco2);
    p.envToPwm          = get (ParamID::envToPwm);
    p.atToVcfSemis      = get (ParamID::atToVcf);
    p.atToLfoDepth      = get (ParamID::atToLfo);
    p.atToVca           = get (ParamID::atToVca);
    p.mwToVcfSemis      = get (ParamID::mwToVcf);
    p.mwToLfoDepth      = get (ParamID::mwToLfo);
    p.mwToVibratoSemis  = get (ParamID::mwToVibrato);

    p.aftertouch = currentAfterT;
    p.modWheel   = currentModWheel;

    p.pitchBendSemis = currentBendSemis + get (ParamID::masterTune) * 0.01;

    p.glideTime = get (ParamID::glide);

    return p;
}

void OB8Processor::handleMidiEvent (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
    }
    else if (msg.isNoteOff())
    {
        const bool holdOn = apvts.getRawParameterValue (ParamID::hold)->load() > 0.5f;
        if (sustainPedalDown || holdOn)
            sustainedNotes.add (msg.getNoteNumber());
        else
            noteOff (msg.getNoteNumber());
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        allNotesOff();
        sustainedNotes.clear();
    }
    else if (msg.isPitchWheel())
    {
        const float v = (msg.getPitchWheelValue() - 8192) / 8192.0f;
        const float range = apvts.getRawParameterValue (ParamID::bendRange)->load();
        currentBendSemis = v * range;
    }
    else if (msg.isChannelPressure())
    {
        currentAfterT = msg.getChannelPressureValue() / 127.0;
    }
    else if (msg.isAftertouch())
    {
        const int n = msg.getNoteNumber();
        if (juce::isPositiveAndBelow (n, polyAfterTouch.size()))
            polyAfterTouch.set (n, msg.getAfterTouchValue());
        // Highest poly aftertouch also drives channel aftertouch as a fallback
        currentAfterT = juce::jmax (currentAfterT,
                                    msg.getAfterTouchValue() / 127.0);
    }
    else if (msg.isController())
    {
        const int cc  = msg.getControllerNumber();
        const int val = msg.getControllerValue();
        const float n = val / 127.0f;

        switch (cc)
        {
            case 1:    // Mod wheel
                currentModWheel = n;
                break;
            case 7:    // Volume
                if (auto* p = apvts.getParameter (ParamID::masterGain))
                    p->setValueNotifyingHost (
                        p->convertTo0to1 (juce::jmap (n, -24.0f, 6.0f)));
                break;
            case 11:   // Expression -- treat as VCA scaler via AT-to-VCA
                currentAfterT = juce::jmax (currentAfterT, (double) n);
                break;
            case 64:   // Sustain pedal
                sustainPedalDown = val >= 64;
                if (! sustainPedalDown)
                {
                    const bool holdOn = apvts.getRawParameterValue (ParamID::hold)->load() > 0.5f;
                    if (! holdOn)
                    {
                        for (int note : sustainedNotes) noteOff (note);
                        sustainedNotes.clear();
                    }
                }
                break;
            case 71:   // Resonance
                if (auto* p = apvts.getParameter (ParamID::resonance))
                    p->setValueNotifyingHost (n);
                break;
            case 72:   // Release
                if (auto* p = apvts.getParameter (ParamID::ampR))
                    p->setValueNotifyingHost (n);
                break;
            case 73:   // Attack
                if (auto* p = apvts.getParameter (ParamID::ampA))
                    p->setValueNotifyingHost (n);
                break;
            case 74:   // Brightness / cutoff
                if (auto* p = apvts.getParameter (ParamID::cutoff))
                    p->setValueNotifyingHost (n);
                break;
            case 75:   // Decay
                if (auto* p = apvts.getParameter (ParamID::ampD))
                    p->setValueNotifyingHost (n);
                break;
            case 76:   // Vibrato (LFO) rate
                if (auto* p = apvts.getParameter (ParamID::lfoRate))
                    p->setValueNotifyingHost (n);
                break;
            case 77:   // Vibrato depth (MW -> vibrato)
                if (auto* p = apvts.getParameter (ParamID::mwToVibrato))
                    p->setValueNotifyingHost (n);
                break;
            case 120:  // All sound off
            case 123:  // All notes off
                allNotesOff();
                sustainedNotes.clear();
                break;
            default:
                break;
        }
    }
}

bool OB8Processor::isAnyVoiceActive() const noexcept
{
    for (const auto& v : voices) if (v.isActive()) return true;
    return false;
}

void OB8Processor::resetLfoIfKeySync (bool wasIdle)
{
    if (wasIdle && apvts.getRawParameterValue (ParamID::lfoKeySync)->load() > 0.5f)
        lfo.reset();
}

void OB8Processor::noteOn (int midiNote, float velocity)
{
    const int  mode    = static_cast<int> (apvts.getRawParameterValue (ParamID::polyMode)->load());
    const bool wasIdle = ! isAnyVoiceActive();
    const auto p       = snapshotParams();
    ++noteOnCounter;
    resetLfoIfKeySync (wasIdle);

    // Voice stealing policy:
    //   1) Free voice (inactive) if available.
    //   2) Otherwise prefer a voice currently in Release stage (its envelope
    //      is already letting go, so stealing it is least audible).
    //   3) Otherwise steal the oldest voice (lowest note-on order).
    auto findFreeOrSteal = [&] (int firstIdx, int lastIdx) -> int
    {
        // Pass 1: a truly free voice
        for (int i = firstIdx; i <= lastIdx; ++i)
            if (! voices[i].isActive()) return i;

        // Pass 2: a voice that is already releasing (oldest one)
        int oldestRel = INT_MAX, relIdx = -1;
        for (int i = firstIdx; i <= lastIdx; ++i)
        {
            if (voices[i].isReleasing() && voices[i].getNoteOnOrder() < oldestRel)
            {
                oldestRel = voices[i].getNoteOnOrder();
                relIdx    = i;
            }
        }
        if (relIdx >= 0) return relIdx;

        // Pass 3: oldest sustaining/active voice
        int oldest = INT_MAX, target = firstIdx;
        for (int i = firstIdx; i <= lastIdx; ++i)
        {
            if (voices[i].getNoteOnOrder() < oldest)
            {
                oldest = voices[i].getNoteOnOrder();
                target = i;
            }
        }
        return target;
    };


    if (mode == 1)        // Unison: all 8 voices on the same note
    {
        const double detune = apvts.getRawParameterValue (ParamID::unisonDetune)->load();
        for (size_t i = 0; i < voices.size(); ++i)
        {
            const double n = static_cast<double> (i) - (voices.size() - 1) * 0.5;
            auto pp = p;
            pp.pitchBendSemis += n * detune;
            voices[i].startNote (midiNote, velocity, noteOnCounter, pp);
        }
        return;
    }

    if (mode == 2)        // Split: keyboard divided at split point
    {
        const int splitPt = static_cast<int> (
            apvts.getRawParameterValue (ParamID::splitPoint)->load());
        const int splitOct = static_cast<int> (
            apvts.getRawParameterValue (ParamID::splitOctaveOffset)->load()) - 2;
        const double splitDet = apvts.getRawParameterValue (ParamID::splitDetune)->load();

        auto pp = p;
        const bool upper = midiNote >= splitPt;
        if (upper)
        {
            pp.splitOctaveOffset = splitOct;
            pp.splitDetuneSemis  = splitDet;
        }
        // Lower notes use voices 0..3, upper notes use voices 4..7
        const int target = upper ? findFreeOrSteal (4, 7) : findFreeOrSteal (0, 3);
        voices[target].startNote (midiNote, velocity, noteOnCounter, pp);
        return;
    }

    if (mode == 3)        // Double: layer two detuned voices per note
    {
        const double det = apvts.getRawParameterValue (ParamID::doubleDetune)->load();
        // Use voices 0..3 for "layer A" and 4..7 for "layer B" (with detune)
        const int a = findFreeOrSteal (0, 3);
        const int b = findFreeOrSteal (4, 7);

        auto pa = p;
        auto pb = p;
        pa.pitchBendSemis -= det * 0.5;
        pb.pitchBendSemis += det * 0.5;
        voices[a].startNote (midiNote, velocity, noteOnCounter, pa);
        voices[b].startNote (midiNote, velocity, noteOnCounter, pb);
        return;
    }

    if (mode == 4)        // Mono
    {
        voices[0].startNote (midiNote, velocity, noteOnCounter, p);
        return;
    }

    // Mode 0: Poly -- one voice per note, with smart stealing
    const int target = findFreeOrSteal (0, kNumVoices - 1);
    voices[target].startNote (midiNote, velocity, noteOnCounter, p);
}

void OB8Processor::noteOff (int midiNote)
{
    const int mode = static_cast<int> (apvts.getRawParameterValue (ParamID::polyMode)->load());

    if (mode == 4)   // Mono: only voice 0 plays at a time
    {
        if (voices[0].getMidiNote() == midiNote) voices[0].stopNote();
        return;
    }
    // Poly / Unison / Split / Double: release every voice that matches
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

    buffer.clear();

    // Inject events from the on-screen / PC keyboard into the MIDI buffer
    // before we iterate through it. Pass true so externally received notes
    // are also forwarded to any keyboard listeners (highlighting the keys).
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

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

    // If Hold was toggled OFF (and the pedal isn't down), release any notes
    // currently being held in the sustained-notes set. Polled here so the
    // user's panel toggle takes effect on the next block.
    {
        const bool holdOn = apvts.getRawParameterValue (ParamID::hold)->load() > 0.5f;
        if (! holdOn && ! sustainPedalDown && ! sustainedNotes.isEmpty())
        {
            for (int note : sustainedNotes) noteOff (note);
            sustainedNotes.clear();
        }
    }

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
    juce::XmlElement root ("OB8State");

    if (auto params = apvts.copyState(); params.isValid())
    {
        if (auto px = params.createXml())
            root.addChildElement (px.release());
    }

    if (bankState.isValid())
    {
        if (auto bx = bankState.createXml())
            root.addChildElement (bx.release());
    }

    root.setAttribute ("currentBank",    currentBank);
    root.setAttribute ("currentProgram", currentProgram);
    root.setAttribute ("currentPatchName", currentPatchName);

    copyXmlToBinary (root, destData);
}

void OB8Processor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;

    if (auto* params = xml->getChildByName (apvts.state.getType().toString()))
        apvts.replaceState (juce::ValueTree::fromXml (*params));

    if (auto* bank = xml->getChildByName ("BANK"))
        bankState = juce::ValueTree::fromXml (*bank);

    currentBank      = xml->getIntAttribute    ("currentBank",      0);
    currentProgram   = xml->getIntAttribute    ("currentProgram",   0);
    currentPatchName = xml->getStringAttribute ("currentPatchName", "Init Patch");
}

void OB8Processor::saveCurrentPatchToXml (juce::XmlElement& dest,
                                          const juce::String& patchName)
{
    dest.setAttribute ("name", patchName);
    dest.setAttribute ("version", 1);
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto x = state.createXml())
            dest.addChildElement (x.release());
    }
}

bool OB8Processor::loadCurrentPatchFromXml (const juce::XmlElement& src)
{
    const auto typeName = apvts.state.getType().toString();
    if (auto* params = src.getChildByName (typeName))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*params));
        currentPatchName = src.getStringAttribute ("name", "Loaded Patch");
        return true;
    }
    return false;
}

bool OB8Processor::saveBankToFile (const juce::File& f) const
{
    juce::XmlElement root ("OB8Bank");
    root.setAttribute ("version", 1);
    root.setAttribute ("name", bankState.getProperty ("name").toString());
    if (auto bx = bankState.createXml())
        root.addChildElement (bx.release());
    return root.writeTo (f, juce::XmlElement::TextFormat());
}

bool OB8Processor::loadBankFromFile (const juce::File& f)
{
    if (! f.existsAsFile()) return false;
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr) return false;
    if (auto* bank = xml->getChildByName ("BANK"))
    {
        bankState = juce::ValueTree::fromXml (*bank);
        return true;
    }
    return false;
}

} // namespace ob8

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ob8::OB8Processor();
}
