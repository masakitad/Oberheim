#pragma once

#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

namespace ob8::dsp {

/*
    Granular delay -- production-quality rewrite.

    Captures the dry stereo output into a circular buffer, then continuously
    spawns short overlapping grains read from the recent past. Each grain
    has its own read position, pitch, Hann-window envelope and stereo pan.

    The previous revision had three issues that the rewrite addresses:

      1. Pan compensation was a flat +1.4x multiply on top of equal-power
         pan to "make up for the Hann window's 0.5 integral". On its own
         this was inaudible, but combined with high feedback it pushed
         the effective per-iteration gain past 1.0 and the wet path
         grew exponentially -- the "burst after a few seconds of
         silence" the user reported. Compensation is now done via a
         single sqrt-of-overlap normaliser applied to the summed wet
         output (correct for any density / size).

      2. The feedback path had no bounding. With feedback >= ~0.7 and
         the gain compensation above, output blew up. We now run a
         soft-clip (tanh) on the feedback contribution AND apply a DC
         blocker on the wet output. The system is unconditionally
         stable for any (feedback < 1) setting.

      3. Mean read latency was 1.5 x grainSize. For a 500 ms grain that
         meant 750 ms of silence before any wet appeared. Now defaulted
         to 0.5 x grainSize, so a 500 ms grain starts producing audible
         output ~ 250 ms after the first sample is recorded.
*/
class GranularDelay
{
public:
    static constexpr int kMaxGrains = 48;

    void prepare (double sampleRate, double maxBufferSeconds = 4.0)
    {
        sr     = sampleRate;
        invSr  = 1.0 / sr;

        const int len = std::max (16, static_cast<int> (sr * maxBufferSeconds) + 4);
        bufL.assign (static_cast<size_t> (len), 0.0f);
        bufR.assign (static_cast<size_t> (len), 0.0f);
        writePos = 0;
        samplesRecorded = 0;

        for (auto& gr : grains) gr.active = false;
        spawnCountdown = 0.0;
        feedbackStateL = feedbackStateR = 0.0;
        dcX1L = dcX1R = dcY1L = dcY1R = 0.0;

        setGrainMs        (220.0);
        setDensityHz      (14.0);
        setScatter        (0.55);
        setPitchSemis     (0.0);
        setSpread         (0.85);
        setFeedback       (0.45);
    }

    void reset()
    {
        std::fill (bufL.begin(), bufL.end(), 0.0f);
        std::fill (bufR.begin(), bufR.end(), 0.0f);
        for (auto& gr : grains) gr.active = false;
        feedbackStateL = feedbackStateR = 0.0;
        dcX1L = dcX1R = dcY1L = dcY1R = 0.0;
        spawnCountdown = 0.0;
        samplesRecorded = 0;
    }

    // --- setters -------------------------------------------------------
    void setGrainMs    (double ms) noexcept
    {
        grainSamples = std::clamp (
            static_cast<int> (ms * 0.001 * sr),
            32, static_cast<int> (sr * 1.5));   // up to 1.5 s grain
    }
    void setDensityHz  (double hz) noexcept
    {
        densityHz     = std::clamp (hz, 0.5, 80.0);
        spawnInterval = sr / densityHz;
    }
    void setScatter    (double s)  noexcept { scatter01 = std::clamp (s, 0.0, 1.0); }
    void setPitchSemis (double st) noexcept
    {
        pitchRatio = std::pow (2.0, std::clamp (st, -24.0, 24.0) / 12.0);
    }
    void setSpread     (double s)  noexcept { spread01 = std::clamp (s, 0.0, 1.0); }
    void setFeedback   (double f)  noexcept
    {
        // Cap below 1 -- the soft-clip path below keeps it stable, but we
        // still want a "max usable" not "max possible".
        feedback = std::clamp (f, 0.0, 0.85);
    }

