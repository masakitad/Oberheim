#pragma once

#include "PolyBLEPOscillator.h"
#include "StateVariableFilter.h"
#include "Envelope.h"
#include "NoiseGenerator.h"
#include "AnalogDrift.h"
#include "DCBlocker.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace ob8::dsp {

/*
    A single OB-8 voice.

    Signal flow per voice:

        VCO1 ─┐
              ├──> MIX ──> VCF ──> VCA ──> DC blocker ──> out
        VCO2 ─┤
        NOISE ┘

    Modulation paths (set per-block by the processor):
        - Pitch:  KEY pitch + voice drift1/drift2 + LFO (if routed) + bend
        - PWM:    panel PW + LFO depth (if routed)
        - VCF:    panel cutoff + Filter Env * amount + LFO (if routed)
                  + keyboard track
        - VCA:    Amp Env (always)
        - X-MOD:  VCO1 output -> VCO2 frequency (linear FM)
        - SYNC:   VCO1 phase wrap -> VCO2 hard sync
*/
class Voice
{
public:
    struct PerVoiceParams
    {
        // Pitch
        double basePitchHz   = 440.0;
        double vco2DetuneSemis = 0.0;
        int    vco1Octave    = 0;       // -2..+2
        int    vco2Octave    = 0;

        // Waveforms
        PolyBLEPOscillator::Wave vco1Wave = PolyBLEPOscillator::Wave::Saw;
        PolyBLEPOscillator::Wave vco2Wave = PolyBLEPOscillator::Wave::Saw;
        double pulseWidth1   = 0.5;
        double pulseWidth2   = 0.5;

        // Mixer (0..1)
        double levelVco1 = 0.8;
        double levelVco2 = 0.6;
        double levelNoise = 0.0;

        // Cross-mod and sync
        double xModAmount = 0.0;    // 0..1 (depth of VCO1 -> VCO2 FM)
        bool   sync       = false;

        // Filter
        double cutoffHz    = 1200.0;
        double resonance   = 0.0;   // 0..1
        double envAmount   = 0.5;   // -1..+1 actually
        double lfoToVcf    = 0.0;
        double kbdTrack    = 0.5;   // 0 = off, 0.5 = half, 1 = full
        StateVariableFilter::Slope filterSlope = StateVariableFilter::Slope::FourPole;

        // Envelopes
        double filtA = 0.005, filtD = 0.250, filtS = 0.0, filtR = 0.250;
        double ampA  = 0.005, ampD  = 0.250, ampS  = 0.7, ampR  = 0.250;

        // LFO routing (depths)
        double lfoToVco1Pitch = 0.0;
        double lfoToVco2Pitch = 0.0;
        double lfoToPwm       = 0.0;

        // ----- Page 2: extra modulation destinations ----------------------
        // Filter envelope -> VCO pitch / PWM (classic "Page 2" routings)
        double envToVco1Semis = 0.0;     // -24..+24 semitones at env peak
        double envToVco2Semis = 0.0;
        double envToPwm       = 0.0;     // 0..0.45 added to PW at env peak

        // Aftertouch routings (state value supplied per-voice below)
        double atToVcfSemis   = 0.0;     // semitones added to cutoff at AT=1
        double atToLfoDepth   = 0.0;     // 0..1 (multiplies LFO output)
        double atToVca        = 0.0;     // 0..1 amplitude contribution

        // Mod wheel routings (state value supplied per-voice below)
        double mwToVcfSemis   = 0.0;
        double mwToLfoDepth   = 0.0;     // adds to vibrato amount
        double mwToVibratoSemis = 0.0;   // pure vibrato (LFO -> pitch via MW)

        // Live state inputs from the processor
        double aftertouch     = 0.0;     // 0..1
        double modWheel       = 0.0;     // 0..1

        // Velocity-to-VCA, velocity-to-VCF
        double velToVca = 0.0;
        double velToVcf = 0.0;

        // Pitch bend semitones applied to both VCOs
        double pitchBendSemis = 0.0;

        // Per-voice split offsets (applied externally per note)
        double splitOctaveOffset = 0.0;  // octaves added to both VCOs
        double splitDetuneSemis  = 0.0;  // semitones added to VCO2 only

        // Glide / portamento (seconds; 0 = no glide). Applied per-voice to
        // smoothly drift currentSmoothedNote toward the target MIDI note.
        double glideTime = 0.0;
    };

    void prepare (double sampleRate, uint32_t seed)
    {
        sr = sampleRate;
        vco1.prepare (sampleRate);
        vco2.prepare (sampleRate);
        vcf.prepare  (sampleRate);
        ampEnv.prepare (sampleRate);
        filtEnv.prepare (sampleRate);
        noise.reset (seed ^ 0xDEADBEEFu);
        drift1.prepare (sampleRate, seed);
        drift2.prepare (sampleRate, seed + 1);
        dcb.prepare (sampleRate);

        // Pre-spread initial phases to avoid all voices starting in lock-step
        vco1.reset (static_cast<double> (seed & 0xFFFF) / 65536.0);
        vco2.reset (static_cast<double> ((seed >> 16) & 0xFFFF) / 65536.0);
    }

