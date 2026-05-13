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

        // Velocity-to-VCA, velocity-to-VCF
        double velToVca = 0.0;
        double velToVcf = 0.0;

        // Pitch bend semitones applied to both VCOs
        double pitchBendSemis = 0.0;
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
    int  getMidiNote() const noexcept { return currentMidiNote; }
    int  getNoteOnOrder() const noexcept { return noteOnOrder; }

    void startNote (int midiNote, float velocity, int order, const PerVoiceParams& p)
    {
        currentMidiNote = midiNote;
        currentVelocity = velocity;
        noteOnOrder     = order;
        active          = true;
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

        for (int i = 0; i < numSamples; ++i)
        {
            const double lfo = lfoBuffer != nullptr ? static_cast<double> (lfoBuffer[i]) : 0.0;

            // ----- Pitch in cents ---------------------------------------
            const double d1 = drift1.processSample();
            const double d2 = drift2.processSample();
            const double lfoPitch1 = lfo * params.lfoToVco1Pitch;
            const double lfoPitch2 = lfo * params.lfoToVco2Pitch;
            const double bend      = params.pitchBendSemis;

            const double f1 = midiToHz (currentMidiNote + 12.0 * params.vco1Octave
                                        + d1 + lfoPitch1 + bend);
            const double f2 = midiToHz (currentMidiNote + 12.0 * params.vco2Octave
                                        + params.vco2DetuneSemis
                                        + d2 + lfoPitch2 + bend);

            vco1.setFrequency (f1);
            vco2.setFrequency (f2);

            // PWM
            const double pwmDepth1 = 0.45 * params.lfoToPwm * lfo;
            const double pwmDepth2 = 0.45 * params.lfoToPwm * lfo;
            vco1.setPulseWidth (params.pulseWidth1 + pwmDepth1);
            vco2.setPulseWidth (params.pulseWidth2 + pwmDepth2);

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
            const double fe = filtEnv.processSample();
            const double ae = ampEnv.processSample();

            // Filter cutoff in semitones above panel value
            const double kbdSemis = (currentMidiNote - 60.0) * params.kbdTrack;
            const double envSemis = params.envAmount * fe * 84.0; // up to 7 octaves
            const double lfoSemis = params.lfoToVcf  * lfo * 36.0; // up to 3 oct
            const double velSemis = params.velToVcf  * currentVelocity * 48.0;
            const double fc = params.cutoffHz
                              * std::exp2 ((kbdSemis + envSemis + lfoSemis + velSemis) / 12.0);

            vcf.setCutoff (fc);
            vcf.setResonance (params.resonance);
            vcf.setSlope (params.filterSlope);

            const double filtered = vcf.processSample (mix);

            // ----- VCA --------------------------------------------------
            const double velAmp = 1.0 - params.velToVca * (1.0 - currentVelocity);
            const double sample = filtered * ae * velAmp;

            outBuffer[i] += static_cast<float> (dcb.processSample (sample));

            // Voice goes inactive when amp envelope finishes its release tail
            if (! ampEnv.isActive())
            {
                active = false;
                currentMidiNote = -1;
                // Don't break: zero contribution from here on is fine and
                // simplifies bookkeeping in the host loop.
            }
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

    bool   active           = false;
    int    currentMidiNote  = -1;
    float  currentVelocity  = 1.0f;
    int    noteOnOrder      = 0;
};

} // namespace ob8::dsp
