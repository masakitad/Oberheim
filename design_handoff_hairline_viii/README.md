# Handoff: OB8-UI — HAIRLINE-VIII Polyphonic Synthesizer

> このバンドルに含まれる HTML/JSX ファイルは **デザインリファレンス** です。
> プロダクションコードとしてそのまま使うのではなく、ターゲットコードベースの
> 既存環境 (React / Vue / Swift / etc.) と命名規約・状態管理パターンに沿って
> **同じ見た目・挙動を忠実に再現** してください。
> コードベースがまだ無い場合は、最も適切なフレームワーク (例: React + Vite + TypeScript)
> を選択して実装してください。

---

## 1. Overview

このデザインは、Oberheim OB-8 を参考にしつつ **オリジナルブランド「HAIRLINE-VIII」** として再構築した
**8音ポリフォニック・アナログモデリング・シンセサイザー** のソフトウェア UI です。

ビジュアル方向性は「**製図インク・温暖クリーム紙**」(drafting ink on aged paper)。
ヘアライン仕切り・モノスペース表記・赤い注釈ペンの言語で構成し、
箱型のセクションフレームを廃した「1枚の製図シート」として成立させています。

提供するバリアント:
- **A. Knob Edition** — 全コントロールが円形ノブ
- **B. Fader Edition** — 全コントロールが縦フェーダー (目盛り段付き)

それ以外は完全に同一のレイアウト・タイポ・色・余白を共有します。

---

## 2. Fidelity

**High-fidelity (hifi)** — 全ピクセル値、配色、書体、字間、ストローク幅まで確定済み。
ピクセルパーフェクトに再現してください。

---

## 3. Tech Notes (再現方法)

### 推奨スタック
- **React + TypeScript + Vite** (or 既存スタックに合わせる)
- **CSS Modules / styled-components / Tailwind** — テーマトークンを変数化
- 装飾は **すべて SVG / CSS** で表現 (画像不使用)

### 全体構成
- 固定幅 **1280px** のキャンバスを designed at 1:1 で実装
- ホスト側ビューポートに合わせてスケーリング (`transform: scale()`) する場合は
  letterbox 表示。コンテンツ側の寸法は固定。

### コンポーネント階層 (上から下へ)

```
<RefinedSynthPanel theme={...}>
  <PaperSheet>                       ← 紙の質感レイヤー (BG + 4種オーバーレイ)
    <CornerCrosshairs />              ← 四隅のレジストレーションマーク
    <Header />                        ← タイトル + シリーズ番号 + tagline
    <TabStrip />                      ← Program / Osc / Filter / Env / Mod / FX / Patch
    <Band height=116>                 ← Band 1: VCO1 / VCO2 / X-MOD / MIXER / FILTER / FILTER ENV
      <Cell index="01" label="VCO 1"> ... </Cell>
      <Cell index="02" label="VCO 2"> ... </Cell>
      ...
    </Band>
    <Band height=116>                 ← Band 2: AMP ENV / LFO / VELOCITY / VOICE
    <Band height=196>                 ← Band 3: PAGE 2 / SPLIT & DOUBLE / PATCH BANK (2行構成)
    <PresetFooter />                  ← プリセット番号付きタブ
    <Keyboard />                      ← C1〜C8 (50白鍵 + 35黒鍵) + 寸法線
    <TitleBlock />                    ← TITLE / SERIES / SCALE / SHEET / REV / DATE
  </PaperSheet>
</RefinedSynthPanel>
```

---

## 4. Design Tokens

### Colors (HAIRLINE-VIII theme)

| Token | Hex / Value | Usage |
|---|---|---|
| `appBg` | `radial-gradient(ellipse at 50% 40%, #a8966c 0%, #786646 70%, #5a4a30 100%)` | 外側デスク色 |
| `paperBg` | `#efe6cc` | 紙ベース (温暖クリーム) |
| `ink` | `#1a2538` | メイン文字・線 (深い藍ブラウン) |
| `inkDim` | `rgba(26,37,56,0.58)` | 二次的テキスト・値表示 |
| `inkFaint` | `rgba(26,37,56,0.20)` | 補助線・グレー番号 |
| `hairline` | `rgba(26,37,56,0.42)` | バンド区切り |
| `hairFine` | `rgba(26,37,56,0.20)` | 細部区切り (セル間・ティック) |
| `accent` | `#a23a1a` | 朱赤注釈ペン (アクセント・モッドチップ・スライダ thumb) |