    void reset()
    {
        vco1.reset();
        vco2.reset();
        vcf.reset();
        ampEnv.reset();
        filtEnv.reset();
        dcb.reset();
        active = false;
        currentMidiNote = -1;
    }

    bool isActive() const noexcept { return active; }
    bool isReleasing() const noexcept { return ampEnv.getStage() == Envelope::Stage::Release; }
    int  getMidiNote() const noexcept { return currentMidiNote; }
    int  getNoteOnOrder() const noexcept { return noteOnOrder; }

    void startNote (int midiNote, float velocity, int order, const PerVoiceParams& p)
    {
        const bool wasIdle = ! active;

        // If the voice was idle, snap the smoothed note to the new target so
        // the first note doesn't glide from 0. Otherwise keep the previous
        // value so subsequent notes glide from where the voice left off.
        if (wasIdle || currentSmoothedNote <= 0.5)
            currentSmoothedNote = midiNote;

        // NOTE: a previous revision used to vcf.reset() / dcb.reset() here
        // (when wasIdle == true) to clear "frozen" filter state inherited
        // from the previous note. That fixed one click but introduced a
        // worse one: with the filter integrators forced to zero, low-Q /
        // low-cutoff patches needed a few milliseconds for the state to
        // settle to the steady-state response of the new oscillator input,
        // and that settle was audible. Real analog filters never reset --
        // they integrate continuously -- so we now preserve the state.
        // The brief fade-in below masks the (very small) discontinuity
        // between the previous frozen state and the new audio.

        currentMidiNote = midiNote;
        currentVelocity = velocity;
        noteOnOrder     = order;
        active          = true;

        // 64-sample anti-click fade-in (~0.7 ms at the host rate, ~0.18 ms
        // at the 4x internal rate). Multiplies the output for the first N
        // samples of the new note so any tiny transient at startup is
        // smoothed away. Inaudible on its own; just there for safety.
        fadeInCountdown = 64;

        applyParams (p);
        ampEnv.noteOn();
        filtEnv.noteOn();
    }

    void stopNote()
    {
        ampEnv.noteOff();
        filtEnv.noteOff();
    }

    void killNote() noexcept
    {
        ampEnv.reset();
        filtEnv.reset();
        active = false;
        currentMidiNote = -1;
    }

