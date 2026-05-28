// synth-controls-refined.jsx — outline knobs + tight monospace label/value units
// All units share a fixed grid (label / control / value / mods) so a KnobRow
// aligns labels, knob centers, values, and chip rows across cells.

const UNIT_GRID = {
  display: 'grid',
  gridTemplateRows: '10px 40px 10px 12px',
  rowGap: 3,
  alignItems: 'center',
  justifyItems: 'center',
};

const labelStyle = (theme) => ({
  fontFamily: theme.fontUI,
  fontSize: 8.5,
  letterSpacing: '0.14em',
  color: theme.ink,
  textTransform: 'uppercase',
  lineHeight: 1,
  whiteSpace: 'nowrap',
});

const valueStyle = (theme) => ({
  fontFamily: theme.fontMono,
  fontSize: 8.5,
  color: theme.inkDim,
  letterSpacing: '0.02em',
  lineHeight: 1,
  whiteSpace: 'nowrap',
});

function RefinedKnob({ value = 0.5, size = 36, theme }) {
  const angle = -135 + value * 270;
  return (
    <svg width={size} height={size} viewBox="0 0 100 100" style={{ display: 'block', overflow: 'visible' }}>
      <path d="M 21.7,78.3 A 40,40 0 1 1 78.3,78.3" fill="none" stroke={theme.hairFine} strokeWidth="1" />
      {[-135, 135].map((a) => {
        const rad = (a - 90) * Math.PI / 180;
        const x1 = 50 + Math.cos(rad) * 40;
        const y1 = 50 + Math.sin(rad) * 40;
        const x2 = 50 + Math.cos(rad) * 46;
        const y2 = 50 + Math.sin(rad) * 46;
        return <line key={a} x1={x1} y1={y1} x2={x2} y2={y2} stroke={theme.hairline} strokeWidth="1.1" />;
      })}
      <line x1="50" y1="6" x2="50" y2="12" stroke={theme.hairline} strokeWidth="1" />
      <circle cx="50" cy="50" r="30" fill={theme.knobFill} stroke={theme.knobStroke} strokeWidth="1.4" />
      <g transform={`rotate(${angle} 50 50)`}>
        <line x1="50" y1="50" x2="50" y2="22" stroke={theme.knobTick} strokeWidth="2.2" strokeLinecap="round" />
        <circle cx="50" cy="50" r="2.2" fill={theme.knobTick} />
      </g>
    </svg>
  );
}

// Drafting-style vertical fader — thin slot, tick ladder, accent thumb bar.
function RefinedFader({ value = 0.5, theme, height = 40, slotWidth = 4 }) {
  const W = 26;
  const top = (1 - value) * (height - 4);
  const cx = W / 2;
  const slotX = cx - slotWidth / 2;
  const ticks = [0, 0.25, 0.5, 0.75, 1];
  return (
    <svg width={W} height={height + 4} viewBox={`0 0 ${W} ${height + 4}`} style={{ display: 'block', overflow: 'visible' }}>
      {ticks.map((t) => {
        const y = 2 + t * (height - 2);
        const major = t === 0 || t === 0.5 || t === 1;
        return (
          <g key={t}>
            <line x1={cx - 11} y1={y} x2={cx - (major ? 7 : 8.5)} y2={y} stroke={theme.hairline} strokeWidth="1" />
            <line x1={cx + (major ? 7 : 8.5)} y1={y} x2={cx + 11} y2={y} stroke={theme.hairline} strokeWidth="1" />
          </g>
        );
      })}
      <rect x={slotX} y={2} width={slotWidth} height={height} fill="transparent" stroke={theme.knobStroke} strokeWidth="1" />
      <line x1={slotX + 0.5} y1={2 + height / 2} x2={slotX + slotWidth - 0.5} y2={2 + height / 2} stroke={theme.hairFine} strokeWidth="1" />
      <g transform={`translate(0, ${2 + top})`}>
        <rect x={cx - 10} y={0} width={20} height={4} fill={theme.knobTick} />
        <line x1={cx - 10} y1={2} x2={cx + 10} y2={2} stroke="rgba(0,0,0,0.35)" strokeWidth="0.6" />
      </g>
    </svg>
  );
}

// Picks the right control based on theme.controlMode (defaults to 'knob').
function Control({ value = 0.5, theme, size = 36 }) {
  const mode = theme.controlMode || 'knob';
  if (mode === 'fader') {
    return <RefinedFader value={value} theme={theme} height={Math.max(40, size + 8)} />;
  }
  return <RefinedKnob value={value} size={size} theme={theme} />;
}