### Typography

| Role | Family | Size / Weight | Letter-spacing | Usage |
|---|---|---|---|---|
| Display | `Fraunces`, `Cormorant Garamond`, Georgia, serif | **30px / 600** | 0.02em | パネル名 "HAIRLINE-VIII" |
| UI | `IBM Plex Mono`, monospace | **9px / 400-600** | 0.14em–0.20em | セルラベル・タブ・ボタン (uppercase) |
| Value | `IBM Plex Mono`, monospace | **8.5px** | 0.02em | ノブ下の数値表記 |
| Series | `IBM Plex Mono` | **9.5px** | 0.12em | 副題 "No. 0427 · rev. C" |
| Tagline | `IBM Plex Mono` | **10px** | 0.08em | "eight-voice polyphonic · drafted at 1:1" |

すべて `letter-spacing` を **厳密に守る** こと。製図感の核心。

### Spacing / Dimensions

| Token | Value |
|---|---|
| Panel width | **1280px** (fixed) |
| Outer desk padding | 36px |
| Header padding | 18px 26px 14px |
| Cell padding | 12px 14px |
| Band 1, 2 min height | **116px** |
| Band 3 min height | **196px** |
| Cell internal gap | 10px (between header & row) |
| KnobRow gap (default) | **14px** |
| Knob size (M / S) | 36 / 30px |
| Slider/fader height | 40px |
| Waveform preview | 88×36px |
| ModChip | h12, font 7.5px, padding 0 4px, border 1px |
| Tab strip item | padding 7px 18px |
| Keyboard area | 70px height; 50 white + 35 black keys |

### Borders / Strokes

- すべて **1px** のヘアライン。例外なし。
- 角丸 (border-radius) は **使用しない**。直角のみ。
- 例外: ModChip のみ `border-radius: 1px`

### Shadows

紙の影 (PaperSheet) のみ:
```css
box-shadow:
  0 22px 50px -18px rgba(20,12,4,0.55),
  0 4px 12px rgba(20,12,4,0.25),
  inset 0 0 0 1px rgba(60,40,15,0.08);
```

---

## 5. Paper Texture System

紙の質感は **4 レイヤー** を重ねて作る (`PaperOverlay` コンポーネント、絶対配置でパネル内側全面)。

| Layer | 内容 |
|---|---|
| 1. Warm halo & stains | 4 つの `radial-gradient` を合成。温色のシミと中央光 |
| 2. Fiber lines | `repeating-linear-gradient` 91° と 2° の 2 本、極めて低い alpha |
| 3. Grain noise | SVG `feTurbulence` (baseFrequency 0.92, octaves 2) → inline data URL。`mix-blend-mode: multiply`, opacity 0.32 |
| 4. Vignette | `radial-gradient` で四隅を暗く |
| 5. "DRAFT" 透かし | Fraunces 220px, opacity 0.05, rotate(-18deg) |

詳細は `synth-controls-refined.jsx` の `PaperOverlay` 関数を参照。

---

## 6. Components — 詳細仕様

### 6.1 RefinedKnob (円形ノブ)

SVG viewBox `0 0 100 100`。サイズは外側プロパティで指定 (デフォルト 36px)。

```
1. 外側スイープアーク (-135° to +135°)
   path: M 21.7,78.3 A 40,40 0 1 1 78.3,78.3
   stroke: hairFine, strokeWidth 1
2. 端ティック × 2 (両端、r40→r46)
   stroke: hairline, strokeWidth 1.1
3. 上中央ティック (50,6 → 50,12)
4. 本体円: cx50 cy50 r30
   fill: transparent, stroke: knobStroke (rgba 0.88), strokeWidth 1.4
5. インジケータ (回転 -135° + value × 270°)
   - 線: x1=50 y1=50 → x2=50 y2=22, stroke: accent (#a23a1a), strokeWidth 2.2, strokeLinecap round
   - 中央ドット: cx50 cy50 r2.2, fill: accent
```

### 6.2 RefinedFader (縦フェーダー — Fader Edition)

SVG width 26, height 48。