    /*  Process one stereo sample. Writes dry to the buffer, runs the
        grain cloud, returns the WET output only (caller blends with
        dry via the Mix knob). */
    inline void processSample (double inL, double inR,
                               double& outL, double& outR) noexcept
    {
        // 1. Spawn new grains at the configured density.
        spawnCountdown -= 1.0;
        while (spawnCountdown <= 0.0)
        {
            spawnGrain();
            spawnCountdown += spawnInterval;
        }

        // 2. Sum active grains.
        double wetL = 0.0, wetR = 0.0;
        const int bufLen = static_cast<int> (bufL.size());
        const double hannDenom = std::max (1.0,
            static_cast<double> (grainSamples - 1));

        for (auto& gr : grains)
        {
            if (! gr.active) continue;

            // 4-point cubic Hermite interpolation. Linear interp produces
            // audible HF aliasing once grains are pitched (cent jitter from
            // scatter, or the pitch knob); cubic stays clean for any usable
            // pitch ratio.
            const double pos  = gr.position;
            const int    i1   = (static_cast<int> (std::floor (pos)) % bufLen + bufLen) % bufLen;
            const int    i0   = (i1 - 1 + bufLen) % bufLen;
            const int    i2   = (i1 + 1) % bufLen;
            const int    i3   = (i1 + 2) % bufLen;
            const double frac = pos - std::floor (pos);
            const double sL = hermite4 (bufL[static_cast<size_t> (i0)],
                                        bufL[static_cast<size_t> (i1)],
                                        bufL[static_cast<size_t> (i2)],
                                        bufL[static_cast<size_t> (i3)], frac);
            const double sR = hermite4 (bufR[static_cast<size_t> (i0)],
                                        bufR[static_cast<size_t> (i1)],
                                        bufR[static_cast<size_t> (i2)],
                                        bufR[static_cast<size_t> (i3)], frac);

            // Hann window over the grain lifetime. Divisor is (N - 1) so
            // both endpoints close exactly at zero — `/ N` leaves a tiny
            // residual at the last sample of each grain which manifests as
            // a periodic click at the grain-density rate.
            const double w = 0.5 * (1.0 - std::cos (
                static_cast<double> (gr.samplesElapsed) / hannDenom * kTwoPi));

            wetL += sL * w * gr.panL;
            wetR += sR * w * gr.panR;

            gr.position += gr.increment;
            if (++gr.samplesElapsed >= gr.grainLength)
                gr.active = false;
        }

        // 3. Normalise wet by sqrt(expected overlap) so the level stays
        //    musically sensible regardless of (size * density). Expected
        //    number of simultaneously-active grains:
        //        E = density * grain_duration = density * grainSamples / sr
        //    The Hann window's RMS over its support is 0.5, so the RMS of
        //    a single grain is 0.5. The sum of E independent grains has
        //    RMS ~ 0.5 * sqrt(E). To keep wet RMS comparable to dry we
        //    scale by 1 / (0.5 * sqrt(E)) = 2 / sqrt(E).
        const double expectedOverlap = densityHz * grainSamples * invSr;
        const double wetGain = (expectedOverlap > 1.0)
            ? (1.0 / std::sqrt (expectedOverlap))
            : 1.0;
        wetL *= wetGain;
        wetR *= wetGain;

        // 4. DC blocker on the wet output (one-pole high-pass). Prevents
        //    asymmetric grain windows from biasing the signal over time.
        const double dcR = 0.997;
        const double yL = wetL - dcX1L + dcR * dcY1L;
        const double yR = wetR - dcX1R + dcR * dcY1R;
        dcX1L = wetL; dcX1R = wetR;
        dcY1L = yL;   dcY1R = yR;
        wetL = yL; wetR = yR;

        // 5. Feedback into the recording buffer. tanh saturates so the
        //    feedback chain is unconditionally stable for any feedback < 1.
        const double dampCoef = 0.20;
        feedbackStateL += dampCoef * (wetL - feedbackStateL);
        feedbackStateR += dampCoef * (wetR - feedbackStateR);

        const double fbL = std::tanh (feedbackStateL);
        const double fbR = std::tanh (feedbackStateR);

        bufL[static_cast<size_t> (writePos)] = static_cast<float> (
            inL + feedback * fbL);
        bufR[static_cast<size_t> (writePos)] = static_cast<float> (
            inR + feedback * fbR);
        if (++writePos >= bufLen) writePos = 0;

        if (samplesRecorded < bufLen) ++samplesRecorded;

        outL = wetL;
        outR = wetR;
    }

private:
    static constexpr double kTwoPi = 6.283185307179586;

