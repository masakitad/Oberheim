#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

namespace ob8::dsp {

/*
    High-quality stereo reverb -- production-grade rewrite.

    Architecture (Lexicon-style):

        in L/R -> pre-delay -> input diffusion (4 allpass / channel)
               -> early reflections (8 taps / channel)
               -> late reverb (8-line FDN with allpass dispersion + damping)
               -> stereo decorrelated output (ER + late) -> output low-cut

    Why this layout vs. a plain FDN:

      1. INPUT DIFFUSION. Cascaded all-pass filters smear transients before
         they enter the recursive tail. Without this, sharp percussive
         input creates audible "machine-gunning" in the FDN's first
         iteration. Four stages per channel (with different delay sets L/R
         for decorrelation) is the classic Schroeder / Dattorro / Lexicon
         arrangement.

      2. EARLY REFLECTIONS. The first 20..100 ms of a room are dominated
         by discrete reflections, not a diffuse tail. We render eight
         tapped delays per channel with prime-millisecond timings and
         per-tap gains; these provide the spatial cue (room size, source
         distance) that pure-FDN reverbs are missing.

      3. LATE REVERB FDN. Eight delay lines (29..127 ms), each followed by
         a small all-pass for dispersion, then a one-pole damping low-pass
         in the feedback path. The all-passes add modal density without
         introducing coloration (their magnitude response is unity).

      4. CUBIC LAGRANGE INTERPOLATION on modulated reads keeps the tail
         clean as the modulation sweeps through fractional sample
         positions; linear interp would add high-frequency loss.

      5. STEREO DECORRELATION. The two input-diffusion paths use different
         prime delays, and the output mix taps non-overlapping subsets of
         the eight FDN lines for L and R. The width control crossfades
         between mono sum and the fully decorrelated mix.

    The public interface matches the previous FDN reverb (prepare,
    processSample, setSize/Decay/Damping/PreDelaySeconds/ModulationDepth/
    Width) so callers don't need to change.
*/
class FDNReverb
{
public:
    static constexpr int kLines        = 8;
    static constexpr int kDiffStages   = 4;
    static constexpr int kEarlyTaps    = 8;
    static constexpr double kTwoPi     = 6.283185307179586;

    void prepare (double sampleRate)
    {
        sr     = sampleRate;
        invSr  = 1.0 / sr;

        // -- (1) Pre-delay (mono, max 300 ms) ---------------------------------
        preLen = static_cast<int> (sr * 0.300) + 4;
        preBuf.assign (static_cast<size_t> (preLen), 0.0f);
        preWrite = 0;

        // -- (2) Input diffusion -- different prime delays L vs. R for
        //        stereo decorrelation. Coefficients climb so later stages
        //        diffuse a slightly broader band.
        const double diffMsL[kDiffStages] = {  4.491,  7.127, 11.703, 19.313 };
        const double diffMsR[kDiffStages] = {  5.701,  8.171, 13.307, 21.911 };
        const double diffCoef[kDiffStages] = { 0.62, 0.65, 0.68, 0.70 };
        for (int i = 0; i < kDiffStages; ++i)
        {
            initAllPass (diffuserL[i], diffMsL[i] * 0.001 * sr, diffCoef[i]);
            initAllPass (diffuserR[i], diffMsR[i] * 0.001 * sr, diffCoef[i]);
        }

        // -- (3) Early reflections -- 8 prime-ms taps per channel. Gains
        //        decay smoothly so they sum into a believable "first 100ms"
        //        impulse response rather than 8 audible echoes.
        const double erMsL[kEarlyTaps]   = { 10.7, 18.3, 26.7, 34.9, 46.7, 58.3, 71.1, 83.7 };
        const double erMsR[kEarlyTaps]   = { 13.1, 22.7, 30.7, 38.3, 50.9, 63.1, 75.3, 87.3 };
        const double erGain[kEarlyTaps]  = { 0.84, 0.72, 0.62, 0.55, 0.45, 0.38, 0.32, 0.27 };
        erLen = static_cast<int> (sr * 0.120) + 8;
        erBufL.assign (static_cast<size_t> (erLen), 0.0f);
        erBufR.assign (static_cast<size_t> (erLen), 0.0f);
        erWrite = 0;
        for (int i = 0; i < kEarlyTaps; ++i)
        {
            erTapsL[i] = { erMsL[i] * 0.001 * sr, erGain[i] };
            erTapsR[i] = { erMsR[i] * 0.001 * sr, erGain[i] };
        }

        // -- (4) Late reverb FDN ---------------------------------------------
        const double baseMs[kLines] = {
            29.731, 37.121, 43.717, 51.349,
            61.213, 73.939, 89.273, 107.357
        };
        // Per-line all-pass dispersion delays (small, primes-ish, varied)
        const double apMs[kLines] = {
             5.221,  7.479,  9.631, 11.297,
             4.183,  6.737,  8.819, 10.453
        };
        const int lineMax = static_cast<int> (sr * 0.180) + 8;  // size up to 1.5x of 127 ms ~ 190 ms
        for (int i = 0; i < kLines; ++i)
        {
            auto& l = lines[i];
            l.buf.assign (static_cast<size_t> (lineMax), 0.0f);
            l.writePos    = 0;
            l.baseDelay   = baseMs[i] * 0.001 * sr;
            l.dampState   = 0.0;
            l.modPhase    = static_cast<double> (i) / kLines;
            l.modRateHz   = 0.10 + 0.045 * i;           // 0.10..0.42 Hz

            // All-pass buffer sized to (delay + 1) so a read at writePos+1
            // is exactly `delay` samples behind the write head.
            const int apLen = std::max (4, static_cast<int> (apMs[i] * 0.001 * sr) + 1);
            l.apBuf.assign (static_cast<size_t> (apLen), 0.0f);
            l.apWritePos  = 0;
            l.apCoef      = 0.55;
        }

        // Output low-cut state
        hpStateL = hpStateR = 0.0;
        hpCoef   = 1.0 - std::exp (-kTwoPi * 12.0 * invSr); // 12 Hz HP

        setSize             (1.0);
        setDecay            (0.65);
        setDamping          (0.4);
        setPreDelaySeconds  (0.012);
        setModulationDepth  (0.0015);   // ~1.5 ms peak
        setWidth            (1.0);
    }

