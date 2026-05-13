#pragma once

#include <cmath>
#include <random>

namespace ob8::dsp {

/*
    Per-voice low-frequency analogue-style drift. Real CEM3340 oscillators
    drift slowly due to temperature variations on the exponential converter;
    that drift, together with small fixed offsets between voices, is much of
    what makes OB-8 unison/poly chords sound rich.

    We model it as a band-limited brown-noise / smoothed white-noise source
    whose output is in fractional semitones. Typical depth is a few cents.

    Each voice owns one instance per oscillator.
*/
class AnalogDrift
{
public:
    void prepare (double sampleRate, uint32_t seed) noexcept
    {
        sr   = sampleRate;
        rng.seed (seed ? seed : 0xA5A5u);
        // Low-pass coefficient for the brown-noise smoothing (~1 Hz)
        smooth = 1.0 - std::exp (-2.0 * 3.14159265358979323846 * 1.0 / sr);
        value  = 0.0;
    }

    void setDepthSemitones (double semis) noexcept { depth = semis; }

    inline double processSample() noexcept
    {
        std::uniform_real_distribution<double> d (-1.0, 1.0);
        const double n = d (rng);
        value += smooth * (n - value);
        return value * depth;
    }

private:
    double       sr     = 44100.0;
    double       smooth = 0.0;
    double       value  = 0.0;
    double       depth  = 0.04;   // ~4 cents
    std::mt19937 rng { 0xA5A5u };
};

} // namespace ob8::dsp
