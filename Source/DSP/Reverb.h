#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <random>
#include <algorithm>

namespace ob8::dsp {

/*
    8-line Feedback Delay Network reverb (Jot, "Digital Delay Networks for
    Designing Artificial Reverberators", 1991), tuned for a warm,
    plate-like decay rather than a giant hall.

    Why 8 lines?
      - 4 lines is the minimum for a believable diffusion field; output
        sounds a bit comb-y on staccato hits.
      - 8 lines gives a much denser modal distribution (8! = 40320 unique
        echo paths after the first few iterations) for the same CPU cost as
        a typical Schroeder reverb.

    Key choices:
      - Prime-millisecond delay lengths so the comb resonances of the
        individual lines don't reinforce.
      - Walsh-Hadamard (butterfly) mixing matrix between iterations. It's
        the cheapest possible unitary mix (no multiplies, only +/-) and
        keeps energy preserved -- so the decay is set purely by the gain
        coefficient, not accidentally by the matrix.
      - Per-line one-pole low-pass in the feedback path ("damping") so the
        high frequencies decay faster than the lows, like a real room or a
        plate. Coupled to a single "damping" knob.
      - Per-line slow random modulation on the read pointer (-/+ 1.5 ms at
        ~0.3 Hz) so the modal resonances drift; without this an FDN sounds
        metallic on sustained notes.
      - Optional pre-delay (mono, 0..200 ms) feeding the inputs.

    The interface is stereo in, stereo out, wet only. Caller does dry/wet
    mix downstream.
*/
class FDNReverb
{
public:
    static constexpr int kLines = 8;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        invSr = 1.0 / sr;

        // Prime-ms delay lengths, scaled to the current sample rate. These
        // span a 16..51 ms range which gives a tight, plate-style decay.
        // Multiplying all of them by `size` (0.5..1.5) lets the user grow
        // or shrink the apparent room.
        const double baseMs[kLines] = {
            15.531, 20.819, 24.871, 29.401,
            35.413, 40.787, 46.039, 50.713
        };

        const int maxLen = static_cast<int> (sr * 0.150) + 8; // size up to 1.5x of 51 ms ~ 76 ms; pad
        for (int i = 0; i < kLines; ++i)
        {
            lines[i].buffer.assign (static_cast<size_t> (maxLen), 0.0f);
            lines[i].writePos    = 0;
            lines[i].baseDelay   = baseMs[i] * 0.001 * sr;
            lines[i].dampState   = 0.0;
            lines[i].modPhase    = static_cast<double> (i) / kLines; // spread
            // Per-line modulation rate (0.10..0.40 Hz); randomly chosen but
            // deterministic so two instances sound identical.
            lines[i].modRateHz   = 0.10 + 0.04 * i;
        }

        // Pre-delay line (mono, max 300 ms)
        preLen = static_cast<int> (sr * 0.300) + 4;
        preBuf.assign (static_cast<size_t> (preLen), 0.0f);
        preWrite = 0;

        setSize (1.0);
        setDecay (0.65);
        setDamping (0.4);
        setPreDelaySeconds (0.012);
        setModulationDepth (0.001);   // ~1 ms at full
        setWidth (1.0);
    }

    void reset() noexcept
    {
        for (auto& l : lines)
        {
            std::fill (l.buffer.begin(), l.buffer.end(), 0.0f);
            l.dampState = 0.0;
        }
        std::fill (preBuf.begin(), preBuf.end(), 0.0f);
        preWrite = 0;
    }

    /*  Apparent room size, [0.5, 1.5]. Multiplies the delay lengths. */
    void setSize (double s) noexcept
    {
        sizeMul = std::clamp (s, 0.5, 1.5);
    }

    /*  Decay time control, [0, 1]. Mapped to a feedback gain that yields
        ~RT60 from a few hundred ms (at 0) to several seconds (at 1). */
    void setDecay (double d) noexcept
    {
        const double clamped = std::clamp (d, 0.0, 1.0);
        // Empirical mapping. Per-iteration gain g, where the average delay
        // is ~35 ms -- so iteration takes 35 ms and RT60 ~ 3 * 35ms / -log(g).
        fbGain = 0.65 + 0.32 * clamped;     // 0.65 .. 0.97
    }

    /*  Damping, [0, 1]. Higher values darken the tail more aggressively. */
    void setDamping (double d) noexcept
    {
        const double clamped = std::clamp (d, 0.0, 1.0);
        const double cornerHz = 18000.0 * std::pow (200.0 / 18000.0, clamped);
        dampCoef = 1.0 - std::exp (-2.0 * 3.14159265358979323846 * cornerHz * invSr);
    }

