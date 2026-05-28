// synth-panel-refined.jsx — drafting-sheet layout with paper overlay & aligned grid

function Cell({ index, label, theme, children, flex = 1, padX = 14, padY = 12, noRight = false, minHeight }) {
  return (
    <div style={{
      flex,
      padding: `${padY}px ${padX}px`,
      borderRight: noRight ? 'none' : `1px solid ${theme.hairFine}`,
      minWidth: 0,
      minHeight,
      display: 'flex',
      flexDirection: 'column',
      gap: 10,
    }}>
      <div style={{
        display: 'flex',
        alignItems: 'baseline',
        gap: 6,
        fontFamily: theme.fontUI,
        fontSize: 9,
        letterSpacing: '0.2em',
        textTransform: 'uppercase',
        color: theme.ink,
        lineHeight: 1,
      }}>
        {index !== undefined && (
          <span style={{ color: theme.accent, fontWeight: 600 }}>{String(index).padStart(2, '0')}</span>
        )}
        <span>{label}</span>
      </div>
      <div style={{ display: 'flex', flexDirection: 'column', gap: 8, flex: 1, justifyContent: 'flex-start' }}>
        {children}
      </div>
    </div>
  );
}

function Band({ theme, children, lastRow, minHeight }) {
  return (
    <div style={{
      display: 'flex',
      borderBottom: lastRow ? 'none' : `1px solid ${theme.hairline}`,
      minHeight,
      alignItems: 'stretch',
    }}>
      {children}
    </div>
  );
}

function KnobRow({ children, gap = 14, align = 'flex-end' }) {
  return (
    <div style={{
      display: 'flex',
      gap,
      alignItems: align,
      justifyContent: 'flex-start',
    }}>{children}</div>
  );
}

function Crosshair({ size = 10, color }) {
  return (
    <svg width={size * 2} height={size * 2} style={{ display: 'block' }}>
      <line x1={size} y1="0" x2={size} y2={size * 2} stroke={color} strokeWidth="1" />
      <line x1="0" y1={size} x2={size * 2} y2={size} stroke={color} strokeWidth="1" />
      <circle cx={size} cy={size} r="3.5" fill="none" stroke={color} strokeWidth="1" />
    </svg>
  );
}

function RefinedKeyboard({ theme, octaveLabel = 'C3' }) {
  const octaves = 7;
  const whites = [];
  for (let oct = 1; oct <= octaves; oct++) {
    for (const n of ['C', 'D', 'E', 'F', 'G', 'A', 'B']) {
      whites.push({ note: n, oct, label: n === 'C' ? `C${oct}` : null });
    }
  }
  whites.push({ note: 'C', oct: 8, label: 'C8' });
  const blackOffsets = [0, 1, 3, 4, 5];
  const blacks = [];
  for (let oct = 0; oct < octaves; oct++) {
    for (const off of blackOffsets) blacks.push(oct * 7 + off);
  }
  const whiteW = 100 / whites.length;

  return (
    <div style={{
      display: 'flex',
      gap: 14,
      padding: '14px 18px 16px',
      alignItems: 'stretch',
    }}>
      <div style={{ display: 'flex', flexDirection: 'column', justifyContent: 'center', gap: 6, minWidth: 64 }}>
        <div style={{ fontFamily: theme.fontMono, fontSize: 10, color: theme.ink, letterSpacing: '0.1em' }}>
          oct {octaveLabel}
        </div>
        <div style={{ display: 'flex', gap: 4 }}>
          <div style={{ width: 22, height: 22, border: `1px solid ${theme.knobStroke}`, display: 'flex', alignItems: 'center', justifyContent: 'center', fontFamily: theme.fontMono, fontSize: 10, color: theme.ink, background: 'transparent' }}>‹</div>
          <div style={{ width: 22, height: 22, border: `1px solid ${theme.knobStroke}`, display: 'flex', alignItems: 'center', justifyContent: 'center', fontFamily: theme.fontMono, fontSize: 10, color: theme.ink, background: 'transparent' }}>›</div>
        </div>
      </div>

      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', gap: 4 }}>
        <div style={{ position: 'relative', height: 10 }}>
          {whites.map((k, i) => (
            <div key={i} style={{
              position: 'absolute',
              left: `${i * whiteW + whiteW / 2}%`,
              top: 0,
              transform: 'translateX(-50%)',
              width: 1,
              height: k.note === 'C' ? 10 : 5,
              background: theme.hairline,
            }} />
          ))}
        </div>
        <div style={{ position: 'relative', height: 70 }}>
          <div style={{ display: 'flex', height: '100%' }}>
            {whites.map((k, i) => (
              <div key={i} style={{
                flex: 1,
                border: `1px solid ${theme.keyStroke}`,
                borderLeft: i === 0 ? `1px solid ${theme.keyStroke}` : 'none',
                background: theme.keyFill,
                position: 'relative',
              }}>
                {k.label && (
                  <div style={{
                    position: 'absolute',
                    bottom: 3, left: 0, right: 0,
                    textAlign: 'center',
                    fontFamily: theme.fontMono,
                    fontSize: 7.5,
                    color: theme.inkDim,
                    letterSpacing: '0.05em',
                  }}>{k.label}</div>
                )}
              </div>
            ))}
          </div>
          {blacks.map((idx, i) => (
            <div key={i} style={{
              position: 'absolute',
              top: 0,
              left: `calc(${(idx + 1) * whiteW}% - ${whiteW * 0.32}%)`,
              width: `${whiteW * 0.64}%`,
              height: '60%',
              background: theme.blackKeyFill,
              border: `1px solid ${theme.keyStroke}`,
              zIndex: 2,
            }} />
          ))}
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginTop: 4, fontFamily: theme.fontMono, fontSize: 8, color: theme.inkDim, letterSpacing: '0.08em' }}>
          <span>├</span>
          <div style={{ flex: 1, borderTop: `1px dashed ${theme.hairFine}` }} />
          <span>C1 — C8 · 50 keys</span>
          <div style={{ flex: 1, borderTop: `1px dashed ${theme.hairFine}` }} />
          <span>┤</span>
        </div>
      </div>
    </div>
  );
}

