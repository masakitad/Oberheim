#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "Parameters.h"

#include <vector>

namespace ob8 {

/*
    Factory preset bank (10 patches) for slot 1 of Bank 1.

    Each preset specifies only the parameters that differ from the APVTS
    defaults; everything else uses the layout default in Parameters.cpp.

    Patches are 1980s Oberheim OB-8 idioms recreated by ear and from the
    OB-8 service manual's example patches -- they are starting points, not
    bit-exact copies of any specific patch number.

    Implementation note: we deliberately use std::vector for the inner
    `params` member instead of std::initializer_list. A nested
    std::initializer_list's backing array does NOT have the outer list's
    lifetime, which would leave dangling pointers once the static
    initialiser completes.
*/
struct FactoryPreset
{
    struct PV { const char* id; float value; };
    const char* name;
    std::vector<PV> params;
};

inline const std::vector<FactoryPreset>& getFactoryPresets()
{
    using namespace ParamID;
    static const std::vector<FactoryPreset> presets
    {
        // ----------------------------------------------------------------
        // 1: Init Patch -- simple bright saw with snappy envelope.
        { "Init Patch",
          { { mixVco1, 0.8f }, { mixVco2, 0.0f }, { cutoff, 2000.0f },
            { envAmount, 0.3f }, { ampR, 0.4f } } },

        // ----------------------------------------------------------------
        // 2: OB Brass -- the classic stacked-saws Oberheim brass.
        { "OB Brass",
          { { vco1Wave, 0.0f }, { vco2Wave, 0.0f },
            { vco2Detune, 0.08f },
            { mixVco1, 0.7f }, { mixVco2, 0.7f },
            { cutoff, 350.0f }, { resonance, 0.05f },
            { envAmount, 0.75f }, { kbdTrack, 0.55f },
            { filtA, 0.004f }, { filtD, 0.35f }, { filtS, 0.15f }, { filtR, 0.25f },
            { ampA, 0.004f },  { ampD, 0.20f }, { ampS, 0.85f }, { ampR, 0.30f } } },

        // ----------------------------------------------------------------
        // 3: Lush Strings -- heavy detune, slow attack, long release.
        { "Lush Strings",
          { { mixVco1, 0.7f }, { mixVco2, 0.7f },
            { vco2Detune, 0.12f },
            { vco1Wave, 0.0f }, { vco2Wave, 0.0f },
            { cutoff, 1100.0f }, { resonance, 0.05f },
            { envAmount, 0.20f }, { kbdTrack, 0.55f },
            { filtA, 0.20f }, { filtD, 0.50f }, { filtS, 0.45f }, { filtR, 1.20f },
            { ampA, 0.45f },  { ampD, 0.50f }, { ampS, 0.85f }, { ampR, 1.30f },
            { lfoRate, 0.5f },{ lfoShape, 0.0f }, { lfoToVco1, 0.025f }, { lfoToVco2, 0.020f },
            { driftDepth, 0.06f } } },

        // ----------------------------------------------------------------
        // 4: Sync Lead -- hard sync on VCO 2, mono, env-swept VCO2 pitch.
        { "Sync Lead",
          { { polyMode, 4.0f },          // Mono
            { sync, 1.0f },
            { vco2Octave, 2.0f },        // idx 2 = octave 0 (relative)
            { mixVco1, 0.4f }, { mixVco2, 0.8f },
            { cutoff, 1500.0f }, { resonance, 0.3f },
            { envAmount, 0.6f },
            { envToVco2, 12.0f },        // Filter Env sweeps VCO2 +1 octave
            { filtA, 0.003f }, { filtD, 0.40f }, { filtS, 0.0f }, { filtR, 0.20f },
            { ampA, 0.003f },  { ampD, 0.30f }, { ampS, 0.85f }, { ampR, 0.30f } } },

        // ----------------------------------------------------------------
        // 5: Warm Pad -- very slow A/R, gentle LFO chorus, low filter.
        { "Warm Pad",
          { { vco1Wave, 0.0f }, { vco2Wave, 0.0f },
            { vco2Detune, 0.10f },
            { mixVco1, 0.7f }, { mixVco2, 0.7f },
            { cutoff, 700.0f }, { resonance, 0.05f },
            { envAmount, 0.25f }, { kbdTrack, 0.45f },
            { filtA, 0.80f }, { filtD, 0.80f }, { filtS, 0.40f }, { filtR, 2.50f },
            { ampA, 1.20f }, { ampD, 0.50f }, { ampS, 0.90f }, { ampR, 3.00f },
            { lfoRate, 0.35f }, { lfoShape, 0.0f }, { lfoToPwm, 0.40f },
            { vco1Pw, 0.5f }, { vco2Pw, 0.5f } } },

        // ----------------------------------------------------------------
        // 6: Funky Bass -- mono, lower octave, fast pluck filter env.
        { "Funky Bass",
          { { polyMode, 4.0f },          // Mono
            { vco1Octave, 1.0f },        // idx 1 = -1 octave
            { vco2Octave, 1.0f },
            { mixVco1, 0.8f }, { mixVco2, 0.5f },
            { vco2Detune, 0.03f },
            { vco1Wave, 0.0f }, { vco2Wave, 1.0f }, // Saw + Pulse
            { cutoff, 250.0f }, { resonance, 0.40f },
            { envAmount, 0.85f }, { kbdTrack, 0.70f },
            { filtA, 0.002f }, { filtD, 0.18f }, { filtS, 0.0f }, { filtR, 0.10f },
            { ampA, 0.002f }, { ampD, 0.20f }, { ampS, 0.70f }, { ampR, 0.15f },
            { velToVcf, 0.45f } } },

        // ----------------------------------------------------------------
        // 7: Bell -- X-MOD-driven inharmonic spectrum, fast decay.
        { "Bell",
          { { xMod, 0.55f },
            { vco1Wave, 1.0f }, { vco2Wave, 1.0f }, // Both Pulse
            { vco2Octave, 3.0f },        // idx 3 = +1 octave
            { vco2Detune, 0.30f },
            { mixVco1, 0.8f }, { mixVco2, 0.4f },
            { cutoff, 3500.0f }, { resonance, 0.15f },
            { envAmount, 0.30f }, { kbdTrack, 0.55f },
            { filtA, 0.002f }, { filtD, 0.45f }, { filtS, 0.0f }, { filtR, 0.40f },
            { ampA, 0.002f }, { ampD, 0.90f }, { ampS, 0.0f }, { ampR, 0.80f } } },

        // ----------------------------------------------------------------
        // 8: Sweep Pad -- slow filter env sweep, generous release.
        { "Sweep Pad",
          { { vco1Wave, 0.0f }, { vco2Wave, 0.0f },
            { vco2Detune, 0.07f },
            { mixVco1, 0.7f }, { mixVco2, 0.7f },
            { cutoff, 200.0f }, { resonance, 0.35f },
            { envAmount, 0.85f }, { kbdTrack, 0.20f },
            { filtA, 1.50f }, { filtD, 1.50f }, { filtS, 0.55f }, { filtR, 2.00f },
            { ampA, 0.50f }, { ampD, 0.50f }, { ampS, 0.85f }, { ampR, 2.00f },
            { lfoRate, 0.20f }, { lfoShape, 0.0f }, { lfoToVcf, 0.20f } } },

        // ----------------------------------------------------------------
        // 9: Plucky -- short percussive pluck, single voice envelope.
        { "Plucky",
          { { vco1Wave, 0.0f }, { vco2Wave, 1.0f },
            { vco2Detune, 0.05f },
            { mixVco1, 0.7f }, { mixVco2, 0.5f },
            { cutoff, 400.0f }, { resonance, 0.20f },
            { envAmount, 0.85f }, { kbdTrack, 0.65f },
            { filtA, 0.002f }, { filtD, 0.18f }, { filtS, 0.0f }, { filtR, 0.10f },
            { ampA, 0.002f }, { ampD, 0.18f }, { ampS, 0.0f }, { ampR, 0.20f },
            { velToVca, 0.7f }, { velToVcf, 0.45f } } },

        // ----------------------------------------------------------------
        // 10: Acid Bass -- mono, screaming resonance, env sweep.
        { "Acid Bass",
          { { polyMode, 4.0f },          // Mono
            { vco1Octave, 1.0f },        // idx 1 = -1 octave
            { vco1Wave, 0.0f },          // Saw
            { mixVco1, 0.9f }, { mixVco2, 0.0f },
            { cutoff, 180.0f }, { resonance, 0.85f },
            { envAmount, 0.95f }, { kbdTrack, 0.65f },
            { slope, 1.0f },             // 4-pole
            { filtA, 0.002f }, { filtD, 0.25f }, { filtS, 0.0f }, { filtR, 0.15f },
            { ampA, 0.002f }, { ampD, 0.20f }, { ampS, 0.85f }, { ampR, 0.15f },
            { glide, 0.080f },
            { velToVcf, 0.40f } } },
    };
    return presets;
}

} // namespace ob8