    void reset() noexcept
    {
        for (auto& l : lines)
        {
            std::fill (l.buf.begin(),   l.buf.end(),   0.0f);
            std::fill (l.apBuf.begin(), l.apBuf.end(), 0.0f);
            l.dampState  = 0.0;
            l.writePos   = 0;
            l.apWritePos = 0;
        }
        for (int i = 0; i < kDiffStages; ++i)
        {
            std::fill (diffuserL[i].buf.begin(), diffuserL[i].buf.end(), 0.0f);
            std::fill (diffuserR[i].buf.begin(), diffuserR[i].buf.end(), 0.0f);
            diffuserL[i].writePos = 0;
            diffuserR[i].writePos = 0;
        }
        std::fill (erBufL.begin(), erBufL.end(), 0.0f);
        std::fill (erBufR.begin(), erBufR.end(), 0.0f);
        std::fill (preBuf.begin(), preBuf.end(), 0.0f);
        erWrite = 0;
        preWrite = 0;
        hpStateL = hpStateR = 0.0;
    }

    /*  Apparent room size, [0.5, 1.5]. Multiplies the FDN delay lengths.
        ER tap times are NOT scaled -- early reflections are perceived
        absolutely so we leave them fixed. */
    void setSize (double s) noexcept
    {
        sizeMul = std::clamp (s, 0.5, 1.5);
    }

    /*  Decay [0, 1] -> feedback gain in the FDN core. */
    void setDecay (double d) noexcept
    {
        const double clamped = std::clamp (d, 0.0, 1.0);
        fbGain = 0.62 + 0.36 * clamped;     // 0.62 .. 0.98
    }

    /*  Damping [0, 1]: higher values darken the tail more aggressively.
        Mapped to a per-line one-pole low-pass corner from 18 kHz down to
        ~200 Hz on a log curve so the knob feels musical. */
    void setDamping (double d) noexcept
    {
        const double clamped = std::clamp (d, 0.0, 1.0);
        const double cornerHz = 18000.0 * std::pow (200.0 / 18000.0, clamped);
        dampCoef = 1.0 - std::exp (-kTwoPi * cornerHz * invSr);
    }

    void setPreDelaySeconds (double s) noexcept
    {
        preDelay = std::clamp (s * sr, 0.0, static_cast<double> (preLen) - 4.0);
    }

    void setModulationDepth (double secs) noexcept
    {
        modDepthSamples = std::clamp (secs * sr, 0.0, sr * 0.005);
    }

    void setWidth (double w) noexcept
    {
        width = std::clamp (w, 0.0, 1.0);
    }