function TitleBlock({ theme }) {
  return (
    <div style={{
      display: 'flex',
      borderTop: `1px solid ${theme.hairline}`,
      fontFamily: theme.fontMono,
      fontSize: 9.5,
      color: theme.ink,
      position: 'relative',
    }}>
      {[
        { k: 'TITLE', v: theme.name },
        { k: 'SERIES', v: theme.series },
        { k: 'SCALE', v: '1 : 1' },
        { k: 'SHEET', v: '01 / 01' },
        { k: 'REV', v: 'C' },
        { k: 'DATE', v: '2026·05' },
      ].map((c, i, arr) => (
        <div key={c.k} style={{
          flex: c.k === 'TITLE' || c.k === 'SERIES' ? 2 : 1,
          padding: '8px 14px',
          borderRight: i === arr.length - 1 ? 'none' : `1px solid ${theme.hairFine}`,
          display: 'flex',
          flexDirection: 'column',
          gap: 4,
        }}>
          <div style={{ fontSize: 7.5, letterSpacing: '0.2em', color: theme.inkDim, textTransform: 'uppercase' }}>{c.k}</div>
          <div style={{ fontSize: 11, color: theme.ink, letterSpacing: '0.04em' }}>{c.v}</div>
        </div>
      ))}
    </div>
  );
}

