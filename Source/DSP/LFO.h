#pragma once

#include <cmath>
#include <random>
#include <algorithm>

namespace ob8::dsp {

/*
    Global LFO shared across voices (the OB-8 has a single LFO whose
    destinations are routed via the LFO panel switches).

    Shapes: Triangle, Square, Saw (up), Inverse-saw (down), Sample & Hold.
    Output is in [-1, 1].
*/
class LFO
{
public:
    enum class Shape { Triangle, Square, SawUp, SawDown, SampleAndHold };

    void prepare (double sampleRate) noexcept
    {
        sr  = sampleRate;
        inv = 1.0 / sr;
        reset();
    }

    void reset() noexcept
    {
        phase  = 0.0;
        sample = 0.0;
        rng.seed (0x12345678u);
    }

    void setRate (double hz) noexcept
    {
        rate     = std::clamp (hz, 0.02, 50.0);
        phaseInc = rate * inv;
    }

    void setShape (Shape s) noexcept { shape = s; }

    inline double processSample() noexcept
    {
        double y = 0.0;
        switch (shape)
        {
            case Shape::Triangle:
                y = 1.0 - 4.0 * std::abs (phase - 0.5);
                break;
            case Shape::Square:
                y = (phase < 0.5) ? 1.0 : -1.0;
                break;
            case Shape::SawUp:
                y = 2.0 * phase - 1.0;
                break;
            case Shape::SawDown:
                y = 1.0 - 2.0 * phase;
                break;
            case Shape::SampleAndHold:
                y = sample;
                break;
        }

        phase += phaseInc;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            // Re-sample S&H once per cycle, using a Mersenne Twister for
            // deterministic but well-distributed values.
            std::uniform_real_distribution<double> d (-1.0, 1.0);
            sample = d (rng);
        }
        return y;
    }

private:
    double sr       = 44100.0;
    double inv      = 1.0 / 44100.0;
    double rate     = 1.0;
    double phaseInc = 0.0;
    double phase    = 0.0;
    double sample   = 0.0;
    Shape  shape    = Shape::Triangle;
    std::mt19937 rng { 0x12345678u };
};

} // namespace ob8::dsp
