# OB-8 Native

Native polyphonic analog-modelling synthesizer plug-in inspired by the
**Oberheim OB-8** (1983). Built as a JUCE 8 plug-in (VST3 / AU / Standalone)
with a focus on faithful DSP architecture rather than panel-by-panel mimicry.

> **Honest disclaimer.** The Oberheim OB-8 is a discrete analog instrument
> whose sound depends on Curtis CEM3340 VCOs, CEM3320 VCFs, CEM3310
> envelopes, and a unique audio bus full of subtle non-linearities. A
> single-session software port can capture the *architecture and DSP
> character* but reaching a level where it is **literally
> indistinguishable** from the hardware requires iterative ear-tuning
> against a real OB-8 (and commercial emulations such as GForce OB-X8 or
> Cherry Audio Eight Voice took experienced DSP teams substantial time to
> achieve this). The codebase here is a serious starting point – every
> module uses the best-practice DSP technique for its task – but expect to
> tune coefficients, envelope curves and saturation curves by ear to push
> the resemblance further.

---

## Features

| Section          | Implementation                                                                 |
|------------------|--------------------------------------------------------------------------------|
| Polyphony        | 8 voices (Poly / Unison / Mono modes), per-voice analog drift                  |
| Oscillators      | 2× band-limited VCOs (saw / pulse + PWM), hard sync, cross-modulation, 4× OS   |
| Mixer            | VCO1, VCO2, Noise levels                                                       |
| Filter           | CEM3320-style 2-pole / 4-pole low-pass, TPT/ZDF SVF, self-oscillation          |
| Envelopes        | 2× ADSR (Filter, Amp), exponential analog curves (CEM3310-style)               |
| LFO              | Triangle / Square / Saw / Inverse Saw / S&H, routable to VCO1, VCO2, PWM, VCF  |
| Velocity         | Velocity → VCA, Velocity → VCF                                                 |
| Pitch bend       | ±1..24 semitones, master tune ±1 semitone                                      |
| Anti-aliasing    | PolyBLEP + 4× oversampling (equiripple half-band FIR)                          |

---

## DSP details

* **`PolyBLEPOscillator`** – band-limited saw and pulse using the
  Välimäki/Huovilainen PolyBLEP residual, with hard-sync correction. Cross-mod
  is implemented as linear FM into the phase increment (the standard analog
  X-MOD topology).
* **`StateVariableFilter`** – Vadim Zavalishin's TPT (zero-delay-feedback)
  state-variable filter, with a soft `tanh` non-linearity in the resonance
  feedback path. 4-pole mode cascades two identical TPT stages, matching the
  OB-8's two-pole / four-pole switch.
* **`Envelope`** – first-order RC-style ADSR that overshoots the attack target
  (1.2 internal, clamped to 1.0 output) to give the Curtis CEM3310 "snap".
* **`AnalogDrift`** – per-voice slow brown-noise drift in semitones, modelling
  the per-VCO thermal drift on the original.
* **`Oversampling`** – `juce::dsp::Oversampling` at 4× (configurable) with an
  equiripple FIR half-band filter; latency-compensated.

---

## Build

JUCE 8.0.4 is fetched automatically via CMake `FetchContent`. You need a
recent C++20 compiler and CMake ≥ 3.22.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

On first configure CMake will clone JUCE under `build/_deps/`. Subsequent
builds reuse it.

### Output

* `build/OB8Native_artefacts/Release/VST3/OB-8 Native.vst3`
* `build/OB8Native_artefacts/Release/Standalone/OB-8 Native` (or `.exe`)
* macOS: `build/OB8Native_artefacts/Release/AU/OB-8 Native.component`

Copy the artefact to your DAW's plug-in folder, or run the Standalone
binary directly.

### Dependencies on Linux

```sh
sudo apt install build-essential libasound2-dev libjack-jackd2-dev \
                 libcurl4-openssl-dev libfreetype-dev libx11-dev libxcomposite-dev \
                 libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev \
                 libxrender-dev libwebkit2gtk-4.1-dev libglu1-mesa-dev
```

---

## File layout

```
.
├── CMakeLists.txt
├── README.md
└── Source/
    ├── Parameters.{h,cpp}     # APVTS parameter layout + IDs
    ├── PluginProcessor.{h,cpp}
    ├── PluginEditor.{h,cpp}
    ├── DSP/
    │   ├── PolyBLEPOscillator.h
    │   ├── StateVariableFilter.h
    │   ├── Envelope.h
    │   ├── LFO.h
    │   ├── NoiseGenerator.h
    │   ├── AnalogDrift.h
    │   ├── DCBlocker.h
    │   ├── Oversampler.h          # (currently unused – host processor uses juce::dsp::Oversampling directly)
    │   ├── Voice.{h,cpp}
    └── GUI/
        ├── OB8LookAndFeel.{h,cpp}
        └── OB8Knob.{h,cpp}
```

---

## What is intentionally *not* in scope (yet)

These were left out so the initial commit stays focused, and they are the
most useful next steps if you want to push fidelity further:

* **Patch memory** – the OB-8 stored 120 patches; here we rely on the host's
  preset support. Easy to add an XML/JSON bank loader.
* **Split / Double** keyboard modes – only Poly / Unison / Mono are wired up.
  Adding Split is straightforward (two parameter banks + per-voice routing).
* **Page-2 parameters** – the OB-8's "Page 2" introduced extra modulation
  destinations. Add by extending `PerVoiceParams`.
* **MIDI CC mapping** for the panel knobs.
* **Component-level model of CEM3320** – the current SVF captures the
  topology but not the exact OTA non-linearity. Replacing the `tanh`
  saturation with a measured CEM3320 curve (e.g. via wave-shaping data
  captured from a real unit) would close more of the gap.
* **Per-voice tempco model on the envelopes** – the CEM3310 has a known
  voltage-vs-time curve that we approximate with a one-pole. A piecewise
  fit to the datasheet curve would be more accurate.

---

## License

This is a clean-room emulation referencing public-domain DSP techniques.
"Oberheim" and "OB-8" are trademarks of their respective owners; this
project is not affiliated with or endorsed by Oberheim Electronics or
its successors.
