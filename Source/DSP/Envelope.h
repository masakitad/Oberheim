#pragma once

#include <cmath>
#include <algorithm>

namespace ob8::dsp {

/*
    Analog ADSR modelled on the Curtis CEM3310.

    CEM3310 physics, briefly:
      * Each stage drives a 0.01 uF cap through an exponential current source
        whose magnitude is set by a control voltage. The result is a true RC
        exponential charge/discharge.
      * The Attack stage charges toward roughly +5 V; the chip compares to a
        ~+4 V threshold and clamps the output at 1.0 at that point. The ratio
        of "target" to "trigger threshold" defines the famous Oberheim snap:
        the envelope crosses 1.0 well before its asymptote, so even long
        attacks rise quickly at first and then would saturate -- but instead
        get clamped and move on to Decay.
      * Decay and Release are standard RC discharges toward the sustain level
        and zero respectively.

    We mirror this exactly:
      Attack:  target = kAttackTarget (~ 1.5), threshold = 1.0
               coef chosen so the one-pole reaches 1.0 in aTime seconds.
      Decay:   target = sustain,   coef chosen for 99% completion in dTime.
      Release: target = 0,         coef chosen for 99% completion in rTime.

    Coefficients are recomputed whenever the corresponding ADSR knob moves
    OR the stage transitions, so live knob tweaks affect the ongoing stage --
    which matches the real analog behaviour (potentiometers always set the
    current source, regardless of stage).
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

    void setAttack  (double s) noexcept
    {
        aTime = std::max (0.0003, s);
        if (stage == Stage::Attack)  coef = attackCoef();
    }
    void setDecay   (double s) noexcept
    {
        dTime = std::max (0.0005, s);
        if (stage == Stage::Decay)   coef = ninetyNineCoef (dTime);
    }
    void setSustain (double v) noexcept
    {
        sLvl  = std::clamp (v, 0.0, 1.0);
        if (stage == Stage::Decay)   target = sLvl;
        if (stage == Stage::Sustain) value  = sLvl;
    }
    void setRelease (double s) noexcept
    {
        rTime = std::max (0.0005, s);
        if (stage == Stage::Release) coef = ninetyNineCoef (rTime);
    }

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
        target = kAttackTarget;
        coef   = attackCoef();
    }

    void noteOff() noexcept
    {
        if (stage == Stage::Idle) return;
        stage  = Stage::Release;
        target = 0.0;
        coef   = ninetyNineCoef (rTime);
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    Stage getStage() const noexcept { return stage; }

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
                    coef   = ninetyNineCoef (dTime);
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
                value = sLvl;
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
                value = 0.0;
                break;
        }

        return value;
    }

private:
    /*  Reach the trigger threshold (1.0) in aTime seconds when charging
        toward kAttackTarget. Solve: 1.0 = T*(1 - (1-coef)^n) for coef. */
    inline double attackCoef() const noexcept
    {
        const double n     = std::max (1.0, aTime * sr);
        const double ratio = kAttackTarget / (kAttackTarget - 1.0);   // T/(T-1)
        return 1.0 - std::exp (-std::log (ratio) / n);
    }

    /*  Coefficient so a one-pole reaches 99% of its target in `seconds`. */
    inline double ninetyNineCoef (double seconds) const noexcept
    {
        const double n = std::max (1.0, seconds * sr);
        return 1.0 - std::exp (-4.605 / n);
    }

    static constexpr double kAttackTarget = 1.5;  // CEM3310-style overshoot

    double sr     = 44100.0;
    double value  = 0.0;
    double target = 0.0;
    double coef   = 0.0;
    double aTime  = 0.005;
    double dTime  = 0.250;
    double sLvl   = 0.7;
    double rTime  = 0.250;
    Stage  stage  = Stage::Idle;
};

} // namespace ob8::dsp
