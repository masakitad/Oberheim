#include "PluginEditor.h"

namespace ob8 {

OB8Editor::OB8Editor (OB8Processor& p)
    : juce::AudioProcessorEditor (p), processorRef (p)
{
    setLookAndFeel (&laf);

    auto& apvts = processorRef.apvts;

    // VCO 1
    vco1Oct .reset (new OB8Choice (apvts, ParamID::vco1Octave, "OCT"));
    vco1Wave.reset (new OB8Choice (apvts, ParamID::vco1Wave,   "WAVE"));
    vco1Pw  .reset (new OB8Knob   (apvts, ParamID::vco1Pw,     "PW"));

    // VCO 2
    vco2Oct   .reset (new OB8Choice (apvts, ParamID::vco2Octave, "OCT"));
    vco2Wave  .reset (new OB8Choice (apvts, ParamID::vco2Wave,   "WAVE"));
    vco2Pw    .reset (new OB8Knob   (apvts, ParamID::vco2Pw,     "PW"));
    vco2Detune.reset (new OB8Knob   (apvts, ParamID::vco2Detune, "DETUNE"));

    // X-MOD / Sync
    xMod.reset (new OB8Knob   (apvts, ParamID::xMod, "X-MOD"));
    sync.reset (new OB8Toggle (apvts, ParamID::sync, "SYNC"));

    // Mixer
    mixVco1 .reset (new OB8Knob (apvts, ParamID::mixVco1,  "VCO 1"));
    mixVco2 .reset (new OB8Knob (apvts, ParamID::mixVco2,  "VCO 2"));
    mixNoise.reset (new OB8Knob (apvts, ParamID::mixNoise, "NOISE"));

    // Filter
    cutoff   .reset (new OB8Knob   (apvts, ParamID::cutoff,    "CUTOFF"));
    resonance.reset (new OB8Knob   (apvts, ParamID::resonance, "RES"));
    envAmount.reset (new OB8Knob   (apvts, ParamID::envAmount, "ENV"));
    lfoToVcf .reset (new OB8Knob   (apvts, ParamID::lfoToVcf,  "LFO"));
    kbdTrack .reset (new OB8Knob   (apvts, ParamID::kbdTrack,  "KBD"));
    slope    .reset (new OB8Choice (apvts, ParamID::slope,     "SLOPE"));

    // Filter env
    filtA.reset (new OB8Knob (apvts, ParamID::filtA, "A"));
    filtD.reset (new OB8Knob (apvts, ParamID::filtD, "D"));
    filtS.reset (new OB8Knob (apvts, ParamID::filtS, "S"));
    filtR.reset (new OB8Knob (apvts, ParamID::filtR, "R"));

    // Amp env
    ampA.reset (new OB8Knob (apvts, ParamID::ampA, "A"));
    ampD.reset (new OB8Knob (apvts, ParamID::ampD, "D"));
    ampS.reset (new OB8Knob (apvts, ParamID::ampS, "S"));
    ampR.reset (new OB8Knob (apvts, ParamID::ampR, "R"));

    // LFO
    lfoRate  .reset (new OB8Knob   (apvts, ParamID::lfoRate,   "RATE"));
    lfoShape .reset (new OB8Choice (apvts, ParamID::lfoShape,  "SHAPE"));
    lfoToVco1.reset (new OB8Knob   (apvts, ParamID::lfoToVco1, "VCO 1"));
    lfoToVco2.reset (new OB8Knob   (apvts, ParamID::lfoToVco2, "VCO 2"));
    lfoToPwm .reset (new OB8Knob   (apvts, ParamID::lfoToPwm,  "PWM"));

    // Velocity
    velToVca.reset (new OB8Knob (apvts, ParamID::velToVca, "-> VCA"));
    velToVcf.reset (new OB8Knob (apvts, ParamID::velToVcf, "-> VCF"));

    // Global
    polyMode    .reset (new OB8Choice (apvts, ParamID::polyMode,     "MODE"));
    unisonDetune.reset (new OB8Knob   (apvts, ParamID::unisonDetune, "UNI DET"));
    driftDepth  .reset (new OB8Knob   (apvts, ParamID::driftDepth,   "DRIFT"));
    masterGain  .reset (new OB8Knob   (apvts, ParamID::masterGain,   "VOLUME"));
    masterTune  .reset (new OB8Knob   (apvts, ParamID::masterTune,   "TUNE"));
    bendRange   .reset (new OB8Knob   (apvts, ParamID::bendRange,    "BEND"));

    // Sections (panel groupings)
    sections.push_back ({ "VCO 1",        {}, { vco1Oct.get(),    vco1Wave.get(),   vco1Pw.get() } });
    sections.push_back ({ "VCO 2",        {}, { vco2Oct.get(),    vco2Wave.get(),   vco2Pw.get(),  vco2Detune.get() } });
    sections.push_back ({ "X-MOD / SYNC", {}, { xMod.get(),       sync.get() } });
    sections.push_back ({ "MIXER",        {}, { mixVco1.get(),    mixVco2.get(),    mixNoise.get() } });
    sections.push_back ({ "FILTER",       {}, { cutoff.get(), resonance.get(), envAmount.get(),
                                                lfoToVcf.get(), kbdTrack.get(), slope.get() } });
    sections.push_back ({ "FILTER ENV",   {}, { filtA.get(), filtD.get(), filtS.get(), filtR.get() } });
    sections.push_back ({ "AMP ENV",      {}, { ampA.get(),  ampD.get(),  ampS.get(),  ampR.get()  } });
    sections.push_back ({ "LFO",          {}, { lfoRate.get(), lfoShape.get(),
                                                lfoToVco1.get(), lfoToVco2.get(), lfoToPwm.get() } });
    sections.push_back ({ "VELOCITY",     {}, { velToVca.get(), velToVcf.get() } });
    sections.push_back ({ "VOICE",        {}, { polyMode.get(), unisonDetune.get(), driftDepth.get(),
                                                masterGain.get(), masterTune.get(), bendRange.get() } });

    for (auto& s : sections)
        for (auto* c : s.children)
            addAndMakeVisible (c);

    setSize (1200, 480);
}

OB8Editor::~OB8Editor()
{
    setLookAndFeel (nullptr);
}

void OB8Editor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Panel gradient: dark blue at top fading to slightly darker bottom
    juce::ColourGradient bg (OB8LookAndFeel::panelBlue().brighter (0.05f),
                             0, 0,
                             OB8LookAndFeel::panelBlue().darker (0.4f),
                             0, bounds.getHeight(),
                             false);
    g.setGradientFill (bg);
    g.fillRect (bounds);