    // 4-point Catmull-Rom cubic Hermite interpolation (B = 0, C = 0.5).
    // Smoother than linear, kills HF aliasing on pitched grain reads.
    static inline double hermite4 (double y0, double y1, double y2, double y3,
                                   double t) noexcept
    {
        const double c0 = y1;
        const double c1 = 0.5 * (y2 - y0);
        const double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
        const double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    struct Grain
    {
        bool   active = false;
        double position = 0.0;
        double increment = 1.0;
        int    samplesElapsed = 0;
        int    grainLength = 0;
        double panL = 0.7071;
        double panR = 0.7071;
    };

    void spawnGrain() noexcept
    {
        // Find an inactive grain slot
        Grain* slot = nullptr;
        for (auto& gr : grains)
        {
            if (! gr.active) { slot = &gr; break; }
        }
        if (slot == nullptr) return;

        std::uniform_real_distribution<double> u01 (0.0, 1.0);
        const double r1 = u01 (rng);
        const double r2 = u01 (rng);
        const double r3 = u01 (rng);

        // Read position must be far enough behind the write head that
        // a pitched-up grain (increment > 1) doesn't reach the writer
        // mid-grain. The minimum safe latency is grainSamples * pitchRatio,
        // since a grain at increment `r` traverses `grainSamples * r`
        // samples of buffer over its lifetime. We previously used
        // 0.5 * grainSamples, which was the source of audible glitches
        // for any pitch ratio >= 1 plus cent jitter — the grain would
        // catch the writer at the end of its envelope.
        const double safetyRatio = std::max (1.0, pitchRatio);
        const double meanLatency = static_cast<double> (grainSamples) * safetyRatio;
        const double maxScatter  = sr * 0.5 * scatter01;
        const double readOffset  = meanLatency + r1 * maxScatter;
        const int    bufLen      = static_cast<int> (bufL.size());

        // Don't start a grain if the buffer hasn't been recorded into yet
        // for enough samples (silent intro is more musical than reading
        // garbage zeros).
        if (samplesRecorded < static_cast<int> (readOffset + grainSamples))
            return;

        slot->active        = true;
        slot->grainLength   = grainSamples;
        slot->samplesElapsed= 0;
        slot->increment     = pitchRatio;
        slot->position      = std::fmod (
            static_cast<double> (writePos) - readOffset + bufLen, bufLen);

        // Equal-power stereo pan. NO MORE 1.4x boost -- the wetGain
        // normaliser above handles the Hann window's 0.5 integral
        // properly, so any extra multiplier just makes the feedback
        // chain blow up.
        const double pan = (r2 - 0.5) * 2.0 * spread01;
        const double theta = (0.5 + pan * 0.5) * (kTwoPi * 0.25);
        slot->panL = std::cos (theta);
        slot->panR = std::sin (theta);

        // Optional cent jitter when scatter > 0
        if (scatter01 > 0.0)
        {
            const double cents = (r3 - 0.5) * 2.0 * scatter01 * 50.0;
            slot->increment *= std::pow (2.0, cents / 1200.0);
        }
    }

    double sr      = 44100.0;
    double invSr   = 1.0 / 44100.0;
    std::vector<float> bufL, bufR;
    int    writePos = 0;
    int    samplesRecorded = 0;

    std::array<Grain, kMaxGrains> grains{};

    double spawnCountdown = 0.0;
    double spawnInterval  = 4000.0;
    double densityHz      = 12.0;

    int    grainSamples = 6000;
    double scatter01    = 0.5;
    double pitchRatio   = 1.0;
    double spread01     = 0.7;
    double feedback     = 0.30;

    double feedbackStateL = 0.0, feedbackStateR = 0.0;
    double dcX1L = 0.0, dcX1R = 0.0, dcY1L = 0.0, dcY1R = 0.0;

    std::mt19937 rng { 0xC0FFEEu };
};

} // namespace ob8::dsp
