#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"

namespace ob8 {

/*
    MacroBridge ties the three SIMPLE-view macro parameters to several
    underlying synth parameters at once, so a single TONE / MOTION / SPACE
    knob feels musically intuitive while the engine sees the usual
    individual fields.

    Each macro is centred at the natural "neutral" feel of an OB-8 patch:

      TONE  (0 .. 1):
        0   -> dark, low cutoff, no envelope sweep
        0.5 -> moderately bright with light dynamics
        1   -> very open, strong env sweep, a touch of resonance

      MOTION (0 .. 1):
        0   -> no movement, LFO silent, drift minimal
        1   -> mid-tempo LFO modulating filter + a hint of vibrato +
               wider analog drift

      SPACE  (0 .. 1):
        0   -> bone dry
        1   -> long reverb tail + audible delay echoes

    The bridge listens for parameter changes and writes through the
    parameter API (setValueNotifyingHost + convertTo0to1) so the host's
    automation lane / undo system still sees the writes as deliberate
    parameter moves. We listen on the macros only -- the underlying
    parameters are still freely user-editable when the FULL view is
    active; moving a macro just stomps over them on the next gesture.
*/
class MacroBridge : private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit MacroBridge (juce::AudioProcessorValueTreeState& a) : apvts (a)
    {
        apvts.addParameterListener (ParamID::macroTone,   this);
        apvts.addParameterListener (ParamID::macroMotion, this);
        apvts.addParameterListener (ParamID::macroSpace,  this);
    }

    ~MacroBridge() override
    {
        apvts.removeParameterListener (ParamID::macroTone,   this);
        apvts.removeParameterListener (ParamID::macroMotion, this);
        apvts.removeParameterListener (ParamID::macroSpace,  this);
    }

private:
    void parameterChanged (const juce::String& paramID, float newValue) override
    {
        if      (paramID == ParamID::macroTone)   applyTone   (newValue);
        else if (paramID == ParamID::macroMotion) applyMotion (newValue);
        else if (paramID == ParamID::macroSpace)  applySpace  (newValue);
    }

    void setParam (const char* id, float realValue)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (realValue));
    }

    void applyTone (float v)
    {
        // Cutoff: 80 Hz @ 0, ~1.6 kHz @ 0.5, ~20 kHz @ 1 (log map)
        const float cutoff = 80.0f * std::pow (250.0f, v);
        setParam (ParamID::cutoff, cutoff);

        // Filter envelope amount: gentle at low, dramatic at high
        setParam (ParamID::envAmount, juce::jmap (v, 0.05f, 0.85f));

        // Resonance: zero until ~0.6, then climb to 0.45
        const float res = (v <= 0.6f) ? 0.0f : juce::jmap (v, 0.6f, 1.0f, 0.0f, 0.45f);
        setParam (ParamID::resonance, res);

        // Brightness also nudges the filter envelope decay to a snappier
        // value at higher tone (so bright patches sound "plucky")
        setParam (ParamID::filtD, juce::jmap (v, 0.50f, 0.18f));
    }

    void applyMotion (float v)
    {
        // LFO -> VCF cutoff sweep
        setParam (ParamID::lfoToVcf, v * 0.40f);

        // LFO rate: 0.5 Hz at 0, 5 Hz at 1
        setParam (ParamID::lfoRate, juce::jmap (v, 0.5f, 5.0f));

        // Slight vibrato via LFO -> VCO1 pitch
        setParam (ParamID::lfoToVco1, v * 0.04f);

        // Wider analog drift as motion goes up
        setParam (ParamID::driftDepth, juce::jmap (v, 0.03f, 0.12f));

        // Slow PWM motion for pulse waves
        setParam (ParamID::lfoToPwm, v * 0.30f);
    }

    void applySpace (float v)
    {
        // Reverb mix and tail
        setParam (ParamID::reverbMix,   v * 0.65f);
        setParam (ParamID::reverbDecay, juce::jmap (v, 0.40f, 0.85f));

        // A touch of stereo delay in parallel
        setParam (ParamID::delayMix,      v * 0.30f);
        setParam (ParamID::delayFeedback, juce::jmap (v, 0.20f, 0.45f));
    }

    juce::AudioProcessorValueTreeState& apvts;
};

} // namespace ob8