```
1. 左右ティック段 × 5本 (0, .25, .5, .75, 1 の位置)
   主要 (0, .5, 1): 長め (x: cx±7 → cx±11)
   副 (.25, .75): 短め (x: cx±8.5 → cx±11)
   stroke: hairline, strokeWidth 1
2. 中央スロット: 4px幅 × height40, stroke knobStroke 1px, fill transparent
3. スロット中央のハーフライン (横): stroke hairFine 1px
4. Thumb (位置 = (1-value) × (height-4))
   rect 20×4px, fill accent (#a23a1a)
   下辺に rgba(0,0,0,0.35) の影線 1px
```

### 6.3 KnobUnit (グリッド単位)

固定の 4 行グリッド (`UNIT_GRID` 定数):

```css
display: grid;
grid-template-rows: 10px 40px 10px 12px;  /* label / control / value / chips */
row-gap: 3px;
align-items: center;
justify-items: center;
```

これがすべての整列の基盤。**全ユニットがこのグリッドを使う**ことで、行内の
ラベル・コントロール中心・値・モッドチップが完全に揃う。

### 6.4 ModChip

```
display: inline-flex
height: 12px
padding: 0 4px
border: 1px solid accent
border-radius: 1px
color: accent
font: IBM Plex Mono 7.5px / 500, letter-spacing 0.05em
background: transparent
```

ラベル例: `E1` `E2` `L1` `L2` `V1` `V2` `P1` `P2` `MW` `AT`

### 6.5 Waveform Preview

SVG 88×36 + 外枠 (rect 0.5,0.5,87,35 stroke hairFine)。
中央水平の点線 (`stroke-dasharray="2 2"`)、波形パスは accent カラー stroke 1.5px。

5 種類のパス: `sine` / `saw` / `square` / `tri` / `noise` (詳細は実装ファイル参照)。

両側に `‹` `›` の選択矢印 (IBM Plex Mono 11px, color inkDim)。

### 6.6 HSliderUnit (横スライダー)

幅は呼び出し側指定 (デフォルト 68px)、高さ 12px。
- 外枠: stroke knobStroke 1px
- 内部ティック × 5 (0, .25, .5, .75, 1)。.5 は hairline、それ以外 hairFine
- Thumb: 6×16px (上下に -2px 飛び出す)、fill accent

### 6.7 SelectorUnit (ドロップダウン)

```
border: 1px solid knobStroke
padding: 4px 10px 4px 7px
font: IBM Plex Mono 9.5px, color ink
右端に ▾ (7px, color inkDim) を絶対配置
```

### 6.8 InlineField (BANK / PROGRAM フィールド)

```
border: 1px solid knobStroke
padding: 5px 8px
font: IBM Plex Mono 10px
右端に ▾ (8px, color inkDim) — オプション
```

### 6.9 LineButton (STORE / RECALL etc.)

```
border: 1px solid knobStroke
padding: 5px 10px
font: IBM Plex Mono 9.5px UPPERCASE, letter-spacing 0.18em
text-align: center
flex: 1 (横並び)
```

### 6.10 TabStrip (上部モジュールタブ)

各項目: padding 7px 18px, border-right hairFine 1px。
アクティブ項目: color accent, border-bottom 1px solid accent (-1px margin で被せる),
font-weight 600。非アクティブ: color inkDim, weight 400。

項目順: `Program`, `Osc`, `Filter`, `Env`, `Mod`, `FX`, `Patch` (デフォルト active=1 → "Osc")

### 6.11 PresetFooter (下部プリセット)

ラベル "PRESETS" + 各項目 (番号 + 名前)。各番号は `01`, `02`, ... の zero-padded。

項目順:
1. `Lush Strings` (active)
2. `Glass Pad`
3. `Mono Lead`
4. `Brass Stack`
5. `Polysynth Bell`
6. `FM Pluck`

### 6.12 Keyboard

- 鍵盤エリア左に **オクターブニアジェ** (`oct C3` ラベル + `‹` `›` ボタン×2)
- 上部に **目盛り段** (各白鍵中心に短ティック、C 鍵だけ長ティック)
- 白鍵 × 50 (C1〜C8): border keyStroke 1px, background `rgba(255,250,235,0.35)`
  - C 鍵のみ下部に `C1` `C2`... の小ラベル (IBM Plex Mono 7.5px)