    /*  Pre-delay before the input hits the FDN core. */
    void setPreDelaySeconds (double s) noexcept
    {
        preDelay = std::clamp (s * sr, 0.0, static_cast<double> (preLen) - 2.0);
    }

    /*  Modulation depth in seconds (peak deviation of the per-line read
        pointer). 1..2 ms is musically pleasant. */
    void setModulationDepth (double secs) noexcept
    {
        modDepthSamples = std::clamp (secs * sr, 0.0, sr * 0.005);
    }

    /*  Stereo width, [0, 1]. 0 = mono out, 1 = full L/R separation. */
    void setWidth (double w) noexcept
    {
        width = std::clamp (w, 0.0, 1.0);
    }

    inline void processSample (double inL, double inR,
                               double& outL, double& outR) noexcept
    {
        // Sum to mono into pre-delay (more typical for a "stage" feel; the
        // FDN restores stereo via its output mix)
        const double in = (inL + inR) * 0.5;

        // -- Pre-delay -----------------------------------------------------
        preBuf[static_cast<size_t> (preWrite)] = static_cast<float> (in);
        const int preReadPos = ((preWrite - static_cast<int> (preDelay)) % preLen + preLen) % preLen;
        const double preOut = preBuf[static_cast<size_t> (preReadPos)];
        if (++preWrite >= preLen) preWrite = 0;

        // -- Read taps -----------------------------------------------------
        std::array<double, kLines> y{};
        for (int i = 0; i < kLines; ++i)
        {
            auto& l = lines[i];

            // Slow modulation of the delay tap (sin LFO -> +/- modDepthSamples)
            l.modPhase += l.modRateHz * invSr;
            if (l.modPhase >= 1.0) l.modPhase -= 1.0;
            const double modOffset = modDepthSamples
                * std::sin (l.modPhase * 6.283185307179586);

            const double delayLen = l.baseDelay * sizeMul + modOffset;
            y[i] = readLinear (l.buffer, l.writePos, delayLen);
        }

        // -- Damping (per-line one-pole LP on the read signal) --
        for (int i = 0; i < kLines; ++i)
        {
            auto& l = lines[i];
            l.dampState += dampCoef * (y[i] - l.dampState);
            y[i] = l.dampState;
        }

        // -- Walsh-Hadamard mixing (in-place butterfly, log2(8) = 3 passes) -
        hadamard8 (y.data());
        const double norm = 1.0 / std::sqrt (static_cast<double> (kLines));
        for (auto& v : y) v *= norm;

        // -- Write back: input + scaled feedback ----
        // Spread the pre-delayed input across all 8 lines so excitation is
        // dense from sample 0 onwards.
        const double drive = preOut * 0.7071; // -3 dB to avoid hot inputs
        for (int i = 0; i < kLines; ++i)
        {
            auto& l = lines[i];
            l.buffer[static_cast<size_t> (l.writePos)] =
                static_cast<float> (drive + fbGain * y[i]);
            if (++l.writePos >= static_cast<int> (l.buffer.size())) l.writePos = 0;
        }

        // -- Stereo output: opposite-phase combinations of the lines for
        //    natural L/R decorrelation; mix with width. ----
        const double left  = (y[0] + y[2] - y[5] - y[7]) * 0.5;
        const double right = (y[1] + y[3] - y[4] - y[6]) * 0.5;
        const double mono  = (left + right) * 0.5;

        outL = mono + width * (left  - mono);
        outR = mono + width * (right - mono);
    }

private:
    struct Line
    {
        std::vector<float> buffer;
        int writePos      = 0;
        double baseDelay  = 0.0;
        double dampState  = 0.0;
        double modPhase   = 0.0;
        double modRateHz  = 0.0;
    };

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

    /*  In-place Walsh-Hadamard transform on 8 doubles. log2(8) = 3 passes,
        each pair of butterflies costs 1 add + 1 sub -> 24 ops total. */
    static inline void hadamard8 (double* v) noexcept
    {
        for (int step = 1; step < kLines; step <<= 1)
        {
            for (int i = 0; i < kLines; i += step << 1)
            {
                for (int j = 0; j < step; ++j)
                {
                    const double a = v[i + j];
                    const double b = v[i + j + step];
                    v[i + j]        = a + b;
                    v[i + j + step] = a - b;
                }
            }
        }
    }

    double sr     = 44100.0;
    double invSr  = 1.0 / 44100.0;
    double sizeMul         = 1.0;
    double fbGain          = 0.85;
    double dampCoef        = 0.1;
    double preDelay        = 0.0;
    double modDepthSamples = 0.0;
    double width           = 1.0;

    std::array<Line, kLines> lines;
    std::vector<float>       preBuf;
    int                      preWrite = 0;
    int                      preLen   = 0;
};

} // namespace ob8::dsp