function ModChip({ label, theme, tone }) {
  const color = tone || theme.accent;
  return (
    <span style={{
      display: 'inline-flex',
      alignItems: 'center',
      justifyContent: 'center',
      padding: '0 4px',
      height: 12,
      border: `1px solid ${color}`,
      color,
      fontFamily: theme.fontMono,
      fontSize: 7.5,
      letterSpacing: '0.05em',
      fontWeight: 500,
      lineHeight: 1,
      borderRadius: 1,
      background: 'transparent',
      whiteSpace: 'nowrap',
    }}>{label}</span>
  );
}

function ChipRow({ mods, theme }) {
  return (
    <div style={{ display: 'flex', gap: 2, alignItems: 'center', height: 12 }}>
      {(mods || []).map((m, i) => <ModChip key={i} label={m} theme={theme} />)}
    </div>
  );
}

// KnobUnit — single API. mods are optional, but the chip row is always
// reserved (12px) so units in a row share baselines.
function KnobUnit({ label, value, theme, size = 36, mods = [] }) {
  const v = typeof value === 'number' ? value : Math.min(0.95, Math.max(0.05, parseFloat(value) || 0.5));
  return (
    <div style={{ ...UNIT_GRID, minWidth: 54 }}>
      <div style={labelStyle(theme)}>{label}</div>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <Control value={v} size={size} theme={theme} />
      </div>
      <div style={valueStyle(theme)}>
        {typeof value === 'number' ? value.toFixed(2) : (value ?? '')}
      </div>
      <ChipRow mods={mods} theme={theme} />
    </div>
  );
}

// KnobUnitMod kept as alias for backward compatibility
const KnobUnitMod = KnobUnit;

// Compact pill selector
function SelectorUnit({ label, value, theme, width = 70, mods = [] }) {
  return (
    <div style={{ ...UNIT_GRID, minWidth: width }}>
      <div style={labelStyle(theme)}>{label}</div>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <div style={{
          border: `1px solid ${theme.knobStroke}`,
          padding: '4px 10px 4px 7px',
          fontFamily: theme.fontMono,
          fontSize: 9.5,
          color: theme.ink,
          width,
          textAlign: 'left',
          position: 'relative',
          background: 'transparent',
          boxSizing: 'border-box',
        }}>
          {value}
          <span style={{
            position: 'absolute',
            right: 4, top: '50%', transform: 'translateY(-50%)',
            fontSize: 7, color: theme.inkDim,
          }}>▾</span>
        </div>
      </div>
      <div style={valueStyle(theme)}></div>
      <ChipRow mods={mods} theme={theme} />
    </div>
  );
}

// Toggle square fitting into the unit grid
function ToggleSquare({ label, theme, size = 18 }) {
  return (
    <div style={{ ...UNIT_GRID, minWidth: 36 }}>
      <div style={labelStyle(theme)}>{label}</div>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <div style={{ width: size, height: size, border: `1px solid ${theme.knobStroke}` }} />
      </div>
      <div style={valueStyle(theme)}></div>
      <ChipRow mods={[]} theme={theme} />
    </div>
  );
}

// Waveform paths
function Waveform({ type = 'sine', theme, width = 88, height = 36, color, withFrame = true }) {
  const stroke = color || theme.knobTick;
  const paths = {
    sine:   'M 4,18 Q 12,2 20,18 T 36,18 T 52,18 T 68,18 T 84,18',
    saw:    'M 4,30 L 14,6 L 14,30 L 24,6 L 24,30 L 34,6 L 34,30 L 44,6 L 44,30 L 54,6 L 54,30 L 64,6 L 64,30 L 74,6 L 74,30 L 84,6',
    square: 'M 4,30 L 4,6 L 14,6 L 14,30 L 24,30 L 24,6 L 34,6 L 34,30 L 44,30 L 44,6 L 54,6 L 54,30 L 64,30 L 64,6 L 74,6 L 74,30 L 84,30',
    tri:    'M 4,18 L 14,4 L 24,32 L 34,4 L 44,32 L 54,4 L 64,32 L 74,4 L 84,18',
    noise:  'M 4,22 L 8,12 L 12,28 L 16,8 L 20,24 L 24,14 L 28,30 L 32,10 L 36,26 L 40,16 L 44,8 L 48,22 L 52,14 L 56,28 L 60,18 L 64,10 L 68,26 L 72,14 L 76,22 L 80,12 L 84,18',
  };
  return (
    <svg viewBox="0 0 88 36" width={width} height={height} style={{ display: 'block' }}>
      {withFrame && <rect x="0.5" y="0.5" width="87" height="35" fill="none" stroke={theme.hairFine} strokeWidth="1" />}
      <line x1="0" y1="18" x2="88" y2="18" stroke={theme.hairFine} strokeWidth="1" strokeDasharray="2 2" />
      <path d={paths[type]} stroke={stroke} strokeWidth="1.5" fill="none" />
    </svg>
  );
}

