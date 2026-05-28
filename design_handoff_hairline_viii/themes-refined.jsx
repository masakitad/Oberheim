// themes-refined.jsx — 3 refined directions rooted in blueprint/technical drawing language
// All use hairlines, monospace, tight density, no boxy section frames.

const themeHairline = {
  id: 'hairline',
  name: 'HAIRLINE-VIII',
  series: 'No. 0427 · rev. C',
  tagline: 'eight-voice polyphonic · drafted at 1:1',
  paper: true,
  controlMode: 'knob',
  // chrome / surface — darker, more leather/desk
  appBg: 'radial-gradient(ellipse at 50% 40%, #a8966c 0%, #786646 70%, #5a4a30 100%)',
  paperBg: '#efe6cc',  // base paper — slightly warmer cream
  paperBg2: '#e7dcbe',
  ink: '#1a2538',       // deep navy-brown ink
  inkDim: 'rgba(26,37,56,0.58)',
  inkFaint: 'rgba(26,37,56,0.20)',
  hairline: 'rgba(26,37,56,0.42)',
  hairFine: 'rgba(26,37,56,0.20)',
  accent: '#a23a1a',    // oxide-red technical-pen
  accentSoft: 'rgba(162,58,26,0.55)',
  // typography
  fontDisplay: '"Fraunces","Cormorant Garamond",Georgia,serif',
  fontUI: '"IBM Plex Mono", monospace',
  fontMono: '"IBM Plex Mono", monospace',
  // knobs
  knobStroke: 'rgba(26,37,56,0.88)',
  knobTick: '#a23a1a',
  knobFill: 'transparent',
  // keyboard
  keyStroke: 'rgba(26,37,56,0.82)',
  keyFill: 'rgba(255,250,235,0.35)',
  blackKeyFill: '#1a2538',
};

// Hairline · Fader edition — same ink/paper, vertical-slider controls
const themeHairlineFader = {
  ...{
    id: 'hairline',
    name: 'HAIRLINE-VIII',
    series: 'No. 0427 · rev. C · fader cut',
    tagline: 'eight-voice polyphonic · slider edition',
    paper: true,
    appBg: 'radial-gradient(ellipse at 50% 40%, #a8966c 0%, #786646 70%, #5a4a30 100%)',
    paperBg: '#efe6cc',
    paperBg2: '#e7dcbe',
    ink: '#1a2538',
    inkDim: 'rgba(26,37,56,0.58)',
    inkFaint: 'rgba(26,37,56,0.20)',
    hairline: 'rgba(26,37,56,0.42)',
    hairFine: 'rgba(26,37,56,0.20)',
    accent: '#a23a1a',
    accentSoft: 'rgba(162,58,26,0.55)',
    fontDisplay: '"Fraunces","Cormorant Garamond",Georgia,serif',
    fontUI: '"IBM Plex Mono", monospace',
    fontMono: '"IBM Plex Mono", monospace',
    knobStroke: 'rgba(26,37,56,0.88)',
    knobTick: '#a23a1a',
    knobFill: 'transparent',
    keyStroke: 'rgba(26,37,56,0.82)',
    keyFill: 'rgba(255,250,235,0.35)',
    blackKeyFill: '#1a2538',
  },
  id: 'hairline-fader',
  controlMode: 'fader',
};

const themeTerminal = {
  id: 'terminal',
  name: 'TERMINAL-Δ8',
  series: 'instance · 0x08',
  tagline: 'polyphonic · vector schematic · 8 voices',
  appBg: '#050608',
  paperBg: '#070a0c',
  paperBg2: '#090c10',
  ink: '#bfeee3',
  inkDim: 'rgba(94,234,212,0.55)',
  inkFaint: 'rgba(94,234,212,0.16)',
  hairline: 'rgba(94,234,212,0.45)',
  hairFine: 'rgba(94,234,212,0.18)',
  accent: '#ff8a3d',
  accentSoft: 'rgba(255,138,61,0.6)',
  fontDisplay: '"JetBrains Mono", "SF Mono", monospace',
  fontUI: '"JetBrains Mono", monospace',
  fontMono: '"JetBrains Mono", monospace',
  knobStroke: 'rgba(94,234,212,0.85)',
  knobTick: '#ff8a3d',
  knobFill: 'transparent',
  keyStroke: 'rgba(94,234,212,0.65)',
  keyFill: 'transparent',
  blackKeyFill: 'rgba(94,234,212,0.92)',
};

const themeCyanotype = {
  id: 'cyanotype',
  name: 'CYANOTYPE 8',
  series: 'sheet 04 of 12 · scale 1:1',
  tagline: 'eight-voice polyphonic — drafted in cyan & bone',
  appBg: '#0a2236',
  paperBg: '#0e2a40',
  paperBg2: '#0c2638',
  ink: '#e8eef4',
  inkDim: 'rgba(232,238,244,0.62)',
  inkFaint: 'rgba(232,238,244,0.22)',
  hairline: 'rgba(232,238,244,0.55)',
  hairFine: 'rgba(232,238,244,0.22)',
  accent: '#f4c75e',
  accentSoft: 'rgba(244,199,94,0.6)',
  fontDisplay: '"Fraunces", "IBM Plex Mono", monospace',
  fontUI: '"IBM Plex Mono", monospace',
  fontMono: '"IBM Plex Mono", monospace',
  knobStroke: 'rgba(232,238,244,0.85)',
  knobTick: '#f4c75e',
  knobFill: 'transparent',
  keyStroke: 'rgba(232,238,244,0.75)',
  keyFill: 'transparent',
  blackKeyFill: 'rgba(232,238,244,0.92)',
};

const REFINED_THEMES = [themeHairline, themeHairlineFader, themeTerminal, themeCyanotype];

Object.assign(window, { themeHairline, themeHairlineFader, themeTerminal, themeCyanotype, REFINED_THEMES });
