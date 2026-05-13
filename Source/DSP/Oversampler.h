#pragma once

#include <juce_dsp/juce_dsp.h>

namespace ob8::dsp {

/*
    Thin wrapper around juce::dsp::Oversampling. We oversample the entire
    voice path (oscillators + filter) by 4x with 8x as an optional high-quality
    mode. This keeps oscillator aliasing well below audibility and stops the
    filter's tan() pre-warp from going non-linear near Nyquist.

    The OB-8 hardware has no oversampling - it's analog. We use oversampling
    on the digital side specifically because the bandlimited oscillator
    formulas (PolyBLEP) only push aliasing down to a moderate level on their
    own.
*/
class Oversampler
{
public:
    void prepare (double hostSampleRate, int blockSize, int factorPow2 = 2) noexcept
    {
        factor = factorPow2;
        ratio  = 1 << factor;
        oversampler.reset (new juce::dsp::Oversampling<float> (
            1,                                    // mono internal bus
            factor,
            juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true,    // maximum quality
            false)); // not integer latency
        oversampler->initProcessing (static_cast<size_t> (blockSize));
        oversampler->reset();
        hostSr = hostSampleRate;
    }

    void reset() noexcept { if (oversampler) oversampler->reset(); }

    double getInternalSampleRate() const noexcept { return hostSr * ratio; }
    int    getRatio()              const noexcept { return ratio; }

    juce::dsp::AudioBlock<float> processSamplesUp (juce::dsp::AudioBlock<float> input)
    {
        return oversampler->processSamplesUp (input);
    }

    void processSamplesDown (juce::dsp::AudioBlock<float> output)
    {
        oversampler->processSamplesDown (output);
    }

    float getLatencyInSamples() const noexcept
    {
        return oversampler ? oversampler->getLatencyInSamples() : 0.0f;
    }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int    factor  = 2;
    int    ratio   = 4;
    double hostSr  = 44100.0;
};

} // namespace ob8::dsp