function WaveformUnit({ label, type, theme, value, mods = [] }) {
  return (
    <div style={{ ...UNIT_GRID, minWidth: 110, gridTemplateRows: '10px 40px 10px 12px' }}>
      <div style={labelStyle(theme)}>{label}</div>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 4 }}>
        <div style={{ color: theme.inkDim, fontFamily: theme.fontMono, fontSize: 11, userSelect: 'none' }}>‹</div>
        <Waveform type={type} theme={theme} />
        <div style={{ color: theme.inkDim, fontFamily: theme.fontMono, fontSize: 11, userSelect: 'none' }}>›</div>
      </div>
      <div style={valueStyle(theme)}>{value ?? ''}</div>
      <ChipRow mods={mods} theme={theme} />
    </div>
  );
}

// Horizontal slider with center notch — fits into unit grid
function HSliderUnit({ label, value = 0.5, theme, width = 68, value_text, mods = [] }) {
  const x = value * (width - 6);
  return (
    <div style={{ ...UNIT_GRID, minWidth: width }}>
      <div style={labelStyle(theme)}>{label}</div>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <div style={{ width, height: 12, border: `1px solid ${theme.knobStroke}`, position: 'relative' }}>
          {[0, 0.25, 0.5, 0.75, 1].map((t) => (
            <div key={t} style={{
              position: 'absolute', top: -3, bottom: -3,
              left: `calc(${t * 100}% - 0.5px)`,
              width: 1,
              background: t === 0.5 ? theme.hairline : theme.hairFine,
            }} />
          ))}
          <div style={{
            position: 'absolute', top: -2, bottom: -2,
            left: x, width: 6, background: theme.knobTick,
          }} />
        </div>
      </div>
      <div style={valueStyle(theme)}>{value_text ?? ''}</div>
      <ChipRow mods={mods} theme={theme} />
    </div>
  );
}

// Legacy vertical slider — used by no-one now but kept for safety
function SliderUnit({ label, theme, value = 0.5, height = 40 }) {
  return <HSliderUnit label={label} theme={theme} value={value} width={56} />;
}

// Labeled inline field (used in PATCH BANK) — does not use the unit grid
function InlineField({ label, value, theme, width = 130, hasArrow }) {
  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 4, minWidth: 0 }}>
      <div style={labelStyle(theme)}>{label}</div>
      <div style={{
        border: `1px solid ${theme.knobStroke}`,
        padding: '5px 8px',
        fontFamily: theme.fontMono,
        fontSize: 10,
        color: theme.ink,
        width,
        boxSizing: 'border-box',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
      }}>
        <span style={{ whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{value}</span>
        {hasArrow && <span style={{ fontSize: 8, color: theme.inkDim }}>▾</span>}
      </div>
    </div>
  );
}

function LineButton({ children, theme }) {
  return (
    <div style={{
      border: `1px solid ${theme.knobStroke}`,
      padding: '5px 10px',
      fontFamily: theme.fontUI,
      fontSize: 9.5,
      letterSpacing: '0.18em',
      color: theme.ink,
      textTransform: 'uppercase',
      textAlign: 'center',
      flex: 1,
      lineHeight: 1.2,
      whiteSpace: 'nowrap',
    }}>{children}</div>
  );
}

function TabStrip({ items, active = 0, theme, accentTab = true }) {
  return (
    <div style={{ display: 'flex', borderBottom: `1px solid ${theme.hairline}`, background: 'transparent' }}>
      {items.map((it, i) => (
        <div key={i} style={{
          padding: '7px 18px',
          fontFamily: theme.fontUI,
          fontSize: 9,
          letterSpacing: '0.2em',
          textTransform: 'uppercase',
          color: i === active ? theme.accent : theme.inkDim,
          borderRight: `1px solid ${theme.hairFine}`,
          borderBottom: i === active && accentTab ? `1px solid ${theme.accent}` : 'none',
          marginBottom: i === active && accentTab ? -1 : 0,
          fontWeight: i === active ? 600 : 400,
          lineHeight: 1,
        }}>{it}</div>
      ))}
      <div style={{ flex: 1, borderBottom: `1px solid ${theme.hairFine}`, marginBottom: -1 }} />
    </div>
  );
}

