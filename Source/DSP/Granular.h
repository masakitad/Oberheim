#pragma once

#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

namespace ob8::dsp {

/*
    Granular delay.

    Captures the dry stereo output into a circular buffer (typically a few
    seconds long), then continuously spawns short overlapping grains that
    read from random positions in that buffer. Each grain has its own:
        - read position (set when spawned; can wander forward or backward)
        - pitch (playback speed)
        - hann-window envelope (avoids click on grain start/end)
        - stereo pan
    Many grains run simultaneously (up to kMaxGrains) so the output is a
    cloud of overlapping fragments of the recent dry signal.

    Add the wet result on top of the dry. Feedback re-injects the wet
    sample into the recording buffer with a tap-band low-pass so the
    cloud builds smoothly rather than going harsh.

    Tunable parameters:
        grain length    : 10..500 ms
        density         : 1..50 grains/sec   (rate of spawning)
        scatter         : 0..1               (random jitter on read pos)
        pitch           : -12..+12 semitones (base pitch ratio)
        spread          : 0..1               (stereo width)
        feedback        : 0..0.9             (re-record wet into buffer)
        mix             : 0..1               (dry + mix * wet)
*/
class GranularDelay
{
public:
    static constexpr int kMaxGrains = 24;

    void prepare (double sampleRate, double maxBufferSeconds = 4.0)
    {
        sr     = sampleRate;
        invSr  = 1.0 / sr;

        const int len = std::max (16, static_cast<int> (sr * maxBufferSeconds) + 4);
        bufL.assign (static_cast<size_t> (len), 0.0f);
        bufR.assign (static_cast<size_t> (len), 0.0f);
        writePos = 0;

        for (auto& gr : grains) gr.active = false;
        spawnCountdown = 0.0;
        feedbackStateL = feedbackStateR = 0.0;

        setGrainMs        (140.0);
        setDensityHz      (12.0);
        setScatter        (0.5);
        setPitchSemis     (0.0);
        setSpread         (0.7);
        setFeedback       (0.30);
    }

    void reset()
    {
        std::fill (bufL.begin(), bufL.end(), 0.0f);
        std::fill (bufR.begin(), bufR.end(), 0.0f);
        for (auto& gr : grains) gr.active = false;
        feedbackStateL = feedbackStateR = 0.0;
        spawnCountdown = 0.0;
    }

    // --- setters -------------------------------------------------------
    void setGrainMs    (double ms) noexcept
    {
        grainSamples = std::clamp (static_cast<int> (ms * 0.001 * sr), 32, kMaxGrainSamples);
    }
    void setDensityHz  (double hz) noexcept
    {
        spawnInterval = sr / std::max (0.5, hz);
    }
    void setScatter    (double s)  noexcept { scatter01 = std::clamp (s, 0.0, 1.0); }
    void setPitchSemis (double st) noexcept
    {
        // 2^(st/12); clamp to a safe range
        pitchRatio = std::pow (2.0, std::clamp (st, -24.0, 24.0) / 12.0);
    }
    void setSpread     (double s)  noexcept { spread01 = std::clamp (s, 0.0, 1.0); }
    void setFeedback   (double f)  noexcept { feedback = std::clamp (f, 0.0, 0.90); }

    /*  Process one stereo sample. Writes dry to the buffer, runs the
        grain cloud, and returns the WET output only (caller blends with
        dry via Mix knob). */
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

        for (auto& gr : grains)
        {
            if (! gr.active) continue;

            const double pos = gr.position;
            const int    i0  = (static_cast<int> (std::floor (pos)) % bufLen + bufLen) % bufLen;
            const int    i1  = (i0 + 1) % bufLen;
            const double frac = pos - std::floor (pos);
            const double sL = (1.0 - frac) * bufL[static_cast<size_t> (i0)]
                                    + frac * bufL[static_cast<size_t> (i1)];
            const double sR = (1.0 - frac) * bufR[static_cast<size_t> (i0)]
                                    + frac * bufR[static_cast<size_t> (i1)];

            // Hann window over the grain lifetime
            const double w = 0.5 * (1.0 - std::cos (
                static_cast<double> (gr.samplesElapsed) /
                static_cast<double> (gr.grainLength) * kTwoPi));

            wetL += sL * w * gr.panL;
            wetR += sR * w * gr.panR;

            gr.position += gr.increment;
            if (++gr.samplesElapsed >= gr.grainLength)
                gr.active = false;
        }

