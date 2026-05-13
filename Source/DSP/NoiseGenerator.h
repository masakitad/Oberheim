#pragma once

#include <cstdint>

namespace ob8::dsp {

/*
    Cheap white-noise source using xorshift32. The OB-8 had a single global
    noise generator routed into the mixer; we mirror that.

    Output is in roughly [-1, 1].
*/
class NoiseGenerator
{
public:
    explicit NoiseGenerator (uint32_t seed = 0xC0FFEEu) : state (seed ? seed : 1u) {}

    inline double processSample() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        // Map to [-1, 1]
        return (static_cast<int32_t> (state) / 2147483648.0);
    }

    void reset (uint32_t seed) noexcept { state = seed ? seed : 1u; }

private:
    uint32_t state;
};

} // namespace ob8::dsp
