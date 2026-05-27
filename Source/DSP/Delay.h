#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

namespace ob8::dsp {

/*
    Stereo feedback delay.

    Two independent delay lines (L / R) with linear-interpolated reads so the
    delay time can be smoothly modulated. Each line has:
      - a feedback path with a one-pole low-pass ("damping") -- prevents the
        feedback signal from building up endless high-frequency hash
      - optional cross-feedback into the other channel for ping-pong patterns
      - separate per-channel time, so dotted-eighth + quarter etc. is trivial

    Caller is responsible for the dry/wet mix; processSample returns ONLY the
    wet (delayed + feedback) output.
*/
class StereoDelay
{
public:
    void prepare (double sampleRate, double maxDelaySeconds = 2.5)
    {
        sr = sampleRate;
        const int maxLen = std::max (16, static_cast<int> (sampleRate * maxDelaySeconds) + 4);
        bufL.assign (static_cast<size_t> (maxLen), 0.0f);
        bufR.assign (static_cast<size_t> (maxLen), 0.0f);
        writePos = 0;
        fbStateL = 0.0;
        fbStateR = 0.0;

        setTimeLSeconds (0.30);
        setTimeRSeconds (0.45);
        setFeedback     (0.4);
        setCrossFeedback (0.0);
        setDamping      (0.5);
    }

    void reset() noexcept
    {
        std::fill (bufL.begin(), bufL.end(), 0.0f);
        std::fill (bufR.begin(), bufR.end(), 0.0f);
        fbStateL = 0.0;
        fbStateR = 0.0;
    }

    void setTimeLSeconds (double s) noexcept
    {
        delayL = std::clamp (s * sr, 1.0, static_cast<double> (bufL.size()) - 2.0);
    }
    void setTimeRSeconds (double s) noexcept
    {
        delayR = std::clamp (s * sr, 1.0, static_cast<double> (bufR.size()) - 2.0);
    }

    /*  Feedback gain, [0, 0.95). Higher = longer tail; near 1.0 the system
        becomes unstable so we clamp short of unity. */
    void setFeedback (double f) noexcept
    {
        fb = std::clamp (f, 0.0, 0.95);
    }

    /*  Cross-feedback amount, [0, 1]. At 1.0 the L feedback goes entirely
        into R and vice-versa -> classic ping-pong. */
    void setCrossFeedback (double c) noexcept
    {
        cross = std::clamp (c, 0.0, 1.0);
    }

    /*  Damping, [0, 1]. Sets the corner of the low-pass on the feedback
        path. 0 = no damping (bright endless repeats), 1 = heavy damping
        (each repeat darker than the last). */
    void setDamping (double d) noexcept
    {
        const double clamped = std::clamp (d, 0.0, 1.0);
        // Map 0..1 to a corner from ~20 kHz down to ~500 Hz at the host SR.
        const double cornerHz = 20000.0 * std::pow (40.0 / 20000.0, clamped);
        // One-pole LP coefficient
        dampCoef = 1.0 - std::exp (-2.0 * 3.14159265358979323846 * cornerHz / sr);
    }

    /*  Reads one delayed-sample pair from the lines, computes feedback,
        writes back, and returns the WET stereo output. */
    inline void processSample (double inL, double inR,
                               double& outL, double& outR) noexcept
    {
        const double yL = readLinear (bufL, writePos, delayL);
        const double yR = readLinear (bufR, writePos, delayR);

        // Feedback path with one-pole low-pass damping per side
        fbStateL += dampCoef * (yL - fbStateL);
        fbStateR += dampCoef * (yR - fbStateR);

        const double fbL = fbStateL * (1.0 - cross) + fbStateR * cross;
        const double fbR = fbStateR * (1.0 - cross) + fbStateL * cross;

        // Write input + feedback back into the buffer
        bufL[static_cast<size_t> (writePos)] = static_cast<float> (inL + fb * fbL);
        bufR[static_cast<size_t> (writePos)] = static_cast<float> (inR + fb * fbR);

        if (++writePos >= static_cast<int> (bufL.size())) writePos = 0;

        outL = yL;
        outR = yR;
    }

private:
    static inline double readLinear (const std::vector<float>& buf,
                                     int writeIdx, double delaySamples) noexcept
    {
        const int sz   = static_cast<int> (buf.size());
        const double r = static_cast<double> (writeIdx) - delaySamples;
        const int  i0  = ((static_cast<int> (std::floor (r)) % sz) + sz) % sz;
        const int  i1  = (i0 + 1) % sz;
        const double frac = r - std::floor (r);
        return (1.0 - frac) * buf[static_cast<size_t> (i0)]
             +         frac  * buf[static_cast<size_t> (i1)];
    }

    double sr = 44100.0;
    std::vector<float> bufL, bufR;
    int    writePos = 0;
    double delayL   = 11025.0;
    double delayR   = 22050.0;
    double fb       = 0.4;
    double cross    = 0.0;
    double dampCoef = 0.1;
    double fbStateL = 0.0;
    double fbStateR = 0.0;
};

} // namespace ob8::dsp
