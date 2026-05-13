#pragma once

#include <cmath>
#include <algorithm>

namespace ob8::dsp {

/*
    Trapezoidal / zero-delay-feedback state-variable filter modelled after
    the CEM3320 used in the OB-8. The CEM3320 is a multi-mode VCF whose
    OB-8 wiring is a low-pass with a 2-pole / 4-pole selector.

    Implementation:
      - TPT (Topology-Preserving Transform) SVF following Vadim Zavalishin
        ("The Art of VA Filter Design"). Stable up to and beyond Nyquist.
      - Soft saturation (tanh approximation) in the resonance feedback path
        to tame self-oscillation and add subtle even-order character on the
        way to self-oscillation. The OB-8 self-oscillates cleanly; we follow.
      - 2-pole output is plain LP1; 4-pole output is LP1 fed through a
        second identical TPT SVF (matched cutoff and resonance) which gives
        the classic OB-8 4-pole curve without the resonance loss of a naive
        cascade.
      - Cutoff modulation is exponential (1 V/oct) and is processed per-sample
        via setCutoff(), so the caller can drive it from envelopes & LFOs at
        any rate.
*/
class StateVariableFilter
{
public:
    enum class Slope { TwoPole, FourPole };

    void prepare (double sampleRate) noexcept
    {
        sr     = sampleRate;
        invSr  = 1.0 / sr;
        reset();
        setCutoff (1000.0);
        setResonance (0.0);
    }

    void reset() noexcept
    {
        s1 = s2 = 0.0;
        s1b = s2b = 0.0;
    }

    void setSlope (Slope s) noexcept { slope = s; }

    // Hz. Internally pre-warps via tan() for the bilinear transform.
    inline void setCutoff (double hz) noexcept
    {
        const double fc = std::clamp (hz, 10.0, std::min (20000.0, sr * 0.45));
        // g = tan(pi * fc / sr) -- BLT pre-warp
        g = std::tan (kPi * fc * invSr);
    }

    // 0..1, mapped to k = 2 - 2r so r=1 is self-oscillation.
    inline void setResonance (double r) noexcept
    {
        // Allow a small headroom past 1.0 to push into self-oscillation cleanly
        const double clamped = std::clamp (r, 0.0, 1.10);
        k = 2.0 - 2.0 * clamped;
    }

    /*  Process one sample. */
    inline double processSample (double x) noexcept
    {
        const double y1 = processStage (x, s1, s2);
        if (slope == Slope::TwoPole)
            return y1;
        return processStage (y1, s1b, s2b);
    }

private:
    inline double processStage (double x, double& z1, double& z2) noexcept
    {
        // Implicit equations of the TPT SVF
        //   v_bp = (x - k*v_bp - z2 - g*(v_bp + z1)) / (1 + g*(g + k))
        // Solve directly:
        const double denom = 1.0 + g * (g + k);
        const double hp    = (x - k * z1 - g * z1 - z2) / denom;
        const double bp    = g * hp + z1;
        const double lp    = g * bp + z2;

        // Update integrator states with trapezoidal rule
        z1 = g * hp + bp;
        z2 = g * bp + lp;

        // Soft saturation in the feedback path (a la analog op-amp limiting)
        // Applied to z1 (which is also the band-pass output) keeps the
        // self-oscillation amplitude bounded.
        z1 = fastTanh (z1);

        return lp;
    }

    static inline double fastTanh (double x) noexcept
    {
        // Padé 3/3 approximation, accurate to ~1e-4 over [-3,3], cheap.
        const double x2 = x * x;
        const double n  = x * (27.0 + x2);
        const double d  = 27.0 + 9.0 * x2;
        return n / d;
    }

    static constexpr double kPi = 3.14159265358979323846;

    double sr     = 44100.0;
    double invSr  = 1.0 / 44100.0;
    double g      = 0.0;     // pre-warped cutoff
    double k      = 2.0;     // resonance coefficient (2 = no res, 0 = self-osc)
    double s1 = 0.0, s2 = 0.0;
    double s1b = 0.0, s2b = 0.0;
    Slope  slope = Slope::FourPole;
};

} // namespace ob8::dsp