function RefinedSynthPanel({ theme }) {
  const T = theme;
  const knobSize = 36;
  const smallKnob = 30;
  const TABS = ['Program', 'Osc', 'Filter', 'Env', 'Mod', 'FX', 'Patch'];
  const PRESETS = ['Lush Strings', 'Glass Pad', 'Mono Lead', 'Brass Stack', 'Polysynth Bell', 'FM Pluck'];

  // Band heights tuned so all units (incl. mod-chip rows) align to a shared baseline.
  const BAND_H_SINGLE = 116;  // single internal row
  const BAND_H_DOUBLE = 196;  // two internal rows

  return (
    <div style={{
      background: T.appBg,
      fontFamily: T.fontUI,
      color: T.ink,
      width: 1280,
      minHeight: 860,
      padding: 36,
      boxSizing: 'border-box',
    }}>
      {/* paper sheet container */}
      <div style={{
        background: T.paperBg,
        boxShadow: T.paper
          ? '0 22px 50px -18px rgba(20,12,4,0.55), 0 4px 12px rgba(20,12,4,0.25), inset 0 0 0 1px rgba(60,40,15,0.08)'
          : `0 0 0 1px ${T.hairline}`,
        border: T.paper ? 'none' : `1px solid ${T.hairline}`,
        position: 'relative',
        overflow: 'hidden',
      }}>
        {/* paper texture layers */}
        {T.paper && <PaperOverlay />}

        {/* corner crosshairs — drawn on top of paper */}
        {[
          { top: 8, left: 8 },
          { top: 8, right: 8 },
          { bottom: 8, left: 8 },
          { bottom: 8, right: 8 },
        ].map((p, i) => (
          <div key={i} style={{ position: 'absolute', pointerEvents: 'none', zIndex: 3, ...p }}>
            <Crosshair size={6} color={T.hairline} />
          </div>
        ))}

        {/* content layer */}
        <div style={{ position: 'relative', zIndex: 2 }}>

          {/* HEADER STRIP */}
          <div style={{
            display: 'flex',
            alignItems: 'baseline',
            justifyContent: 'space-between',
            padding: '18px 26px 14px',
            borderBottom: `1px solid ${T.hairline}`,
          }}>
            <div style={{ display: 'flex', alignItems: 'baseline', gap: 18 }}>
              <div style={{
                fontFamily: T.fontDisplay,
                fontSize: 30,
                fontWeight: 600,
                letterSpacing: '0.02em',
                color: T.ink,
                lineHeight: 1,
              }}>{T.name}</div>
              <div style={{ fontFamily: T.fontMono, fontSize: 9.5, color: T.accent, letterSpacing: '0.12em', textTransform: 'uppercase' }}>
                {T.series}
              </div>
            </div>
            <div style={{
              fontFamily: T.fontMono,
              fontSize: 10,
              color: T.inkDim,
              letterSpacing: '0.08em',
            }}>{T.tagline}</div>
          </div>

          <TabStrip items={TABS} active={1} theme={T} />

          {/* BAND 1 — 6 cells, single internal row */}
          <Band theme={T} minHeight={BAND_H_SINGLE}>
            <Cell index={1} label="VCO 1" theme={T} flex={1.45}>
              <KnobRow>
                <WaveformUnit label="WAVE" type="saw" theme={T} value="saw · 0" />
                <HSliderUnit label="OCT" value={0.5} theme={T} width={56} value_text="+0" />
                <KnobUnit label="PW" value={0.50} theme={T} size={smallKnob} />
              </KnobRow>
            </Cell>
            <Cell index={2} label="VCO 2" theme={T} flex={1.65}>
              <KnobRow>
                <WaveformUnit label="WAVE" type="tri" theme={T} value="tri · −2" />
                <HSliderUnit label="OCT" value={0.4} theme={T} width={56} value_text="−1" />
                <KnobUnit label="PW" value={0.50} theme={T} size={smallKnob} />
                <KnobUnit label="DETUNE" value={0.12} theme={T} size={smallKnob} />
              </KnobRow>
            </Cell>
            <Cell index={3} label="X-MOD / SYNC" theme={T} flex={0.80}>
              <KnobRow gap={10}>
                <KnobUnit label="X-MOD" value={0.00} theme={T} size={smallKnob} />
                <ToggleSquare label="SYNC" theme={T} />
              </KnobRow>
            </Cell>
            <Cell index={4} label="MIXER" theme={T} flex={1.10}>
              <KnobRow>
                <KnobUnit label="VCO 1" value={0.70} theme={T} size={smallKnob} />
                <KnobUnit label="VCO 2" value={0.70} theme={T} size={smallKnob} />
                <KnobUnit label="NOISE" value={0.00} theme={T} size={smallKnob} />
              </KnobRow>
            </Cell>
            <Cell index={5} label="FILTER" theme={T} flex={1.70}>
              <KnobRow>
                <KnobUnit label="CUTOFF" value={0.55} theme={T} size={knobSize} mods={['E2', 'L1', 'V1']} />
                <KnobUnit label="RES" value={0.50} theme={T} size={knobSize} mods={['E2']} />
                <KnobUnit label="ENV" value={0.64} theme={T} size={knobSize} />
                <KnobUnit label="LFO" value={0.00} theme={T} size={smallKnob} />
                <KnobUnit label="KBD" value={0.55} theme={T} size={smallKnob} />
                <SelectorUnit label="SLOPE" value="2-POLE" theme={T} width={68} />
              </KnobRow>
            </Cell>
            <Cell index={6} label="FILTER ENV" theme={T} flex={1.30} noRight>
              <KnobRow>
                <KnobUnit label="A" value={0.20} theme={T} size={smallKnob} />
                <KnobUnit label="D" value={0.56} theme={T} size={smallKnob} />
                <KnobUnit label="S" value={0.62} theme={T} size={smallKnob} />
                <KnobUnit label="R" value={0.78} theme={T} size={smallKnob} />
              </KnobRow>
            </Cell>
          </Band>

          {/* BAND 2 — 4 cells */}
          <Band theme={T} minHeight={BAND_H_SINGLE}>
            <Cell index={7} label="AMP ENV" theme={T} flex={1.30}>
              <KnobRow>
                <KnobUnit label="A" value={0.26} theme={T} size={smallKnob} />
                <KnobUnit label="D" value={0.50} theme={T} size={smallKnob} />
                <KnobUnit label="S" value={0.85} theme={T} size={smallKnob} />
                <KnobUnit label="R" value={0.82} theme={T} size={smallKnob} />
              </KnobRow>
            </Cell>
            <Cell index={8} label="LFO" theme={T} flex={1.95}>
              <KnobRow>
                <WaveformUnit label="SHAPE" type="sine" theme={T} value="sine · L1" />
                <KnobUnit label="RATE" value={0.50} theme={T} size={smallKnob} />
                <KnobUnit label="→ VCO 1" value={0.18} theme={T} size={smallKnob} />
                <KnobUnit label="→ VCO 2" value={0.18} theme={T} size={smallKnob} />
                <KnobUnit label="→ PWM" value={0.00} theme={T} size={smallKnob} />
              </KnobRow>
            </Cell>
            <Cell index={9} label="VELOCITY" theme={T} flex={0.95}>
              <KnobRow gap={16}>
                <KnobUnit label="→ VCA" value={0.00} theme={T} size={knobSize} />
                <KnobUnit label="→ VCF" value={0.00} theme={T} size={knobSize} />
              </KnobRow>
            </Cell>
            <Cell index={10} label="VOICE" theme={T} flex={2.00} noRight>
              <KnobRow>
                <SelectorUnit label="MODE" value="POLY" theme={T} width={68} />
                <KnobUnit label="UNI·DET" value={0.06} theme={T} size={smallKnob} />
                <KnobUnit label="DRIFT" value={0.06} theme={T} size={smallKnob} />
                <KnobUnit label="TUNE" value={0.50} theme={T} size={smallKnob} />
                <KnobUnit label="BEND" value={0.70} theme={T} size={smallKnob} />
                <KnobUnit label="GLIDE" value={0.05} theme={T} size={smallKnob} />
                <KnobUnit label="VOLUME" value={0.65} theme={T} size={smallKnob} />
                <ToggleSquare label="HOLD" theme={T} />
              </KnobRow>
            </Cell>
          </Band>

          {/* BAND 3 — 3 cells, two internal rows each */}
          <Band theme={T} minHeight={BAND_H_DOUBLE}>
            <Cell index={11} label="PAGE 2 · ENVELOPE & AT/MW" theme={T} flex={2.05}>
              <KnobRow>
                <KnobUnit label="FE→V1" value={0.0} theme={T} size={smallKnob} />
                <KnobUnit label="FE→V2" value={0.0} theme={T} size={smallKnob} />
                <KnobUnit label="FE→PW" value={0.0} theme={T} size={smallKnob} />
                <KnobUnit label="AT→VCF" value={0.0} theme={T} size={smallKnob} />
                <KnobUnit label="AT→LFO" value={0.0} theme={T} size={smallKnob} />
              </KnobRow>
              <KnobRow>
                <KnobUnit label="AT→VCA" value={0.0} theme={T} size={smallKnob} />
                <KnobUnit label="MW→VCF" value={0.0} theme={T} size={smallKnob} />
                <KnobUnit label="MW→LFO" value={0.0} theme={T} size={smallKnob} />
                <KnobUnit label="MW→VIB" value={0.0} theme={T} size={smallKnob} />
                <ToggleSquare label="KEY SYN" theme={T} />
              </KnobRow>
            </Cell>
            <Cell index={12} label="SPLIT / DOUBLE" theme={T} flex={1.00}>
              <KnobRow>
                <KnobUnit label="SPLIT" value={0.50} theme={T} size={smallKnob} />
                <SelectorUnit label="S OCT" value="0" theme={T} width={48} />
              </KnobRow>
              <KnobRow>
                <KnobUnit label="S DET" value={0.00} theme={T} size={smallKnob} />
                <KnobUnit label="D DET" value={0.10} theme={T} size={smallKnob} />
              </KnobRow>
            </Cell>
            <Cell index={13} label="PATCH BANK" theme={T} flex={1.55} noRight>
              <div style={{ display: 'flex', gap: 10, alignItems: 'flex-end' }}>
                <InlineField label="BANK" value="Bank A" theme={T} width={96} hasArrow />
                <InlineField label="PROGRAM" value="3 · Lush Strings" theme={T} width={150} hasArrow />
              </div>
              <div style={{ display: 'flex', gap: 6 }}>
                <LineButton theme={T}>Store</LineButton>
                <LineButton theme={T}>Recall</LineButton>
                <LineButton theme={T}>Save</LineButton>
                <LineButton theme={T}>Load</LineButton>
              </div>
            </Cell>
          </Band>

          <PresetFooter items={PRESETS} active={0} theme={T} />

          <div style={{ borderBottom: `1px solid ${T.hairline}`, position: 'relative' }}>
            <RefinedKeyboard theme={T} octaveLabel="C3" />
          </div>

          <TitleBlock theme={T} />
        </div>
      </div>
    </div>
  );
}

Object.assign(window, { RefinedSynthPanel, RefinedKeyboard, TitleBlock });
