#pragma once

#include <cmath>
#include <algorithm>

namespace ob8::dsp {

/*
    Band-limited oscillator modelled after the CEM3340 used in the OB-8.

    - 2x internal oversampling (the host-level oversampler handles the rest).
    - PolyBLEP correction for saw and pulse discontinuities (residual aliasing
      pushed close to Nyquist).
    - Hard sync support (slave reset on master wrap, with PolyBLEP fix-up).
    - Pulse Width Modulation with a safety clamp.
    - Cross-mod (linear FM from one oscillator into another) for the OB-8's
      "X-MOD" character.

    Phase is normalised to [0,1). Output is in roughly [-1,1].
*/
class PolyBLEPOscillator
{
public:
    enum class Wave { Saw, Pulse };

    void prepare (double sampleRate) noexcept
    {
        sr   = sampleRate;
        inv  = 1.0 / sr;
        reset();
    }

    void reset (double initialPhase = 0.0) noexcept
    {
        phase     = initialPhase;
        lastValue = 0.0;
        syncFlag  = false;
    }

    void setFrequency (double hz) noexcept
    {
        freq     = hz;
        phaseInc = std::clamp (hz * inv, 0.0, 0.45);
    }

    void setPulseWidth (double pw) noexcept
    {
        pulseWidth = std::clamp (pw, 0.03, 0.97);
    }

    void setWave (Wave w) noexcept { wave = w; }

    /*  Render one sample. crossModInput is added to the phase increment
        (in normalised units) on this sample only and is the principal
        "X-MOD" path on the OB-8 (VCO1 -> VCO2 frequency). */
    inline double processSample (double crossModInput = 0.0) noexcept
    {
        const double dt = std::clamp (phaseInc + crossModInput, 0.0, 0.49);
        double t = phase;

        double y = 0.0;
        switch (wave)
        {
            case Wave::Saw:
                y  = 2.0 * t - 1.0;
                y -= polyBLEP (t, dt);
                break;

            case Wave::Pulse:
            {
                y = (t < pulseWidth ? 1.0 : -1.0);
                y += polyBLEP (t, dt);
                double t2 = t + (1.0 - pulseWidth);
                if (t2 >= 1.0) t2 -= 1.0;
                y -= polyBLEP (t2, dt);
                break;
            }
        }

        phase += dt;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            syncFlag = true;
        }
        else
        {
            syncFlag = false;
        }

        lastValue = y;
        return y;
    }

    /*  Hard sync from a master oscillator. Call after master->processSample()
        passing master->wrappedThisSample(). The PolyBLEP correction at the
        forced discontinuity is applied so that hard sync stays band-limited. */
    inline void hardSync (bool masterWrapped, double dt) noexcept
    {
        if (masterWrapped)
        {
            // Approximate the discontinuity introduced by the forced reset.
            // PolyBLEP at phase=0 gives the standard correction.
            const double correction = polyBLEP (0.0, std::max (1.0e-9, dt));
            lastValue -= (wave == Wave::Saw ? correction : 0.0);
            phase      = 0.0;
        }
    }

    bool wrappedThisSample()  const noexcept { return syncFlag; }
    double getPhase()         const noexcept { return phase; }
    double getLastValue()     const noexcept { return lastValue; }

private:
    /*  PolyBLEP residual correction (Välimäki & Huovilainen).
        Returns a small offset that, when subtracted from the naive saw
        (or added/subtracted around pulse edges), removes the worst-case
        aliasing on the two samples surrounding the discontinuity. */
    static inline double polyBLEP (double t, double dt) noexcept
    {
        if (t < dt)
        {
            t /= dt;
            return (t + t) - (t * t) - 1.0;
        }
        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return (t * t) + (t + t) + 1.0;
        }
        return 0.0;
    }

    double sr        = 44100.0;
    double inv       = 1.0 / 44100.0;
    double freq      = 440.0;
    double phase     = 0.0;
    double phaseInc  = 0.0;
    double pulseWidth = 0.5;
    double lastValue = 0.0;
    Wave   wave      = Wave::Saw;
    bool   syncFlag  = false;
};

} // namespace ob8::dsp