- 黒鍵 × 35: 各オクターブで C#, D#, F#, G#, A# の位置にだけ。
  幅 = 白鍵幅 × 0.64、高さ = 鍵盤高さの 60%、background ink (#1a2538)
- 下部に **寸法線**: `├ ─ ─ ─ C1 — C8 · 50 keys ─ ─ ─ ┤` (dashed, hairFine)

### 6.13 TitleBlock (下端の図面情報)

6 セル横並び (TITLE / SERIES / SCALE / SHEET / REV / DATE):
- TITLE と SERIES は flex 2、他は flex 1
- 各セル: padding 8px 14px、右に hairFine 1px (最後を除く)
- 上にラベル (7.5px, letter-spacing 0.2em, uppercase, color inkDim)
- 下に値 (11px, color ink, letter-spacing 0.04em)

値:
- TITLE: `HAIRLINE-VIII`
- SERIES: `No. 0427 · rev. C`
- SCALE: `1 : 1`
- SHEET: `01 / 01`
- REV: `C`
- DATE: `2026·05`

---

## 7. 全 13 セルの正確なコントロール一覧

| # | Cell | flex | Controls (順序通り) |
|---|---|---|---|
| 01 | VCO 1 | 1.45 | WAVE (waveform: saw) · OCT (hslider 0.5, "+0") · PW (knob 0.50) |
| 02 | VCO 2 | 1.65 | WAVE (waveform: tri) · OCT (hslider 0.4, "−1") · PW (knob 0.50) · DETUNE (knob 0.12) |
| 03 | X-MOD / SYNC | 0.80 | X-MOD (knob 0.00) · SYNC (toggle) |
| 04 | MIXER | 1.10 | VCO 1 (knob 0.70) · VCO 2 (knob 0.70) · NOISE (knob 0.00) |
| 05 | FILTER | 1.70 | CUTOFF (knob 0.55, mods: E2 L1 V1) · RES (knob 0.50, mods: E2) · ENV (knob 0.64) · LFO (knob 0.00) · KBD (knob 0.55) · SLOPE (selector "2-POLE") |
| 06 | FILTER ENV | 1.30 | A (0.20) · D (0.56) · S (0.62) · R (0.78) |
| 07 | AMP ENV | 1.30 | A (0.26) · D (0.50) · S (0.85) · R (0.82) |
| 08 | LFO | 1.95 | SHAPE (waveform: sine) · RATE (0.50) · → VCO 1 (0.18) · → VCO 2 (0.18) · → PWM (0.00) |
| 09 | VELOCITY | 0.95 | → VCA (0.00) · → VCF (0.00) (両方 size 36) |
| 10 | VOICE | 2.00 | MODE (selector "POLY") · UNI·DET (0.06) · DRIFT (0.06) · TUNE (0.50) · BEND (0.70) · GLIDE (0.05) · VOLUME (0.65) · HOLD (toggle) |
| 11 | PAGE 2 · ENVELOPE & AT/MW | 2.05 | (2行) Row 1: FE→V1, FE→V2, FE→PW, AT→VCF, AT→LFO (各 0.0) · Row 2: AT→VCA, MW→VCF, MW→LFO, MW→VIB (各 0.0), KEY SYN (toggle) |
| 12 | SPLIT / DOUBLE | 1.00 | (2行) Row 1: SPLIT (0.50), S OCT (selector "0") · Row 2: S DET (0.00), D DET (0.10) |
| 13 | PATCH BANK | 1.55 | (2行) Row 1: BANK (field "Bank A"), PROGRAM (field "3 · Lush Strings") · Row 2: Store, Recall, Save, Load (LineButton ×4) |

`flex` 値は CSS Flexbox の `flex-grow` 相当。Band 内のセル幅比率。

---

## 8. Variants

### A. HAIRLINE-VIII Knob Edition
- `controlMode: 'knob'` (デフォルト)
- すべての主要コントロールは `RefinedKnob`

### B. HAIRLINE-VIII Fader Edition
- `controlMode: 'fader'`
- `RefinedKnob` 表示箇所がすべて `RefinedFader` に置換される
- `Series` 表記: `No. 0427 · rev. C · fader cut`
- `Tagline`: `eight-voice polyphonic · slider edition`
- 他のテーマトークンは Knob Edition と同一

実装パターン:
```tsx
function Control({ value, theme, size = 36 }) {
  if (theme.controlMode === 'fader')
    return <RefinedFader value={value} theme={theme} height={Math.max(40, size + 8)} />;
  return <RefinedKnob value={value} size={size} theme={theme} />;
}
```

---

## 9. State Management

このデザインは現状 **静的モック** (パラメータ値は固定)。実機接続時に必要な state:

```ts
interface SynthState {
  vco1: { wave: 'saw'|'tri'|'square'|'sine'|'noise'; oct: -2..2; pw: 0..1 };
  vco2: { wave; oct; pw; detune: -1..1 };
  xmod: number; sync: boolean;
  mixer: { vco1: 0..1; vco2: 0..1; noise: 0..1 };
  filter: { cutoff; res; env; lfo; kbd; slope: '2-pole'|'4-pole' };
  filterEnv: { a; d; s; r };
  ampEnv: { a; d; s; r };
  lfo: { shape; rate; toVco1; toVco2; toPwm };
  velocity: { toVca; toVcf };
  voice: { mode: 'poly'|'mono'|'unison'; uniDet; drift; tune; bend; glide; volume; hold };
  page2: { feToV1, feToV2, feToPw, atToVcf, atToLfo, atToVca, mwToVcf, mwToLfo, mwToVib; keySyn };
  split: { split; sOct; sDet; dDet };
  patch: { bank: string; program: { idx: number; name: string } };
}
```

各 0..1 ノブの値が変化するとインジケータ角度: `angle = -135° + value × 270°`。

ホスト DAW との接続は範囲外。MIDI / Web Audio が必要な場合は別途設計。

---

## 10. Interactions (今後の実装時)

現状は静的だが、再現時には以下を想定:
- ノブ: 縦ドラッグで値変更 (1 px ≈ 1/200 step)、Shift で微調整、ダブルクリックでリセット
- フェーダー: 縦ドラッグ、Shift 微調整、ダブルクリックリセット
- セレクタ: クリックでメニュー展開
- タブ: クリックでアクティブ切替 (現状は装飾)
- プリセット: クリックでロード
- 鍵盤: クリックで発音、ドラッグで連続発音
- ‹ › ボタン: 該当パラメータをサイクル

トランジション: すべて 120ms ease-out。色遷移は accent (#a23a1a) を維持。

---

## 11. Files in this Bundle

| File | 役割 |
|---|---|
| `Synth UI Refined.html` | エントリポイント。Design Canvas で 4 アートボードを並べる |
| `design-canvas.jsx` | Design Canvas 共通コンポーネント (参考。実装には不要) |
| `themes-refined.jsx` | 全テーマトークン定義 (Hairline / Hairline Fader / Terminal / Cyanotype) |
| `synth-controls-refined.jsx` | アトミックコンポーネント定義 (Knob/Fader/Waveform/Chip/Tab/Footer/PaperOverlay 等) |
| `synth-panel-refined.jsx` | パネル全体の組み立て (Header / TabStrip / 3 Bands / PresetFooter / Keyboard / TitleBlock) |
| `README.md` | このファイル |

実装の際は `themes-refined.jsx` と `synth-panel-refined.jsx` を読むのが最も速い。
React + JSX は参考実装であり、最終言語/フレームワークは自由。

---

## 12. Acceptance Criteria (再現完了の判定)

- [ ] 全寸法・色・字間がトークン表通りである
- [ ] 4 段 (Header / 3 Bands / Footer / Keyboard / TitleBlock) の境界は **1px hairline**
- [ ] 各 Band 内のセルは **min-height 統一** (Band 1, 2 = 116px / Band 3 = 196px)
- [ ] 各 KnobUnit はグリッド 10/40/10/12 を守り、行内で完全整列
- [ ] 紙テクスチャは 4 レイヤー (halo / fiber / grain / vignette) + DRAFT 透かし
- [ ] 四隅にレジストレーションマーク (十字 + 円)
- [ ] Knob Edition と Fader Edition がトグル可能で、それ以外は同一
- [ ] 鍵盤 50 白鍵 + 35 黒鍵、C 鍵のみラベル表示
- [ ] TitleBlock の値は仕様表通り (HAIRLINE-VIII / No. 0427 · rev. C / 1:1 / 01/01 / C / 2026·05)
- [ ] 角丸はゼロ (ModChip の 1px を除く)

---

## 13. 質問があれば

このパッケージは self-contained。不明点があれば Claude Code に
「`README.md` を読んで実装してください」と伝えるだけで再現できます。