function PresetFooter({ items, active = 0, theme }) {
  return (
    <div style={{
      display: 'flex',
      gap: 0,
      borderTop: `1px solid ${theme.hairline}`,
      borderBottom: `1px solid ${theme.hairline}`,
    }}>
      <div style={{ padding: '5px 14px', fontFamily: theme.fontMono, fontSize: 8.5, color: theme.inkDim, borderRight: `1px solid ${theme.hairFine}`, letterSpacing: '0.1em' }}>
        PRESETS
      </div>
      {items.map((it, i) => (
        <div key={i} style={{
          padding: '5px 12px',
          fontFamily: theme.fontUI,
          fontSize: 9,
          letterSpacing: '0.12em',
          textTransform: 'uppercase',
          color: i === active ? theme.accent : theme.inkDim,
          borderRight: `1px solid ${theme.hairFine}`,
          lineHeight: 1.2,
          display: 'flex',
          alignItems: 'center',
          gap: 6,
        }}>
          <span style={{ fontFamily: theme.fontMono, color: theme.inkFaint }}>{String(i + 1).padStart(2, '0')}</span>
          <span>{it}</span>
        </div>
      ))}
      <div style={{ flex: 1 }} />
    </div>
  );
}

// PaperOverlay — adds noise grain, sepia stains, vignette and fine fibers.
// Renders as absolutely-positioned siblings of the panel content. Wrap the
// panel inner in position:relative and put this BEFORE the content (zIndex on
// content is higher).
function PaperOverlay() {
  const noise = `url("data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 240 240'><filter id='n'><feTurbulence type='fractalNoise' baseFrequency='0.92' numOctaves='2' seed='5' stitchTiles='stitch'/><feColorMatrix values='0 0 0 0 0.20 0 0 0 0 0.14 0 0 0 0 0.06 0 0 0 0.62 0'/></filter><rect width='100%' height='100%' filter='url(%23n)'/></svg>")`;
  const layer = { position: 'absolute', inset: 0, pointerEvents: 'none' };
  return (
    <>
      {/* warm halo + stains */}
      <div style={{
        ...layer,
        background: `
          radial-gradient(ellipse 900px 600px at 18% 22%, rgba(180,130,60,0.07) 0%, transparent 60%),
          radial-gradient(ellipse 700px 500px at 84% 78%, rgba(120,70,30,0.09) 0%, transparent 55%),
          radial-gradient(ellipse 500px 300px at 62% 18%, rgba(220,180,110,0.10) 0%, transparent 60%),
          radial-gradient(ellipse 1100px 800px at 50% 50%, rgba(245,232,200,0.4) 0%, transparent 80%)
        `,
      }} />
      {/* fiber */}
      <div style={{
        ...layer,
        backgroundImage: `repeating-linear-gradient(91deg, rgba(60,35,12,0.03) 0 1px, transparent 1px 4px),
                          repeating-linear-gradient(2deg, rgba(60,35,12,0.018) 0 1px, transparent 1px 6px)`,
      }} />
      {/* grain noise */}
      <div style={{
        ...layer,
        backgroundImage: noise,
        backgroundSize: '240px 240px',
        opacity: 0.32,
        mixBlendMode: 'multiply',
      }} />
      {/* vignette */}
      <div style={{
        ...layer,
        background: 'radial-gradient(ellipse at center, transparent 55%, rgba(60,35,12,0.18) 100%)',
      }} />
      {/* faint diagonal "DRAFT" watermark */}
      <div style={{
        ...layer,
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        opacity: 0.05,
        transform: 'rotate(-18deg)',
        fontFamily: '"Fraunces", Georgia, serif',
        fontSize: 220,
        fontWeight: 700,
        letterSpacing: '0.04em',
        color: '#1a2538',
        userSelect: 'none',
      }}>DRAFT</div>
    </>
  );
}

// Small ink stamp — circular registration / approval stamp in accent
function InkStamp({ theme, text = 'APPR\u2019D', sub = '26·05·28', style }) {
  return (
    <div style={{
      width: 88, height: 88,
      borderRadius: '50%',
      border: `2px solid ${theme.accent}`,
      color: theme.accent,
      display: 'flex',
      flexDirection: 'column',
      alignItems: 'center',
      justifyContent: 'center',
      fontFamily: theme.fontUI,
      letterSpacing: '0.16em',
      transform: 'rotate(-8deg)',
      opacity: 0.78,
      pointerEvents: 'none',
      ...style,
    }}>
      <div style={{ fontSize: 16, fontWeight: 700, letterSpacing: '0.12em' }}>{text}</div>
      <div style={{ width: '60%', height: 1, background: theme.accent, margin: '4px 0', opacity: 0.6 }} />
      <div style={{ fontSize: 9, fontFamily: theme.fontMono }}>{sub}</div>
    </div>
  );
}

Object.assign(window, {
  RefinedKnob, RefinedFader, Control,
  KnobUnit, KnobUnitMod, SliderUnit, SelectorUnit, ToggleSquare, InlineField, LineButton,
  Waveform, WaveformUnit, ModChip, ChipRow, HSliderUnit, TabStrip, PresetFooter,
  PaperOverlay, InkStamp,
});
