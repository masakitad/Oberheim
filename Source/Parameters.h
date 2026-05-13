#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ob8 {

/*
    All AudioProcessorValueTreeState parameter IDs are listed here so that
    the editor and processor agree.
*/
namespace ParamID
{
    // VCO 1
    static constexpr auto vco1Octave   = "vco1_octave";
    static constexpr auto vco1Wave     = "vco1_wave";     // 0=Saw, 1=Pulse
    static constexpr auto vco1Pw       = "vco1_pw";

    // VCO 2
    static constexpr auto vco2Octave   = "vco2_octave";
    static constexpr auto vco2Wave     = "vco2_wave";
    static constexpr auto vco2Pw       = "vco2_pw";
    static constexpr auto vco2Detune   = "vco2_detune";   // semitones, -7..+7

    // X-MOD / Sync
    static constexpr auto xMod         = "xmod";
    static constexpr auto sync         = "sync";

    // Mixer
    static constexpr auto mixVco1      = "mix_vco1";
    static constexpr auto mixVco2      = "mix_vco2";
    static constexpr auto mixNoise     = "mix_noise";

    // Filter
    static constexpr auto cutoff       = "cutoff";
    static constexpr auto resonance    = "resonance";
    static constexpr auto envAmount    = "env_amount";
    static constexpr auto lfoToVcf     = "lfo_to_vcf";
    static constexpr auto kbdTrack     = "kbd_track";
    static constexpr auto slope        = "slope";          // 0=2P, 1=4P

    // Filter envelope
    static constexpr auto filtA = "filt_a";
    static constexpr auto filtD = "filt_d";
    static constexpr auto filtS = "filt_s";
    static constexpr auto filtR = "filt_r";

    // Amp envelope
    static constexpr auto ampA = "amp_a";
    static constexpr auto ampD = "amp_d";
    static constexpr auto ampS = "amp_s";
    static constexpr auto ampR = "amp_r";

    // LFO
    static constexpr auto lfoRate   = "lfo_rate";
    static constexpr auto lfoShape  = "lfo_shape";        // 0..4
    static constexpr auto lfoToVco1 = "lfo_to_vco1";
    static constexpr auto lfoToVco2 = "lfo_to_vco2";
    static constexpr auto lfoToPwm  = "lfo_to_pwm";

    // Velocity
    static constexpr auto velToVca = "vel_to_vca";
    static constexpr auto velToVcf = "vel_to_vcf";

    // Global / voice
    static constexpr auto polyMode    = "poly_mode";      // 0=Poly, 1=Unison, 2=Mono
    static constexpr auto unisonDetune = "unison_detune";
    static constexpr auto driftDepth  = "drift_depth";
    static constexpr auto masterGain  = "master_gain";
    static constexpr auto masterTune  = "master_tune";    // cents
    static constexpr auto bendRange   = "bend_range";     // semis
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace ob8