    inline void processSample (double inL, double inR,
                               double& outL, double& outR) noexcept
    {
        // ---- (1) Pre-delay: sum to mono so the input has consistent
        //         decorrelation behaviour entering the diffusers (the
        //         diffusers themselves produce the stereo image). --------------
        const double inMono = (inL + inR) * 0.5;
        preBuf[static_cast<size_t> (preWrite)] = static_cast<float> (inMono);
        const int preReadPos = ((preWrite - static_cast<int> (preDelay)) % preLen + preLen) % preLen;
        const double preOut  = preBuf[static_cast<size_t> (preReadPos)];
        if (++preWrite >= preLen) preWrite = 0;

        // ---- (2) Input diffusion -- 4 cascaded all-passes per channel --------
        double dL = preOut, dR = preOut;
        for (int i = 0; i < kDiffStages; ++i) dL = processAllPass (diffuserL[i], dL);
        for (int i = 0; i < kDiffStages; ++i) dR = processAllPass (diffuserR[i], dR);

        // ---- (3) Early reflections -- write diffused signal, read taps -----
        erBufL[static_cast<size_t> (erWrite)] = static_cast<float> (dL);
        erBufR[static_cast<size_t> (erWrite)] = static_cast<float> (dR);

        double erL = 0.0, erR = 0.0;
        for (int i = 0; i < kEarlyTaps; ++i)
        {
            const int rdL = ((erWrite - static_cast<int> (erTapsL[i].delay))
                              % erLen + erLen) % erLen;
            const int rdR = ((erWrite - static_cast<int> (erTapsR[i].delay))
                              % erLen + erLen) % erLen;
            erL += erBufL[static_cast<size_t> (rdL)] * erTapsL[i].gain;
            erR += erBufR[static_cast<size_t> (rdR)] * erTapsR[i].gain;
        }
        // Normalise ER level (sum of gains ~3.55 -> divide so peak stays sane)
        erL *= 0.32;
        erR *= 0.32;

        if (++erWrite >= erLen) erWrite = 0;

        // ---- (4) Late reverb FDN -------------------------------------------
        std::array<double, kLines> y{};

        // 4a. Read each line with cubic interp + slow modulation
        for (int i = 0; i < kLines; ++i)
        {
            auto& l = lines[i];

            l.modPhase += l.modRateHz * invSr;
            if (l.modPhase >= 1.0) l.modPhase -= 1.0;
            const double modOffset = modDepthSamples * std::sin (l.modPhase * kTwoPi);

            const double delayLen = l.baseDelay * sizeMul + modOffset;
            y[i] = readCubic (l.buf, l.writePos, delayLen);
        }

        // 4b. Per-line all-pass dispersion (Schroeder allpass, ~5..11 ms)
        for (int i = 0; i < kLines; ++i)
        {
            auto& l = lines[i];
            const int rd = (l.apWritePos + 1) % static_cast<int> (l.apBuf.size());
            const double delayed = l.apBuf[static_cast<size_t> (rd)];
            const double yAp     = -l.apCoef * y[i] + delayed;
            l.apBuf[static_cast<size_t> (l.apWritePos)] =
                static_cast<float> (y[i] + l.apCoef * yAp);
            if (++l.apWritePos >= static_cast<int> (l.apBuf.size())) l.apWritePos = 0;
            y[i] = yAp;
        }

        // 4c. Per-line damping low-pass
        for (int i = 0; i < kLines; ++i)
        {
            auto& l = lines[i];
            l.dampState += dampCoef * (y[i] - l.dampState);
            y[i] = l.dampState;
        }

        // 4d. Walsh-Hadamard mixing (unitary, no multiplies)
        hadamard8 (y.data());
        const double norm = 1.0 / std::sqrt (static_cast<double> (kLines));
        for (auto& v : y) v *= norm;

        // 4e. Write back: diffused input + scaled feedback. Spread input
        //     across all 8 lines with sign alternation so excitation is
        //     dense from sample 0.
        const double driveL = dL * 0.5;
        const double driveR = dR * 0.5;
        const double inj[kLines] = {
             driveL + driveR,  driveL - driveR,
             driveL + driveR,  driveR - driveL,
             driveR + driveL,  driveR - driveL,
             driveL + driveR,  driveL - driveR
        };
        for (int i = 0; i < kLines; ++i)
        {
            auto& l = lines[i];
            l.buf[static_cast<size_t> (l.writePos)] =
                static_cast<float> (inj[i] * 0.3535 + fbGain * y[i]);
            if (++l.writePos >= static_cast<int> (l.buf.size())) l.writePos = 0;
        }

        // ---- (5) Stereo decorrelation -- mix opposite-phase subsets --------
        const double lateL = (y[0] + y[2] - y[5] - y[7]) * 0.5;
        const double lateR = (y[1] + y[3] - y[4] - y[6]) * 0.5;

        // Combine ER + late tail. The two are out-of-phase friends:
        // ER carries spatial cue, late tail carries decay character.
        const double mixL = erL + lateL;
        const double mixR = erR + lateR;

        // Width crossfade between mono sum and decorrelated mix
        const double mono = (mixL + mixR) * 0.5;
        double wetL = mono + width * (mixL - mono);
        double wetR = mono + width * (mixR - mono);

        // ---- (6) Output low-cut (12 Hz HP, one-pole) -----------------------
        hpStateL += hpCoef * (wetL - hpStateL);
        hpStateR += hpCoef * (wetR - hpStateR);
        outL = wetL - hpStateL;
        outR = wetR - hpStateR;
    }

private:
    struct AllPass
    {
        std::vector<float> buf;
        int    writePos = 0;
        double coef     = 0.5;
    };

