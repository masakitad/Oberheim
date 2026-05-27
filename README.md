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

| Section          | Implementation                                                                       |
|------------------|--------------------------------------------------------------------------------------|
| Polyphony        | 8 voices, **Poly / Unison / Split / Double / Mono** modes, per-voice analog drift    |
| Oscillators      | 2× band-limited VCOs (saw / pulse + PWM), hard sync, cross-modulation, 4× OS         |
| Mixer            | VCO1, VCO2, Noise levels                                                             |
| Filter           | CEM3320-modelled 2/4-pole low-pass, TPT/ZDF SVF, asymmetric OTA saturation           |
| Envelopes        | 2× ADSR (Filter, Amp), CEM3310-modelled exponential charge + attack overshoot snap   |
| LFO              | Triangle / Square / Saw / Inverse Saw / S&H, routable + **key sync**                 |
| Page 2 mods      | Filter Env → VCO1/VCO2/PWM, Aftertouch → VCF/LFO/VCA, Mod Wheel → VCF/LFO/Vibrato    |
| Velocity         | Velocity → VCA, Velocity → VCF                                                       |
| Pitch bend       | ±1..24 semitones, master tune ±1 semitone                                            |
| Anti-aliasing    | PolyBLEP + 4× oversampling (equiripple half-band FIR)                                |
| Patches          | 120-slot patch memory (12 banks × 10), Save / Load `.ob8bank` files                  |
| MIDI             | CC 1/7/11/64/71-77, Channel & Poly Aftertouch, Pitch Bend, Sustain pedal             |
| Performance      | **Glide** (portamento, 1 ms - 2 s), **Hold** (latches notes until released)          |
| Post FX          | **Stereo Delay** (separate L/R times, feedback w/ damping LPF, cross-feed) + **FDN8 Reverb** (8-line Walsh-Hadamard, prime delay lines, modulated taps, plate-style) |

---

## DSP details

* **`PolyBLEPOscillator`** – band-limited saw and pulse using the
  Välimäki/Huovilainen PolyBLEP residual, with hard-sync correction. Cross-mod
  is implemented as linear FM into the phase increment.
* **`StateVariableFilter`** – Vadim Zavalishin's TPT (zero-delay-feedback) SVF
  with **CEM3320-fitted OTA saturation**: quadratic pre-skew (~3% 2nd harmonic
  at full level) followed by a 5th-order minimax tanh approximation, plus
  resonance-loss compensation. 4-pole mode cascades two identical TPT stages.
* **`Envelope`** – ADSR fitted to the CEM3310 charge/discharge behaviour: the
  attack stage charges toward 1.5× the threshold and is clamped at 1.0,
  reproducing the Oberheim "snap" even for long attack times. Time
  coefficients are recomputed on the fly so live knob movements affect the
  ongoing stage (analog-accurate behaviour).
* **`AnalogDrift`** – per-voice slow brown-noise drift in semitones,
  independent per oscillator, modelling thermal drift.
* **`Oversampling`** – `juce::dsp::Oversampling` at 4×, equiripple FIR
  half-band filter; latency-compensated.

## On-screen keyboard

The bottom of the editor hosts a horizontal MIDI keyboard, starting at **C3 (scientific notation, MIDI 48)**. Click with the mouse or use the **PC keyboard**:

| PC key | Note |
|---|---|
| `A S D F G H J K L ;` | white keys (C3, D3, E3, F3, G3, A3, B3, ...) |
| `W E   T Y U   O P` | black keys (C#3, D#3, F#3, G#3, A#3, ...) |
| `Z` / `X` | shift play octave down / up (JUCE built-in) |
| **`<` / `>` buttons** | shift octave (visible on screen, syncs with label) |

Mouse and PC-keyboard notes are merged into the same MIDI stream that drives the synth, so velocity / mod / sustain CCs from external MIDI work alongside them. The on-screen keyboard also highlights notes played by external MIDI.

## Modes

* **Poly** – 8-voice last-note-priority allocation with oldest-voice steal.
* **Unison** – all 8 voices stack on the played note, spread by *Unison Detune*.
* **Split** – voices 0–3 play notes below *Split Point*, voices 4–7 play above.
  The upper half is offset by *Split Octave* / *Split Detune*.
* **Double** – every note is voiced twice (voices 0–3 and 4–7 layered) with
  ±½ × *Double Detune* between layers.
* **Mono** – single-voice with re-trigger.

## Factory presets

Bank 1 ships with 10 factory presets that you can RECALL straight away:

| # | Name | Description |
|---|---|---|
| 1 | Init Patch | Bright saw with snappy envelope (synth's neutral state) |
| 2 | OB Brass | Classic stacked-saws Oberheim brass |
| 3 | Lush Strings | Heavy detune, slow attack, long release |
| 4 | Sync Lead | Hard sync, mono, env-swept VCO2 pitch |
| 5 | Warm Pad | Very slow A/R, LFO PWM chorus, low filter |
| 6 | Funky Bass | Mono, lower octave, fast pluck env, kbd-tracked |
| 7 | Bell | X-MOD inharmonic spectrum, fast decay |
| 8 | Sweep Pad | Slow filter env sweep, long release |
| 9 | Plucky | Short percussive pluck with velocity |
| 10 | Acid Bass | Mono, screaming 4-pole resonance, env sweep |

The factory bank is loaded into Bank 1 on a fresh plug-in instance. The synth starts with the parameter defaults; **to switch presets, use the PATCH BANK section** at the bottom-right of the editor:

1. **BANK** dropdown -> select Bank 1
2. **PROGRAM** dropdown -> choose the preset (1: Init Patch, 2: OB Brass, ...)
3. Click **RECALL**

Once you **STORE** over a slot the factory data for that slot is replaced.

The editor window is resizable (drag the bottom-right corner). The default size is 1280x820; minimum 1180x720, so it fits on 13" laptop screens.

## Patch memory

* **Bank** dropdown: 12 banks (1–12), **Program**: 10 patches per bank.
* **STORE** writes the current parameter state into the selected slot under
  the entered name.
* **RECALL** loads the patch from the selected slot.
* **SAVE BANK… / LOAD BANK…** read/write `.ob8bank` XML files containing all
  120 slots.
* Banks are also saved as part of the host DAW's project state alongside
  the current parameters.

## MIDI mapping

| CC  | Destination               |
|-----|---------------------------|
| 1   | Mod Wheel (depth)         |
| 7   | Master Volume             |
| 11  | Expression (folded into AT-to-VCA path) |
| 64  | Sustain pedal             |
| 71  | Resonance                 |
| 72  | Amp Release               |
| 73  | Amp Attack                |
| 74  | Cutoff                    |
| 75  | Amp Decay                 |
| 76  | LFO Rate                  |
| 77  | Vibrato depth             |
| 120/123 | All Sound/Notes Off   |

---

## Distributing the standalone to other Macs

A self-built `.app` only runs on the machine that built it because the ad-hoc signature applied at build time isn't trusted by other Macs and the file picks up a `com.apple.quarantine` attribute on transfer. To make a bundle that a recipient can use:

```sh
cmake --build build --target dist        # writes build/dist/OB-8 Native/
cmake --build build --target dist-zip    # writes build/dist/OB-8 Native.zip
```

`build/dist/OB-8 Native/` contains three things:

- **`OB-8 Native.app`** -- the standalone bundle
- **`install.command`** -- a double-clickable shell script that, on the recipient's Mac, runs `xattr -cr` and `codesign --force --deep --sign -` against the `.app` so Gatekeeper lets it open. The script also offers to copy the app into `/Applications`.
- **`お読みください.txt`** -- Japanese instructions (also has command-line fall-back instructions).

Send the folder via AirDrop / a zip / a USB stick. The recipient double-clicks `install.command` once and then the `.app` runs like any other application.

This is the "free" distribution path (no Apple Developer account needed). For a polished install with no warnings at all you'd need a Developer ID Application certificate and Apple Notarization; see the macOS Apple Developer docs.

## Linting / formatting

The project ships with a `.clang-format` (JUCE-style) and a `.clang-tidy`. Both are picked up automatically by editors and language servers; the CMake configure also generates `build/compile_commands.json` so clang-tidy can find the include paths.

```sh
# After `cmake -B build` so the compile-commands DB exists:

cmake --build build --target format         # rewrites Source/ in place
cmake --build build --target format-check   # dry-run; fails on diffs
cmake --build build --target tidy           # full clang-tidy pass
```

`format-check` / `tidy` exit non-zero on findings -- handy for a pre-commit hook or a CI step. If `clang-format` / `clang-tidy` aren't installed (e.g. `brew install llvm`), the targets simply aren't created and the rest of the build works as before.

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

* **Component-level (SPICE-style) CEM3320 model** – the current asymmetric
  OTA fit is musically close but not bit-identical to a real chip across all
  operating points. A SPICE simulation captured to a wavetable would close
  the remaining gap.
* **Per-stage CEM3310 datasheet curve** – attack/decay/release currently use
  the same one-pole topology with different coefficients. The CEM3310's
  decay stage has a slight curvature change near the sustain level that we
  don't model.
* **Sample-accurate MIDI event splitting** – MIDI events are currently
  processed at the head of the block. For sub-block-accurate event timing
  the rendering loop needs to be split at each event.
* **Stereo voice panning** – the OB-8 was mono out; we duplicate to stereo.
* **Hard-sync PolyBLEP residual** – the slave reset on hard sync uses a
  simplified correction. A full BLEP residual at the sub-sample reset
  position would be cleaner.

---

## License

This is a clean-room emulation referencing public-domain DSP techniques.
"Oberheim" and "OB-8" are trademarks of their respective owners; this
project is not affiliated with or endorsed by Oberheim Electronics or
its successors.