    // Header strip
    auto header = bounds.removeFromTop (44.0f);
    g.setColour (OB8LookAndFeel::panelDark());
    g.fillRect (header);
    g.setColour (OB8LookAndFeel::panelAccent());
    g.setFont (juce::Font (juce::FontOptions (24.0f).withStyle ("Bold")));
    g.drawText ("OB-8  NATIVE", header.reduced (16.0f, 0.0f),
                juce::Justification::centredLeft);
    g.setColour (OB8LookAndFeel::panelCream());
    g.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Italic")));
    g.drawText ("polyphonic analog modeling synthesizer", header.reduced (16.0f, 0.0f),
                juce::Justification::centredRight);

    // Section frames
    for (auto& s : sections)
    {
        auto r = s.bounds.toFloat();
        g.setColour (OB8LookAndFeel::panelDark());
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (OB8LookAndFeel::panelAccent().withAlpha (0.7f));
        g.drawRoundedRectangle (r, 6.0f, 1.2f);
        g.setColour (OB8LookAndFeel::panelAccent());
        g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
        g.drawText (s.title, r.removeFromTop (18.0f).reduced (8.0f, 0.0f),
                    juce::Justification::centredLeft);
    }
}

void OB8Editor::layoutSection (Section& s, juce::Rectangle<int> bounds, int cols)
{
    s.bounds = bounds;
    auto inner = bounds.reduced (6, 22);

    const int n     = (int) s.children.size();
    const int rows  = (n + cols - 1) / cols;
    const int cellW = inner.getWidth()  / cols;
    const int cellH = inner.getHeight() / juce::jmax (1, rows);

    for (int i = 0; i < n; ++i)
    {
        const int r = i / cols;
        const int c = i % cols;
        s.children[(size_t) i]->setBounds (
            inner.getX() + c * cellW + 2,
            inner.getY() + r * cellH + 2,
            cellW - 4, cellH - 4);
    }
}

void OB8Editor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (44);              // header
    bounds = bounds.reduced (10);

    // Top row: VCO1 | VCO2 | X-MOD | MIXER | FILTER | FILTER ENV
    auto topRow = bounds.removeFromTop (bounds.getHeight() * 11 / 20);
    auto bottomRow = bounds;

    const int wVCO1   = 170;
    const int wVCO2   = 210;
    const int wXMod   = 110;
    const int wMixer  = 200;
    const int wFilter = 320;
    const int wFEnv   = topRow.getWidth() - (wVCO1 + wVCO2 + wXMod + wMixer + wFilter) - 5*8;

    layoutSection (sections[0], topRow.removeFromLeft (wVCO1),   3);
    topRow.removeFromLeft (8);
    layoutSection (sections[1], topRow.removeFromLeft (wVCO2),   4);
    topRow.removeFromLeft (8);
    layoutSection (sections[2], topRow.removeFromLeft (wXMod),   1);
    topRow.removeFromLeft (8);
    layoutSection (sections[3], topRow.removeFromLeft (wMixer),  3);
    topRow.removeFromLeft (8);
    layoutSection (sections[4], topRow.removeFromLeft (wFilter), 3);
    topRow.removeFromLeft (8);
    layoutSection (sections[5], topRow,                          4);

    // Bottom row: AMP ENV | LFO | VELOCITY | VOICE
    const int wAEnv = 240;
    const int wLfo  = 340;
    const int wVel  = 180;
    const int wVoice = bottomRow.getWidth() - (wAEnv + wLfo + wVel) - 3*8;

    layoutSection (sections[6], bottomRow.removeFromLeft (wAEnv), 4);
    bottomRow.removeFromLeft (8);
    layoutSection (sections[7], bottomRow.removeFromLeft (wLfo),  5);
    bottomRow.removeFromLeft (8);
    layoutSection (sections[8], bottomRow.removeFromLeft (wVel),  2);
    bottomRow.removeFromLeft (8);
    layoutSection (sections[9], bottomRow,                        3);
}

} // namespace ob8