    struct Tap
    {
        double delay = 0.0;
        double gain  = 0.0;
    };

    struct Line
    {
        std::vector<float> buf;
        int    writePos     = 0;
        double baseDelay    = 0.0;
        double modPhase     = 0.0;
        double modRateHz    = 0.0;
        double dampState    = 0.0;
        std::vector<float> apBuf;
        int    apWritePos   = 0;
        double apCoef       = 0.5;
    };

    static void initAllPass (AllPass& ap, double samples, double coef)
    {
        const int len = std::max (4, static_cast<int> (std::round (samples)));
        ap.buf.assign (static_cast<size_t> (len), 0.0f);
        ap.writePos = 0;
        ap.coef     = coef;
    }

    static inline double processAllPass (AllPass& ap, double in) noexcept
    {
        const int sz = static_cast<int> (ap.buf.size());
        const int rd = (ap.writePos + 1) % sz;
        const double delayed = ap.buf[static_cast<size_t> (rd)];
        const double y       = -ap.coef * in + delayed;
        ap.buf[static_cast<size_t> (ap.writePos)] =
            static_cast<float> (in + ap.coef * y);
        if (++ap.writePos >= sz) ap.writePos = 0;
        return y;
    }

    /*  4-point Catmull-Rom cubic interpolation. For a fractional read
        position we use samples at indices [i-1, i, i+1, i+2] and evaluate

            y = a3 t^3 + a2 t^2 + a1 t + a0
              with  a0 = x0
                    a1 = 0.5 (x1 - x_-1)
                    a2 = x_-1 - 2.5 x0 + 2 x1 - 0.5 x2
                    a3 = -0.5 x_-1 + 1.5 x0 - 1.5 x1 + 0.5 x2

        ~6 multiplies + 6 adds vs. linear's 1 + 1, but eliminates audible
        HF loss when sweeping a modulated tap. */
    static inline double readCubic (const std::vector<float>& buf,
                                    int writeIdx, double delaySamples) noexcept
    {
        const int sz   = static_cast<int> (buf.size());
        const double r = static_cast<double> (writeIdx) - delaySamples;
        const int    i = static_cast<int> (std::floor (r));
        const double t = r - static_cast<double> (i);
        auto at = [&] (int k) -> double
        {
            const int idx = ((k % sz) + sz) % sz;
            return static_cast<double> (buf[static_cast<size_t> (idx)]);
        };
        const double xm1 = at (i - 1);
        const double x0  = at (i);
        const double x1  = at (i + 1);
        const double x2  = at (i + 2);

        const double a0 = x0;
        const double a1 = 0.5 * (x1 - xm1);
        const double a2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
        const double a3 = -0.5 * xm1 + 1.5 * x0 - 1.5 * x1 + 0.5 * x2;
        return ((a3 * t + a2) * t + a1) * t + a0;
    }

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

    // Input chain
    std::vector<float>          preBuf;
    int                         preWrite = 0;
    int                         preLen   = 0;
    std::array<AllPass, kDiffStages> diffuserL{}, diffuserR{};

    // Early reflections
    std::vector<float>          erBufL, erBufR;
    int                         erWrite = 0;
    int                         erLen   = 0;
    std::array<Tap, kEarlyTaps> erTapsL{}, erTapsR{};

    // Late reverb FDN
    std::array<Line, kLines>    lines{};

    // Output HP
    double hpCoef   = 0.0;
    double hpStateL = 0.0;
    double hpStateR = 0.0;
};

} // namespace ob8::dsp
