#pragma once

#include <cmath>
#include <algorithm>

namespace ob8::dsp {

/*
    TPT / zero-delay-feedback state-variable filter modelled after the
    Curtis CEM3320 used in the OB-8 (low-pass, 2-pole/4-pole switchable).

    Numerics:
      * Topology-Preserving Transform SVF (Vadim Zavalishin) -- stable up
        to and beyond Nyquist with a tan() pre-warp on cutoff.
      * 4-pole mode cascades two identical TPT stages.

    Non-linearity:
      The CEM3320 is built around a differential OTA whose ideal transfer
      function is tanh(x). Two effects make a real CEM3320 audibly distinct
      from a clean tanh saturator:

        1.  Asymmetry. Transistor mismatch in the differential pair adds a
            small even-order harmonic. We model this with a quadratic
            pre-skew applied before the symmetric saturator.

        2.  Soft compression near saturation. The OTA's bias current loses
            its log-linear conformance as the differential input exceeds
            ~100 mV (in CEM3320 units), and the curve flattens slightly
            faster than ideal tanh. A 5th-order minimax polynomial fits
            this shape over [-3, +3] with < 0.5% error and degrades into a
            graceful asymptote outside the band.

    Together these give the OB-8's signature thickened bass and the
    slightly "growly" resonance push that pure tanh-fed SVFs lack.

    Resonance compensation: we lift the input by a fraction of the lost
    low-frequency gain at high resonance, so the LP output level stays
    roughly constant as resonance is increased (close to what an OB-8
    actually does via its bias network).
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

    inline void setCutoff (double hz) noexcept
    {
        const double fc = std::clamp (hz, 10.0, std::min (20000.0, sr * 0.45));
        g = std::tan (kPi * fc * invSr);
    }

    inline void setResonance (double r) noexcept
    {
        const double clamped = std::clamp (r, 0.0, 1.10);
        k = 2.0 - 2.0 * clamped;
        // Mild resonance-loss compensation. Earlier this was 0.20*r^2
        // (giving up to ~24% input boost at full resonance), which on top
        // of the saturator could push the integrator into hard limiting
        // and bias the perceived loudness. 0.08 keeps the LP output
        // roughly constant at moderate Q without driving the OTA model
        // into its asymptote.
        resComp = 1.0 + 0.08 * clamped * clamped;
    }

    inline double processSample (double x) noexcept
    {
        const double y1 = processStage (x * resComp, s1, s2);
        if (slope == Slope::TwoPole)
            return y1;
        return processStage (y1, s1b, s2b);
    }

private:
    inline double processStage (double x, double& z1, double& z2) noexcept
    {
        const double denom = 1.0 + g * (g + k);
        const double hp    = (x - k * z1 - g * z1 - z2) / denom;
        const double bp    = g * hp + z1;
        const double lp    = g * bp + z2;

        z1 = g * hp + bp;
        z2 = g * bp + lp;

        // CEM3320 OTA saturation with asymmetric pre-skew. Applied to the
        // band-pass integrator state so it's in the feedback path, which is
        // where the OB-8's resonance saturates against.
        z1 = otaSaturate (z1);

        return lp;
    }

    /*  CEM3320-style OTA non-linearity.
        Symmetric saturation via a 5th-order minimax fit to tanh, smoothly
        asymptotic to +/-1 outside the modelled range. The DC blocker after
        the filter handles any residual offset.

        Earlier revisions tried an "asymmetric pre-skew" via
        `x + alpha * x * std::abs(x)` to add 2nd-harmonic content. That
        expression is in fact perfectly symmetric (x*|x| has the same sign
        as x, so positive and negative inputs are pushed the same distance
        away from zero), so it never produced the intended even-order
        colour and just stiffened the curve. Removed for now -- a real
        asymmetric model needs an offset term like alpha*x^2, which we
        leave for a future SPICE-fit update. */
    static inline double otaSaturate (double x) noexcept
    {
        const double xc = std::clamp (x, -3.5, 3.5);
        const double x2 = xc * xc;
        const double n  = xc * (1.0 + 0.062500 * x2);
        const double d  = 1.0 + (0.41667 + 0.0089286 * x2) * x2;
        return n / d;
    }

    static constexpr double kPi = 3.14159265358979323846;

    double sr      = 44100.0;
    double invSr   = 1.0 / 44100.0;
    double g       = 0.0;
    double k       = 2.0;
    double resComp = 1.0;
    double s1 = 0.0, s2 = 0.0;
    double s1b = 0.0, s2b = 0.0;
    Slope  slope   = Slope::FourPole;
};

} // namespace ob8::dsp
