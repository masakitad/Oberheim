#include "Parameters.h"

namespace ob8 {

using APVTS = juce::AudioProcessorValueTreeState;
using Range = juce::NormalisableRange<float>;

static std::unique_ptr<juce::AudioParameterFloat> mkF (const juce::String& id,
                                                      const juce::String& name,
                                                      Range range,
                                                      float def,
                                                      const juce::String& unit = {})
{
    juce::AudioParameterFloatAttributes attrs;
    attrs = attrs.withLabel (unit);
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name, range, def, attrs);
}

static std::unique_ptr<juce::AudioParameterChoice> mkC (const juce::String& id,
                                                        const juce::String& name,
                                                        juce::StringArray choices,
                                                        int defIdx)
{
    return std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { id, 1 }, name, choices, defIdx);
}

static std::unique_ptr<juce::AudioParameterBool> mkB (const juce::String& id,
                                                      const juce::String& name,
                                                      bool def)
{
    return std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { id, 1 }, name, def);
}

static Range hzLog (float lo, float hi)
{
    Range r (lo, hi);
    r.setSkewForCentre (std::sqrt (lo * hi));
    return r;
}

APVTS::ParameterLayout createParameterLayout()
{
    APVTS::ParameterLayout layout;

    using namespace ParamID;
    const juce::StringArray waves      { "Saw", "Pulse" };
    const juce::StringArray octaves    { "-2", "-1", "0", "+1", "+2" };
    const juce::StringArray modes      { "Poly", "Unison", "Split", "Double", "Mono" };
    const juce::StringArray slopes     { "2-Pole", "4-Pole" };
    const juce::StringArray lfoShapes  { "Triangle", "Square", "Saw Up", "Saw Down", "S&H" };
    const juce::StringArray splitOcts  { "-2", "-1", "0", "+1", "+2" };

    // VCO 1
    layout.add (mkC (vco1Octave, "VCO 1 Octave",  octaves, 2));
    layout.add (mkC (vco1Wave,   "VCO 1 Wave",    waves,   0));
    layout.add (mkF (vco1Pw,     "VCO 1 PW",      Range (0.05f, 0.95f),   0.50f));

    // VCO 2
    layout.add (mkC (vco2Octave, "VCO 2 Octave",  octaves, 2));
    layout.add (mkC (vco2Wave,   "VCO 2 Wave",    waves,   0));
    layout.add (mkF (vco2Pw,     "VCO 2 PW",      Range (0.05f, 0.95f),   0.50f));
    layout.add (mkF (vco2Detune, "VCO 2 Detune",  Range (-7.0f, 7.0f),    0.07f, "st"));

    // X-MOD / Sync
    layout.add (mkF (xMod, "X-Mod",   Range (0.0f, 1.0f), 0.0f));
    layout.add (mkB (sync, "Sync",    false));

    // Mixer
    layout.add (mkF (mixVco1,  "Mix VCO 1",  Range (0.0f, 1.0f), 0.8f));
    layout.add (mkF (mixVco2,  "Mix VCO 2",  Range (0.0f, 1.0f), 0.6f));
    layout.add (mkF (mixNoise, "Mix Noise",  Range (0.0f, 1.0f), 0.0f));

    // Filter
    layout.add (mkF (cutoff,    "Cutoff",     hzLog (20.0f, 18000.0f), 1200.0f, "Hz"));
    layout.add (mkF (resonance, "Resonance",  Range (0.0f, 1.0f), 0.0f));
    layout.add (mkF (envAmount, "Env Amount", Range (-1.0f, 1.0f), 0.5f));
    layout.add (mkF (lfoToVcf,  "LFO -> VCF", Range (0.0f, 1.0f), 0.0f));
    layout.add (mkF (kbdTrack,  "Kbd Track",  Range (0.0f, 1.0f), 0.5f));
    layout.add (mkC (slope,     "Slope",      slopes, 1));

    // Filter env
    layout.add (mkF (filtA, "Filt A", hzLog (0.001f, 10.0f), 0.005f, "s"));
    layout.add (mkF (filtD, "Filt D", hzLog (0.001f, 10.0f), 0.250f, "s"));
    layout.add (mkF (filtS, "Filt S", Range (0.0f, 1.0f),    0.0f));
    layout.add (mkF (filtR, "Filt R", hzLog (0.001f, 10.0f), 0.250f, "s"));

    // Amp env
    layout.add (mkF (ampA, "Amp A", hzLog (0.001f, 10.0f), 0.005f, "s"));
    layout.add (mkF (ampD, "Amp D", hzLog (0.001f, 10.0f), 0.250f, "s"));
    layout.add (mkF (ampS, "Amp S", Range (0.0f, 1.0f),    0.7f));
    layout.add (mkF (ampR, "Amp R", hzLog (0.001f, 10.0f), 0.250f, "s"));

    // LFO
    layout.add (mkF (lfoRate,   "LFO Rate",   hzLog (0.05f, 30.0f), 4.0f, "Hz"));
    layout.add (mkC (lfoShape,  "LFO Shape",  lfoShapes, 0));
    layout.add (mkF (lfoToVco1, "LFO -> VCO1", Range (0.0f, 1.0f), 0.0f));
    layout.add (mkF (lfoToVco2, "LFO -> VCO2", Range (0.0f, 1.0f), 0.0f));
    layout.add (mkF (lfoToPwm,  "LFO -> PWM",  Range (0.0f, 1.0f), 0.0f));

    // Velocity
    layout.add (mkF (velToVca, "Vel -> VCA", Range (0.0f, 1.0f), 0.0f));
    layout.add (mkF (velToVcf, "Vel -> VCF", Range (0.0f, 1.0f), 0.0f));

    // Page 2 modulation
    layout.add (mkF (envToVco1,   "Env -> VCO 1",   Range (-24.0f, 24.0f), 0.0f, "st"));
    layout.add (mkF (envToVco2,   "Env -> VCO 2",   Range (-24.0f, 24.0f), 0.0f, "st"));
    layout.add (mkF (envToPwm,    "Env -> PWM",     Range (0.0f, 0.45f), 0.0f));
    layout.add (mkF (atToVcf,     "AT -> VCF",      Range (-48.0f, 48.0f), 0.0f, "st"));
    layout.add (mkF (atToLfo,     "AT -> LFO",      Range (0.0f, 1.0f), 0.0f));
    layout.add (mkF (atToVca,     "AT -> VCA",      Range (0.0f, 0.5f), 0.0f));
    layout.add (mkF (mwToVcf,     "MW -> VCF",      Range (-48.0f, 48.0f), 0.0f, "st"));
    layout.add (mkF (mwToLfo,     "MW -> LFO",      Range (0.0f, 1.0f), 0.0f));
    layout.add (mkF (mwToVibrato, "MW -> Vibrato",  Range (0.0f, 1.0f), 0.0f, "st"));
    layout.add (mkB (lfoKeySync,  "LFO Key Sync",   false));

    // Global
    layout.add (mkC (polyMode,     "Mode",          modes, 0));
    layout.add (mkF (unisonDetune, "Unison Detune", Range (0.0f, 0.3f), 0.06f));
    layout.add (mkF (driftDepth,   "Drift Depth",   Range (0.0f, 0.20f), 0.04f));
    layout.add (mkF (masterGain,   "Master Gain",   Range (-24.0f, 6.0f), -6.0f, "dB"));
    layout.add (mkF (masterTune,   "Master Tune",   Range (-100.0f, 100.0f), 0.0f, "ct"));
    layout.add (mkF (bendRange,    "Bend Range",    Range (1.0f, 24.0f), 2.0f, "st"));

    // Split / Double
    layout.add (mkF (splitPoint,        "Split Point",     Range (24.0f, 96.0f), 60.0f));
    layout.add (mkC (splitOctaveOffset, "Split Octave",    splitOcts, 2));
    layout.add (mkF (splitDetune,       "Split Detune",    Range (-12.0f, 12.0f), 0.0f, "st"));
    layout.add (mkF (doubleDetune,      "Double Detune",   Range (0.0f, 1.0f), 0.10f, "st"));

    // Performance
    layout.add (mkF (glide, "Glide",   hzLog (0.001f, 2.0f), 0.001f, "s"));
    layout.add (mkB (hold,  "Hold",    false));

    // Delay
    layout.add (mkF (delayTimeL,    "Delay L",          hzLog (0.005f, 2.0f), 0.30f, "s"));
    layout.add (mkF (delayTimeR,    "Delay R",          hzLog (0.005f, 2.0f), 0.45f, "s"));
    layout.add (mkF (delayFeedback, "Delay Feedback",   Range (0.0f, 0.95f),  0.35f));
    layout.add (mkF (delayCross,    "Delay Cross",      Range (0.0f, 1.0f),   0.0f));
    layout.add (mkF (delayDamping,  "Delay Damping",    Range (0.0f, 1.0f),   0.40f));
    layout.add (mkF (delayMix,      "Delay Mix",        Range (0.0f, 1.0f),   0.0f));

    // Reverb
    layout.add (mkF (reverbSize,       "Reverb Size",       Range (0.5f, 1.5f),     1.0f));
    layout.add (mkF (reverbDecay,      "Reverb Decay",      Range (0.0f, 1.0f),     0.55f));
    layout.add (mkF (reverbDamping,    "Reverb Damping",    Range (0.0f, 1.0f),     0.45f));
    layout.add (mkF (reverbPreDelay,   "Reverb Pre-delay",  Range (0.0f, 0.2f),     0.012f, "s"));
    layout.add (mkF (reverbModulation, "Reverb Mod",        Range (0.0f, 0.005f),   0.001f));
    layout.add (mkF (reverbWidth,      "Reverb Width",      Range (0.0f, 1.0f),     1.0f));
    layout.add (mkF (reverbMix,        "Reverb Mix",        Range (0.0f, 1.0f),     0.0f));

    return layout;
}

} // namespace ob8
