# CLAUDE.md — OB-8 Native 開発メモ

このファイルは Claude (および将来の開発セッション) が **同じ落とし穴に再度はまらない** ためのプロジェクト固有のメモです。実装の概要は `README.md` を参照。

---

## ワークフローの掟

1. **PR はマージが速い。古い branch に push しない。**
   このリポジトリでは PR を出すとすぐ (数分以内) マージされる運用。マージ済みの branch に追加 commit を push しても無視される。
   → **追加修正は必ず main から新しい branch を切って新 PR を立てる**。
2. **PR を立てたら subscribe して CI / レビューコメントを確認**。
3. branch 命名: feature は `feature/...`、修正は `fix/...`。

---

## 配布

別の Mac に渡すと、ad-hoc 署名は通用せず Gatekeeper にブロックされる。`dist/install.command` を同梱して受け取った人が一度ダブルクリックする方式を採用している。

```sh
cmake --build build --target dist        # build/dist/OB-8 Native/ を生成
cmake --build build --target dist-zip    # build/dist/OB-8 Native.zip を生成
```

`build/dist/OB-8 Native/` に入るもの:

- `OB-8 Native.app` (ad-hoc 署名済み)
- `install.command` (受け取った Mac で xattr -cr + codesign を打ち直す)
- `お読みください.txt` (日本語インストール手順)

公証 (notarytool + stapler) なしで配るための簡易ルート。Developer ID + notarize する場合は別途。

---

## Lint / Format

JUCE スタイルの `.clang-format` と DSP コード向けにチューニングした `.clang-tidy` がリポジトリ直下にある。CMake が `compile_commands.json` を吐くので clang-tidy がそのまま動く。

```sh
cmake -B build  # compile_commands.json を生成
cmake --build build --target format         # Source/ を in-place で整形
cmake --build build --target format-check   # dry-run、差分があれば失敗
cmake --build build --target tidy           # clang-tidy パス
```

`clang-format` / `clang-tidy` 未インストール時はターゲットそのものが生成されないため、ビルドは壊れない (`brew install llvm` で導入可能)。

`bugprone-*` `clang-analyzer-*` `cert-*` 系は warning-as-error。スタイル系 (readability, modernize) は warning にとどめて opinionated にしない。

---