    /*  Render `numSamples` of mono audio additively into `outBuffer`.
        `params` is read at the start of the block; modulation can vary
        across the block via the LFO sample stream supplied by the caller
        (lfoBuffer must be `numSamples` long). */
    void renderAdd (float* outBuffer, int numSamples,
                    const PerVoiceParams& params,
                    const float* lfoBuffer)
    {
        applyParams (params);

        // Per-block glide coefficient. 0 means glide is off (snap to target).
        // For glide > 0 we use a one-pole that reaches 99% of the target in
        // `glideTime` seconds.
        const double glideCoef = (params.glideTime <= 0.0001)
            ? 1.0
            : (1.0 - std::exp (-4.605 / std::max (1.0, params.glideTime * sr)));

        for (int i = 0; i < numSamples; ++i)
        {
            const double rawLfo = lfoBuffer != nullptr ? static_cast<double> (lfoBuffer[i]) : 0.0;

            // LFO depth modulated by aftertouch and mod wheel
            const double lfoDepthMul = 1.0
                                     + params.atToLfoDepth * params.aftertouch
                                     + params.mwToLfoDepth * params.modWheel;
            const double lfo = rawLfo * lfoDepthMul;

            // ----- Pitch in semitones -----------------------------------
            const double d1 = drift1.processSample();
            const double d2 = drift2.processSample();
            const double lfoPitch1 = lfo * params.lfoToVco1Pitch;
            const double lfoPitch2 = lfo * params.lfoToVco2Pitch;
            const double bend      = params.pitchBendSemis;

            // Filter env contribution at this point of the loop -- precomputed
            // below; declared here so it can feed pitch and PWM modulation.
            const double fe = filtEnv.processSample();

            // Vibrato from mod wheel: extra LFO -> pitch routing
            const double vibrato = rawLfo * params.mwToVibratoSemis * params.modWheel;

            // Glide: smoothly approach the target MIDI note
            currentSmoothedNote += glideCoef * (currentMidiNote - currentSmoothedNote);

            const double oct1 = params.vco1Octave + params.splitOctaveOffset;
            const double oct2 = params.vco2Octave + params.splitOctaveOffset;

            const double f1 = midiToHz (currentSmoothedNote + 12.0 * oct1
                                        + d1 + lfoPitch1 + bend + vibrato
                                        + params.envToVco1Semis * fe);
            const double f2 = midiToHz (currentSmoothedNote + 12.0 * oct2
                                        + params.vco2DetuneSemis
                                        + params.splitDetuneSemis
                                        + d2 + lfoPitch2 + bend + vibrato
                                        + params.envToVco2Semis * fe);

            vco1.setFrequency (f1);
            vco2.setFrequency (f2);

            // PWM: LFO depth + Filter Env depth (Page 2)
            const double envPwm  = params.envToPwm * fe;
            const double lfoPwm  = 0.45 * params.lfoToPwm * lfo;
            vco1.setPulseWidth (params.pulseWidth1 + lfoPwm + envPwm);
            vco2.setPulseWidth (params.pulseWidth2 + lfoPwm + envPwm);

            // ----- Oscillators ------------------------------------------
            const double o1 = vco1.processSample();

            // Hard sync slave to master (VCO1)
            if (params.sync)
                vco2.hardSync (vco1.wrappedThisSample(), 0.0);

            // Cross-mod: VCO1 audio injected into VCO2 phase increment
            // Scale chosen so that xMod=1 yields a strong but stable FM index.
            const double xMod = params.xModAmount * 0.002 * o1;
            const double o2   = vco2.processSample (xMod);

            // ----- Mixer ------------------------------------------------
            const double n = noise.processSample();
            const double mix = o1 * params.levelVco1
                             + o2 * params.levelVco2
                             + n  * params.levelNoise;

            // ----- Modulators -------------------------------------------
            // (filter env already advanced above for pitch/PWM routing)
            const double ae = ampEnv.processSample();

            // Filter cutoff in semitones above panel value (uses smoothed
            // note so kbd-tracking glides too).
            const double kbdSemis = (currentSmoothedNote - 60.0) * params.kbdTrack;
            const double envSemis = params.envAmount * fe * 84.0; // up to 7 octaves
            const double lfoSemis = params.lfoToVcf  * lfo * 36.0; // up to 3 oct
            const double velSemis = params.velToVcf  * currentVelocity * 48.0;
            const double atSemis  = params.atToVcfSemis * params.aftertouch;
            const double mwSemis  = params.mwToVcfSemis * params.modWheel;
            const double fc = params.cutoffHz
                              * std::exp2 ((kbdSemis + envSemis + lfoSemis
                                            + velSemis + atSemis + mwSemis) / 12.0);

            vcf.setCutoff (fc);
            vcf.setResonance (params.resonance);
            vcf.setSlope (params.filterSlope);

            const double filtered = vcf.processSample (mix);

            // ----- VCA --------------------------------------------------
            const double velAmp = 1.0 - params.velToVca * (1.0 - currentVelocity);
            const double atAmp  = 1.0 + params.atToVca * params.aftertouch;

            // 64-sample anti-click fade-in applied after startNote. Multiplies
            // the first samples of a fresh voice by a 0..1 ramp so any small
            // transient at activation (filter state mismatch, oscillator
            // phase jump from the frozen idle position) is smoothed away.
            double fadeFactor = 1.0;
            if (fadeInCountdown > 0)
            {
                fadeFactor = 1.0 - static_cast<double> (fadeInCountdown) / 64.0;
                --fadeInCountdown;
            }

            const double sample = filtered * ae * velAmp * atAmp * fadeFactor;

            outBuffer[i] += static_cast<float> (dcb.processSample (sample));
        }

        // Voice deactivation -- done ONCE per block after the inner loop has
        // finished, not per sample. This prevents any possibility of the
        // release tail being clipped mid-block (which would show up as a
        // sudden cutoff before the user-set release time elapses).
        if (! ampEnv.isActive())
        {
            active = false;
            currentMidiNote = -1;
        }
    }

private:
    void applyParams (const PerVoiceParams& p)
    {
        vco1.setWave (p.vco1Wave);
        vco2.setWave (p.vco2Wave);
        ampEnv.setADSR  (p.ampA,  p.ampD,  p.ampS,  p.ampR);
        filtEnv.setADSR (p.filtA, p.filtD, p.filtS, p.filtR);
    }

    static double midiToHz (double note) noexcept
    {
        return 440.0 * std::exp2 ((note - 69.0) / 12.0);
    }

    double sr = 44100.0;

    PolyBLEPOscillator    vco1, vco2;
    StateVariableFilter   vcf;
    Envelope              ampEnv, filtEnv;
    NoiseGenerator        noise;
    AnalogDrift           drift1, drift2;
    DCBlocker             dcb;

    bool   active             = false;
    int    currentMidiNote    = -1;
    double currentSmoothedNote = 60.0;  // glide state in MIDI note units
    float  currentVelocity    = 1.0f;
    int    noteOnOrder        = 0;
    int    fadeInCountdown    = 0;
};

} // namespace ob8::dsp