        // 3. Write the dry signal + feedback of the WET back into the buffer.
        //    A one-pole low-pass on the feedback path keeps repeated passes
        //    from going harsh.
        const double dampCoef = 0.20;
        feedbackStateL += dampCoef * (wetL - feedbackStateL);
        feedbackStateR += dampCoef * (wetR - feedbackStateR);

        bufL[static_cast<size_t> (writePos)] = static_cast<float> (
            inL + feedback * feedbackStateL);
        bufR[static_cast<size_t> (writePos)] = static_cast<float> (
            inR + feedback * feedbackStateR);
        if (++writePos >= bufLen) writePos = 0;

        outL = wetL;
        outR = wetR;
    }

private:
    static constexpr double kTwoPi = 6.283185307179586;
    static constexpr int    kMaxGrainSamples = 96000;   // ~2 s @ 48 kHz; enough headroom

    struct Grain
    {
        bool   active = false;
        double position = 0.0;     // fractional read position into the buffer
        double increment = 1.0;    // pitch ratio (sample step per audio sample)
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
        if (slot == nullptr) return;   // all 24 grains in flight; skip

        // Random helpers
        std::uniform_real_distribution<double> u01 (0.0, 1.0);
        const double r1 = u01 (rng);
        const double r2 = u01 (rng);
        const double r3 = u01 (rng);

        // Read position lives roughly "(grainSamples * 1.5) + scatter*1s"
        // samples behind the write head -- gives a familiar grain-cloud feel
        // without ever overlapping the writer.
        const double meanLatency = static_cast<double> (grainSamples) * 1.5;
        const double maxScatter  = sr * 1.0 * scatter01;
        const double readOffset  = meanLatency + r1 * maxScatter;
        const int    bufLen      = static_cast<int> (bufL.size());

        slot->active        = true;
        slot->grainLength   = grainSamples;
        slot->samplesElapsed= 0;
        slot->increment     = pitchRatio;
        slot->position      = std::fmod (
            static_cast<double> (writePos) - readOffset + bufLen, bufLen);

        // Equal-power stereo pan, deviating from centre by spread amount.
        const double pan = (r2 - 0.5) * 2.0 * spread01;   // -spread .. +spread
        const double theta = (0.5 + pan * 0.5) * (kTwoPi * 0.25);
        slot->panL = std::cos (theta) * 1.4;   // light boost to compensate window loss
        slot->panR = std::sin (theta) * 1.4;

        // Occasionally tweak pitch slightly to spread harmonics
        if (scatter01 > 0.0)
        {
            const double cents = (r3 - 0.5) * 2.0 * scatter01 * 50.0;   // +/- 25 cents max
            slot->increment *= std::pow (2.0, cents / 1200.0);
        }
    }

    double sr      = 44100.0;
    double invSr   = 1.0 / 44100.0;
    std::vector<float> bufL, bufR;
    int    writePos = 0;

    std::array<Grain, kMaxGrains> grains{};

    double spawnCountdown = 0.0;
    double spawnInterval  = 4000.0;   // samples between spawns

    int    grainSamples = 6000;       // ~140 ms @ 44 kHz
    double scatter01    = 0.5;
    double pitchRatio   = 1.0;
    double spread01     = 0.7;
    double feedback     = 0.30;

    double feedbackStateL = 0.0, feedbackStateR = 0.0;

    std::mt19937 rng { 0xC0FFEEu };
};

} // namespace ob8::dsp