## ビルド (macOS)

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
open "build/OB8Native_artefacts/Release/Standalone/OB-8 Native.app"
```

`CMakeLists.txt` の post-build で `xattr -cr` と `codesign --force --deep --sign -` を自動実行するため、Gatekeeper の「破損しているか不完全である」エラーは出ない。

ユニバーサルバイナリ (arm64 + x86_64) がデフォルト。

### よくあるビルドエラー

| 症状 | 原因 | 対処 |
|---|---|---|
| `kLSNoExecutableErr` で起動失敗 | `cmake -B build` だけ走らせて `cmake --build` を忘れている | `cmake --build build --config Release -j` |
| `アプリケーション "..." は、破損している...` | アドホック署名漏れ | `xattr -cr` + `codesign --force --deep --sign -` (post-build で自動化済み) |
| `error: no type named 'XXX' in namespace 'juce'` | JUCE モジュールのヘッダ include 漏れ | 後述 |

---

## JUCE 8 の落とし穴

### 1. モジュールヘッダの include は必須

`CMakeLists.txt` で `target_link_libraries(... juce::juce_audio_utils)` してリンクしていても、ソースで型を使うには **アンブレラヘッダの明示的な include が必要**:

```cpp
#include <juce_audio_utils/juce_audio_utils.h>   // MidiKeyboardComponent
```

**型 → 必要なモジュール対応表**:

| 型 | モジュールヘッダ |
|---|---|
| `juce::MidiKeyboardState` | `juce_audio_basics` (juce_audio_processors 経由で transitive) |
| `juce::MidiKeyboardComponent` | `juce_audio_utils` (**明示 include 必要**) |
| `juce::dsp::Oversampling` | `juce_dsp` |
| `juce::AudioProcessorValueTreeState` | `juce_audio_processors` |
| `juce::FileChooser` | `juce_gui_basics` |

### 2. `apvts.copyState()` は **非-const**

JUCE 8 の `AudioProcessorValueTreeState::copyState()` は内部で `flushParameterValuesToValueTree()` を呼ぶため非-const。 const メンバ関数から呼ぶとコンパイルエラー:

```
error: 'this' argument to member function 'copyState' has type
'const juce::AudioProcessorValueTreeState', but function is not marked const
```

→ そのメソッドの `const` を外す。`apvts.state.createCopy()` で代用すると flush されないので注意。

### 3. `setSize()` の値はホストにキャッシュされる

エディタの `setSize()` を変えても、DAW によっては前回サイズを覚えていて反映されない場合がある。プラグインを一度ホストから外して再ロード、または `setResizable(true, true)` で対応。

### 4. PolyBLEP / TPT-SVF / oversampling は全部入り

`Source/DSP/` に揃っている (DSP の細部は `README.md` 参照):

- `PolyBLEPOscillator.h` — CEM3340 風 (saw/pulse, hard sync, X-MOD)
- `StateVariableFilter.h` — CEM3320 風 (2/4-pole, 非対称 OTA 飽和)
- `Envelope.h` — CEM3310 風 (overshoot snap)
- `LFO.h` / `AnalogDrift.h` / `NoiseGenerator.h` / `DCBlocker.h` / `Oversampler.h`
- `Voice.h` (renderAdd で 1 voice 全パス)
- `Delay.h` (ステレオフィードバックディレイ, FB に LPF + cross-feedback)
- `Reverb.h` (FDN8 + Walsh-Hadamard ミキシング + 各 line 遅延変調 + 減衰 LPF)

Post FX (`Delay` / `Reverb`) は host SR で動作。Voice/VCF は OS 内部 SR、master gain と downsample の **後** に挟んでいる。

`Voice.h` の `PerVoiceParams` が Voice ↔ Processor の唯一の I/F。新しいモジュレーション先を増やす時はここに field を足し、`snapshotParams()` で値を埋め、`renderAdd` で使う。

### 4.5 Filter / DC blocker は note-on で reset しない

VCF / DCBlocker の `reset()` を `Voice::startNote` で呼ばない。実機 OB-8 はアナログ積分器が常時動作しているため state リセットの概念が無い。digital で reset = 0 から再開すると、低カットオフ時にフィルタが新規入力に対する steady-state に追従するまでの数 ms が「立ち上がり transient」として聞こえる。

代わりに **previously-idle voice では `preSettleFilter` で内部的に 256 サンプル先回し処理してフィルタを定常応答に持っていく** (出力は捨てる)。これで chord attack 時に複数 voice の transient が重なって露呈する click も消える。

最後の安全網として `fadeInCountdown = 64` も入れているが、これは voice stealing の場合 (pre-settle を skip) を主に守るためのもの。

### 5. Voice deactivation は **ブロック後** にやる

リリース中の音切れを防ぐため、`Voice::renderAdd` のサンプル内ループでは `active = false` にしない。ループが完了した **後** に一度だけチェックする:

```cpp
for (int i = 0; i < numSamples; ++i) { ... }
if (! ampEnv.isActive()) { active = false; currentMidiNote = -1; }
```

サンプル内で deactivate すると、その後のサンプルで「最後のテール」が描画されない / 音切れの原因になる可能性あり。

### 6. Voice stealing は Release 中の voice を最優先で奪う

`PluginProcessor.cpp::findFreeOrSteal` の 3 段階優先順位:

1. inactive な voice
2. Release ステージ中の voice (oldest)
3. それ以外の oldest voice

---

## GUI レイアウト

エディタは 1280 × 880。`OB8Editor::resized()` が全 13 セクションを配置:

```
┌─────────────────────────────────────────────────────────────────┐
│ HEADER (OB-8 NATIVE)                                            │
├─────────────────────────────────────────────────────────────────┤
│ VCO1 | VCO2 | X-MOD | MIXER | FILTER | FILTER ENV               │  Row 1 (230 px)
├─────────────────────────────────────────────────────────────────┤
│ AMP ENV | LFO | VELOCITY | VOICE                                │  Row 2 (200 px)
├─────────────────────────────────────────────────────────────────┤
│ PAGE 2 | SPLIT/DOUBLE | PATCH BANK                              │  Row 3 (~ remainder)
├─────────────────────────────────────────────────────────────────┤
│ ON-SCREEN KEYBOARD (mouse + PC keys A..K, W..U, Z/X)            │  100 px
└─────────────────────────────────────────────────────────────────┘
```

新しいセクションを足す時は:
1. `sections.push_back ({...})` を constructor に追加
2. `resized()` で `layoutSection (sections[N], bounds_subset, cols)` を呼ぶ
3. `Section.bounds` がフレーム描画に使われる (`paint()`)

---

## MIDI ハンドリング

`OB8Processor::handleMidiEvent` ですべての MIDI を処理:

- Note On/Off (sustain pedal 対応: `sustainedNotes` に保持)
- Pitch Wheel (`bendRange` パラメータでスケール)
- Channel & Poly Aftertouch (`currentAfterT` / `polyAfterTouch[]`)
- CC: 1 (MW), 7 (Vol), 11 (Exp), 64 (Sustain), 71-77 (各種), 120/123 (All Off)
- `keyboardState.processNextMidiBuffer` で画面鍵盤を merge

新しい CC を追加する時は `handleMidiEvent` の `switch (cc)` に case を足す。

### Hold / Sustain pedal

両方とも `sustainedNotes` (juce::SortedSet) に release 予定の note を貯める。

- **Sustain pedal (CC 64)**: 押されている間 noteOff を保留、離されたら (Hold が ON でなければ) 蓄積した note を全部 noteOff
- **Hold (panel toggle)**: 同様のロジック。pedal の代替。Hold が OFF になった瞬間は `processBlock` の冒頭でポーリングして `sustainedNotes` を解放

---

## パッチメモリ

- `bankState` (juce::ValueTree, "BANK") に 12 × 10 = 120 スロット
- 各 patch は子 ValueTree、中に APVTS の state を 1 child として保持
- `.ob8bank` ファイル形式は XML 1 階層 ("OB8Bank" → "BANK" → "Bank" × N → "Patch" × N)
- DAW プロジェクトに保存する時は `getStateInformation` が APVTS + BANK + メタ情報を MemoryBlock に詰める

---

## デバッグの当たりどころ

- **音が出ない**: voice allocation の問題、または amp env が released になっている。`Voice::isActive()` をログ。
- **歪む / クリップ**: oversampling のラウンドトリップで上限が変わる。`mixBuffer` の gain を確認。
- **GUI が真っ黒**: `OB8LookAndFeel` の `setColour` 順序問題。`drawRotarySlider` を疑う。
- **MIDI が効かない**: `acceptsMidi() == true` か、`processBlock` で `midi.isEmpty()` が常に true でないか。
