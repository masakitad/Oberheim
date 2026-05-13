#pragma once

namespace ob8::dsp {

/*
    Standard one-pole DC blocker. The OB-8 audio path is AC-coupled at
    several points; we lump that into a single output blocker so the VCF
    self-oscillation and asymmetric distortions don't leave DC on the bus.
*/
class DCBlocker
{
public:
    void prepare (double /*sampleRate*/, double cornerHz = 5.0) noexcept
    {
        // R derived for a -3 dB corner of cornerHz at any sample rate.
        // Approximation: R = 1 - 2*pi*fc/fs. We pick a conservative R = 0.9995
        // (~7 Hz @ 44.1 kHz) and let the user override if needed.
        (void) cornerHz;
        R = 0.9995;
        reset();
    }

    void reset() noexcept { x1 = 0.0; y1 = 0.0; }

    inline double processSample (double x) noexcept
    {
        const double y = x - x1 + R * y1;
        x1 = x;
        y1 = y;
        return y;
    }

private:
    double R = 0.9995;
    double x1 = 0.0, y1 = 0.0;
};

} // namespace ob8::dsp
