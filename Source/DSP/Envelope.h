#pragma once

#include <cmath>
#include <algorithm>

namespace ob8::dsp {

/*
    Analog-style ADSR modelled after the CEM3310 used in the OB-8.

    The CEM3310 charges/discharges an RC network through a current source
    proportional to the control voltage. The resulting curves are exponentials
    (not linear) — which is what gives Oberheim envelopes their characteristic
    "snap". We use sample-by-sample one-pole approximations with calibrated
    time constants.

    Times are in seconds and follow the OB-8 panel: ~1 ms shortest, ~10 s
    longest. We pick the one-pole coefficient so that the envelope reaches
    99% of its target in the specified time (corresponds to ~4.6 time
    constants, which matches the CEM3310 datasheet behaviour).
*/
class Envelope
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
        setADSR (0.005, 0.150, 0.7, 0.250);
    }

    void reset() noexcept
    {
        stage  = Stage::Idle;
        value  = 0.0;
        target = 0.0;
        coef   = 0.0;
    }

    void setAttack  (double s) noexcept { aTime = std::max (0.0005, s); }
    void setDecay   (double s) noexcept { dTime = std::max (0.0005, s); }
    void setSustain (double v) noexcept { sLvl  = std::clamp (v, 0.0, 1.0); }
    void setRelease (double s) noexcept { rTime = std::max (0.0005, s); }

    void setADSR (double a, double d, double s, double r) noexcept
    {
        setAttack (a);
        setDecay  (d);
        setSustain (s);
        setRelease (r);
    }

    void noteOn() noexcept
    {
        stage  = Stage::Attack;
        // Attack on CEM3310 charges toward a level slightly above 1.0, which
        // is why Oberheim envelopes "snap" — they cross the threshold quickly
        // even with longer attack settings.
        target = 1.2;
        coef   = timeToCoef (aTime);
    }

    void noteOff() noexcept
    {
        if (stage == Stage::Idle) return;
        stage  = Stage::Release;
        target = 0.0;
        coef   = timeToCoef (rTime);
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }

    inline double processSample() noexcept
    {
        value += coef * (target - value);

        switch (stage)
        {
            case Stage::Attack:
                if (value >= 1.0)
                {
                    value  = 1.0;
                    stage  = Stage::Decay;
                    target = sLvl;
                    coef   = timeToCoef (dTime);
                }
                break;

            case Stage::Decay:
                if (std::abs (value - sLvl) < 1.0e-4)
                {
                    value  = sLvl;
                    stage  = Stage::Sustain;
                    target = sLvl;
                    coef   = 0.0;
                }
                break;

            case Stage::Sustain:
                // Track sustain level even if it's modulated externally
                target = sLvl;
                value  = sLvl;
                break;

            case Stage::Release:
                if (value < 1.0e-5)
                {
                    value  = 0.0;
                    stage  = Stage::Idle;
                    target = 0.0;
                }
                break;

            case Stage::Idle:
                value  = 0.0;
                break;
        }

        return value;
    }

private:
    inline double timeToCoef (double seconds) const noexcept
    {
        // Pick coefficient so that a one-pole reaches ~99% in `seconds`.
        // value' = value + coef * (target - value); for target=1, value=0,
        // value(n) = 1 - (1-coef)^n; solve for n where value=0.99 -> n ~ 4.6/coef
        const double n = std::max (1.0, seconds * sr);
        return 1.0 - std::exp (-4.605 / n);   // ln(100)
    }

    double sr     = 44100.0;
    double value  = 0.0;
    double target = 0.0;
    double coef   = 0.0;
    double aTime  = 0.005;
    double dTime  = 0.150;
    double sLvl   = 0.7;
    double rTime  = 0.250;
    Stage  stage  = Stage::Idle;
};

} // namespace ob8::dsp
