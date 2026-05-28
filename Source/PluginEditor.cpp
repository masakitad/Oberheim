#include "PluginEditor.h"

namespace ob8 {

OB8Editor::OB8Editor (OB8Processor& p)
    : juce::AudioProcessorEditor (p),
      processorRef (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&laf);

    // Make the editor itself focus-friendly so the OS will route key events
    // here, then forward them to the keyboard component.
    setWantsKeyboardFocus (true);

    addAndMakeVisible (keyboard);
    keyboard.setWantsKeyboardFocus (true);
    keyboard.setKeyWidth (22.0f);                 // pixels
    keyboard.setAvailableRange (21, 108);         // standard 88-key span
    keyboard.setLowestVisibleKey (48);            // start at C3 (scientific)
    keyboard.setOctaveForMiddleC (4);             // MIDI 60 labelled "C4"
    keyboard.setKeyPressBaseOctave (4);           // PC 'A' key plays C3 = MIDI 48
    // White keys: warm cream-paper tint (handoff §6.12)
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,
                        juce::Colour::fromRGBA (255, 250, 235, 90));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId,
                        OB8LookAndFeel::ink());
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,
                        OB8LookAndFeel::hairline());
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                        OB8LookAndFeel::accent().withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
                        OB8LookAndFeel::accent().withAlpha (0.65f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId,
                        OB8LookAndFeel::ink());

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

    // FILTER ModChips (handoff §6.4)
    chipCutoffE2.reset (new ModChip ("E2"));
    chipCutoffL1.reset (new ModChip ("L1"));
    chipCutoffV1.reset (new ModChip ("V1"));
    chipResE2   .reset (new ModChip ("E2"));
    addAndMakeVisible (*chipCutoffE2);
    addAndMakeVisible (*chipCutoffL1);
    addAndMakeVisible (*chipCutoffV1);
    addAndMakeVisible (*chipResE2);

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

    // Performance
    glide.reset (new OB8Knob   (apvts, ParamID::glide, "GLIDE"));
    hold .reset (new OB8Toggle (apvts, ParamID::hold,  "HOLD"));
    ampReleaseInf.reset (new OB8Toggle (apvts, ParamID::ampReleaseInf, "REL\xe2\x88\x9e"));

    // Delay
    dlyTimeL.reset (new OB8Knob (apvts, ParamID::delayTimeL,    "TIME L"));
    dlyTimeR.reset (new OB8Knob (apvts, ParamID::delayTimeR,    "TIME R"));
    dlyFb   .reset (new OB8Knob (apvts, ParamID::delayFeedback, "FBK"));
    dlyCross.reset (new OB8Knob (apvts, ParamID::delayCross,    "CROSS"));
    dlyDamp .reset (new OB8Knob (apvts, ParamID::delayDamping,  "DAMP"));
    dlyMix  .reset (new OB8Knob (apvts, ParamID::delayMix,      "MIX"));

    // Reverb
    rvbSize .reset (new OB8Knob (apvts, ParamID::reverbSize,       "SIZE"));
    rvbDecay.reset (new OB8Knob (apvts, ParamID::reverbDecay,      "DECAY"));
    rvbDamp .reset (new OB8Knob (apvts, ParamID::reverbDamping,    "DAMP"));
    rvbPre  .reset (new OB8Knob (apvts, ParamID::reverbPreDelay,   "PRE-DLY"));
    rvbMod  .reset (new OB8Knob (apvts, ParamID::reverbModulation, "MOD"));
    rvbWidth.reset (new OB8Knob (apvts, ParamID::reverbWidth,      "WIDTH"));
    rvbMix  .reset (new OB8Knob (apvts, ParamID::reverbMix,        "MIX"));

    // Granular delay
    grSize    .reset (new OB8Knob (apvts, ParamID::granularSize,     "SIZE"));
    grDensity .reset (new OB8Knob (apvts, ParamID::granularDensity,  "DENS"));
    grScatter .reset (new OB8Knob (apvts, ParamID::granularScatter,  "SCAT"));
    grPitch   .reset (new OB8Knob (apvts, ParamID::granularPitch,    "PITCH"));
    grSpread  .reset (new OB8Knob (apvts, ParamID::granularSpread,   "SPREAD"));
    grFeedback.reset (new OB8Knob (apvts, ParamID::granularFeedback, "FBK"));
    grMix     .reset (new OB8Knob (apvts, ParamID::granularMix,      "MIX"));

    // Page 2
    envToVco1 .reset (new OB8Knob   (apvts, ParamID::envToVco1,   "FE -> V1"));
    envToVco2 .reset (new OB8Knob   (apvts, ParamID::envToVco2,   "FE -> V2"));
    envToPwm  .reset (new OB8Knob   (apvts, ParamID::envToPwm,    "FE -> PW"));
    atToVcf   .reset (new OB8Knob   (apvts, ParamID::atToVcf,     "AT -> VCF"));
    atToLfo   .reset (new OB8Knob   (apvts, ParamID::atToLfo,     "AT -> LFO"));
    atToVca   .reset (new OB8Knob   (apvts, ParamID::atToVca,     "AT -> VCA"));
    mwToVcf   .reset (new OB8Knob   (apvts, ParamID::mwToVcf,     "MW -> VCF"));
    mwToLfo   .reset (new OB8Knob   (apvts, ParamID::mwToLfo,     "MW -> LFO"));
    mwToVibrato.reset (new OB8Knob  (apvts, ParamID::mwToVibrato, "MW -> VIB"));
    lfoKeySync.reset (new OB8Toggle (apvts, ParamID::lfoKeySync,  "KEY SYNC"));

    // Split / Double
    splitPoint  .reset (new OB8Knob   (apvts, ParamID::splitPoint,        "SPLIT"));
    splitOctave .reset (new OB8Choice (apvts, ParamID::splitOctaveOffset, "S OCT"));
    splitDetune .reset (new OB8Knob   (apvts, ParamID::splitDetune,       "S DET"));
    doubleDetune.reset (new OB8Knob   (apvts, ParamID::doubleDetune,      "D DET"));

    // Simple-view macros
    macroTone  .reset (new OB8Knob (apvts, ParamID::macroTone,   "TONE"));
    macroMotion.reset (new OB8Knob (apvts, ParamID::macroMotion, "MOTION"));
    macroSpace .reset (new OB8Knob (apvts, ParamID::macroSpace,  "SPACE"));
    addAndMakeVisible (*macroTone);
    addAndMakeVisible (*macroMotion);
    addAndMakeVisible (*macroSpace);

    // View-mode selector lives in the header area regardless of mode
    addAndMakeVisible (viewModeLabel);
    addAndMakeVisible (viewModeCombo);
    viewModeLabel.setFont    (OB8LookAndFeel::monoBold (9.0f).withExtraKerningFactor (0.10f));
    viewModeLabel.setColour  (juce::Label::textColourId, OB8LookAndFeel::panelMute());
    viewModeLabel.setJustificationType (juce::Justification::centredRight);
    viewModeCombo.addItem ("FULL",   1);
    viewModeCombo.addItem ("SIMPLE", 2);
    viewModeAttach.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (
        apvts, ParamID::viewMode, viewModeCombo));

    // Listen for view-mode changes so we can swap the visible UI
    apvts.addParameterListener (ParamID::viewMode, this);

    // Patch management
    addAndMakeVisible (bankLabel);
    addAndMakeVisible (programLabel);
    addAndMakeVisible (patchNameLabel);
    bankLabel.setColour    (juce::Label::textColourId, OB8LookAndFeel::inkDim());
    programLabel.setColour (juce::Label::textColourId, OB8LookAndFeel::inkDim());
    patchNameLabel.setColour (juce::Label::textColourId, OB8LookAndFeel::inkDim());
    bankLabel.setFont      (OB8LookAndFeel::monoBold (10.5f));
    programLabel.setFont   (OB8LookAndFeel::monoBold (10.5f));
    patchNameLabel.setFont (OB8LookAndFeel::monoBold (10.5f));

    addAndMakeVisible (bankCombo);
    addAndMakeVisible (programCombo);
    addAndMakeVisible (patchNameEdit);
    addAndMakeVisible (storeBtn);
    addAndMakeVisible (recallBtn);
    addAndMakeVisible (saveBankBtn);
    addAndMakeVisible (loadBankBtn);

    patchNameEdit.setText (processorRef.currentPatchName);
    patchNameEdit.setColour (juce::TextEditor::backgroundColourId, OB8LookAndFeel::paperBg());
    patchNameEdit.setColour (juce::TextEditor::textColourId,       OB8LookAndFeel::ink());
    patchNameEdit.setColour (juce::TextEditor::outlineColourId,    OB8LookAndFeel::hairline());
    patchNameEdit.setColour (juce::TextEditor::highlightColourId,  OB8LookAndFeel::accent().withAlpha (0.30f));
    patchNameEdit.setFont (OB8LookAndFeel::monoRegular (10.5f).withExtraKerningFactor (0.04f));

    populateBankCombo();
    populateProgramCombo();

    bankCombo.onChange    = [this] { processorRef.currentBank = bankCombo.getSelectedItemIndex(); populateProgramCombo(); };
    programCombo.onChange = [this] { processorRef.currentProgram = programCombo.getSelectedItemIndex(); };

    storeBtn.onClick     = [this] { storeCurrentPatch(); };
    recallBtn.onClick    = [this] { recallSelectedPatch(); };
    saveBankBtn.onClick  = [this] { chooseSaveBank(); };
    loadBankBtn.onClick  = [this] { chooseLoadBank(); };

    // Section table -- order must match resized()
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
                                                masterGain.get(), masterTune.get(), bendRange.get(),
                                                glide.get(), hold.get(),
                                                ampReleaseInf.get() } });

    sections.push_back ({ "PAGE 2 - ENVELOPE & AT/MW", {},
                          { envToVco1.get(), envToVco2.get(), envToPwm.get(),
                            atToVcf.get(),  atToLfo.get(),   atToVca.get(),
                            mwToVcf.get(),  mwToLfo.get(),   mwToVibrato.get(),
                            lfoKeySync.get() } });

    sections.push_back ({ "SPLIT / DOUBLE", {},
                          { splitPoint.get(), splitOctave.get(),
                            splitDetune.get(), doubleDetune.get() } });

    // Section 12: PATCH BANK (frame only; widgets laid out by hand in resized())
    sections.push_back ({ "PATCH BANK", {}, {} });

    // Section 13: DELAY
    sections.push_back ({ "DELAY", {},
                          { dlyTimeL.get(), dlyTimeR.get(), dlyFb.get(),
                            dlyCross.get(), dlyDamp.get(), dlyMix.get() } });

    // Section 14: REVERB
    sections.push_back ({ "REVERB", {},
                          { rvbSize.get(), rvbDecay.get(), rvbDamp.get(),
                            rvbPre.get(), rvbMod.get(), rvbWidth.get(), rvbMix.get() } });

    // Section 15: GRANULAR
    sections.push_back ({ "GRANULAR", {},
                          { grSize.get(), grDensity.get(), grScatter.get(),
                            grPitch.get(), grSpread.get(), grFeedback.get(),
                            grMix.get() } });

    for (auto& s : sections)
        for (auto* c : s.children)
            addAndMakeVisible (c);

    // Octave shift buttons next to the keyboard
    addAndMakeVisible (octDownBtn);
    addAndMakeVisible (octUpBtn);
    addAndMakeVisible (octaveLabel);
    octaveLabel.setJustificationType (juce::Justification::centred);
    octaveLabel.setFont (OB8LookAndFeel::monoBold (13.0f));
    octaveLabel.setColour (juce::Label::textColourId, OB8LookAndFeel::panelAccent());

    octDownBtn.onClick = [this]
    {
        pcKeyboardBaseOctave = juce::jmax (1, pcKeyboardBaseOctave - 1);
        keyboard.setKeyPressBaseOctave (pcKeyboardBaseOctave);
        updateOctaveLabel();
        keyboard.grabKeyboardFocus();
    };
    octUpBtn.onClick = [this]
    {
        pcKeyboardBaseOctave = juce::jmin (8, pcKeyboardBaseOctave + 1);
        keyboard.setKeyPressBaseOctave (pcKeyboardBaseOctave);
        updateOctaveLabel();
        keyboard.grabKeyboardFocus();
    };
    updateOctaveLabel();

    // Per the HAIRLINE-VIII handoff, the panel is designed at a fixed
    // 1280 px width. We keep it resizable for laptop screens but lock
    // the aspect roughly at the designed proportions.
    setResizable (true, true);
    setResizeLimits (1180, 820, 1920, 1300);
    setSize (1280, 920);

    buildPaperTexture();

    // Initial visibility based on the persisted view mode parameter.
    applyViewMode();
}

void OB8Editor::updateOctaveLabel()
{
    // Show the octave that the PC keyboard's 'A' key plays (in scientific
    // pitch notation, e.g. C3 when pcKeyboardBaseOctave == 4).
    const int oct = pcKeyboardBaseOctave - 1; // setOctaveForMiddleC(4) means MIDI 60 = C4
    octaveLabel.setText (juce::String ("C") + juce::String (oct),
                         juce::dontSendNotification);
}

void OB8Editor::buildPaperTexture()
{
    // Composite three low-alpha layers onto a transparent canvas:
    //   1. warm halo (centre-top to bottom darken)
    //   2. fiber lines (very faint diagonals)
    //   3. vignette (corners darkened)
    // The result is cached as juce::Image and re-blitted each paint().
    const int w = juce::jmax (1, getWidth());
    const int h = juce::jmax (1, getHeight());
    if (w < 4 || h < 4) return;

    paperTexture = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics tg (paperTexture);

    using LF = OB8LookAndFeel;

    // 1. Halo: a soft radial highlight near the top centre
    {
        juce::ColourGradient halo (
            LF::ink().withAlpha (0.00f), w * 0.5f, h * 0.20f,
            LF::ink().withAlpha (0.04f), 0.0f,     h * 1.10f,
            true);
        tg.setGradientFill (halo);
        tg.fillRect (juce::Rectangle<int> (0, 0, w, h));
    }

    // 2. Fiber lines: shallow diagonals at very low alpha
    {
        tg.setColour (LF::ink().withAlpha (0.018f));
        for (int i = -h; i < w; i += 11)
        {
            tg.drawLine (static_cast<float> (i),       0.0f,
                         static_cast<float> (i + h),   static_cast<float> (h),
                         0.5f);
        }
        tg.setColour (LF::ink().withAlpha (0.012f));
        for (int i = -h; i < w; i += 9)
        {
            tg.drawLine (static_cast<float> (i + h),   0.0f,
                         static_cast<float> (i),       static_cast<float> (h),
                         0.5f);
        }
    }

    // 3. Vignette: four corner shadings
    {
        juce::ColourGradient vg (
            LF::ink().withAlpha (0.00f), w * 0.5f, h * 0.5f,
            LF::ink().withAlpha (0.08f), 0.0f,     0.0f,
            true);
        tg.setGradientFill (vg);
        tg.fillRect (juce::Rectangle<int> (0, 0, w, h));
    }
}

OB8Editor::~OB8Editor()
{
    processorRef.apvts.removeParameterListener (ParamID::viewMode, this);
    setLookAndFeel (nullptr);
}

void OB8Editor::parameterChanged (const juce::String& id, float /*newValue*/)
{
    if (id == ParamID::viewMode)
    {
        // The APVTS listener fires on the parameter-change thread; bounce
        // back to the message thread before touching component visibility.
        juce::MessageManager::callAsync (
            [safe = juce::Component::SafePointer (this)]
        {
            if (safe != nullptr) safe->applyViewMode();
        });
    }
}

void OB8Editor::applyViewMode()
{
    const int mode = static_cast<int> (
        processorRef.apvts.getRawParameterValue (ParamID::viewMode)->load());
    const bool simple = (mode == 1);

    // 3 macro knobs visible only in SIMPLE
    if (macroTone)   macroTone  ->setVisible (simple);
    if (macroMotion) macroMotion->setVisible (simple);
    if (macroSpace)  macroSpace ->setVisible (simple);

    // All section children + the bank-management widgets are FULL-only
    for (auto& s : sections)
        for (auto* c : s.children)
            if (c != nullptr) c->setVisible (! simple);

    bankLabel.setVisible      (! simple);
    programLabel.setVisible   (! simple);
    patchNameLabel.setVisible (! simple);
    bankCombo.setVisible      (! simple);
    programCombo.setVisible   (! simple);
    patchNameEdit.setVisible  (! simple);
    storeBtn.setVisible       (! simple);
    recallBtn.setVisible      (! simple);
    saveBankBtn.setVisible    (! simple);

    // ModChips track the FULL view
    if (chipCutoffE2) chipCutoffE2->setVisible (! simple);
    if (chipCutoffL1) chipCutoffL1->setVisible (! simple);
    if (chipCutoffV1) chipCutoffV1->setVisible (! simple);
    if (chipResE2)    chipResE2   ->setVisible (! simple);
    loadBankBtn.setVisible    (! simple);

    resized();
    repaint();
}

void OB8Editor::visibilityChanged()
{
    // Once the editor is on-screen, hand keyboard focus to the on-screen
    // keyboard so PC key events go directly to it. Once the user clicks
    // anywhere else, focus moves away -- key forwarding (below) covers
    // the rest.
    if (isShowing())
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer (this)]
        {
            if (safe != nullptr) safe->keyboard.grabKeyboardFocus();
        });
}

bool OB8Editor::keyPressed (const juce::KeyPress& key)
{
    // Forward A..K, W..U, Z/X etc. to the keyboard component even when
    // some child (knob, combo box) currently has keyboard focus. JUCE
    // propagates unhandled keys up through the parent chain to this
    // override; we just hand them back to the keyboard.
    return keyboard.keyPressed (key);
}

bool OB8Editor::keyStateChanged (bool isKeyDown)
{
    return keyboard.keyStateChanged (isKeyDown);
}

void OB8Editor::populateBankCombo()
{
    bankCombo.clear (juce::dontSendNotification);
    for (int i = 0; i < OB8Processor::kNumBanks; ++i)
        bankCombo.addItem (juce::String ("Bank ") + juce::String (i + 1), i + 1);
    bankCombo.setSelectedItemIndex (juce::jlimit (0, OB8Processor::kNumBanks - 1, processorRef.currentBank),
                                    juce::dontSendNotification);
}

void OB8Editor::populateProgramCombo()
{
    programCombo.clear (juce::dontSendNotification);
    if (auto bank = processorRef.bankState.getChild (processorRef.currentBank); bank.isValid())
    {
        for (int p = 0; p < bank.getNumChildren(); ++p)
        {
            auto patch = bank.getChild (p);
            auto name  = patch.getProperty ("name").toString();
            programCombo.addItem (juce::String (p + 1) + ": " + name, p + 1);
        }
    }
    programCombo.setSelectedItemIndex (juce::jlimit (0, OB8Processor::kPatchesPerBank - 1, processorRef.currentProgram),
                                       juce::dontSendNotification);
}

void OB8Editor::storeCurrentPatch()
{
    auto bank  = processorRef.bankState.getChild (processorRef.currentBank);
    auto patch = bank.getChild (processorRef.currentProgram);
    if (! patch.isValid()) return;

    const auto name = patchNameEdit.getText().isNotEmpty()
                      ? patchNameEdit.getText() : juce::String ("User Patch");
    processorRef.currentPatchName = name;

    patch.removeAllChildren (nullptr);
    patch.removeAllProperties (nullptr);
    patch.setProperty ("name", name, nullptr);

    juce::XmlElement holder ("PatchHolder");
    processorRef.saveCurrentPatchToXml (holder, name);
    if (auto* paramsXml = holder.getFirstChildElement())
    {
        if (auto vt = juce::ValueTree::fromXml (*paramsXml); vt.isValid())
            patch.appendChild (vt, nullptr);
    }
    populateProgramCombo();
}

void OB8Editor::recallSelectedPatch()
{
    auto bank  = processorRef.bankState.getChild (processorRef.currentBank);
    auto patch = bank.getChild (processorRef.currentProgram);
    if (! patch.isValid() || patch.getNumChildren() == 0) return;

    juce::XmlElement holder ("PatchHolder");
    holder.setAttribute ("name", patch.getProperty ("name").toString());
    if (auto x = patch.getChild (0).createXml())
        holder.addChildElement (x.release());
    if (processorRef.loadCurrentPatchFromXml (holder))
        patchNameEdit.setText (processorRef.currentPatchName);
}

void OB8Editor::chooseSaveBank()
{
    fileChooser.reset (new juce::FileChooser ("Save OB-8 bank",
                                              juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                              "*.ob8bank"));
    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
    {
        if (fc.getResult() != juce::File())
        {
            auto file = fc.getResult();
            if (! file.hasFileExtension ("ob8bank"))
                file = file.withFileExtension (".ob8bank");
            processorRef.saveBankToFile (file);
        }
    });
}

void OB8Editor::chooseLoadBank()
{
    fileChooser.reset (new juce::FileChooser ("Load OB-8 bank",
                                              juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                              "*.ob8bank"));
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
    {
        if (auto file = fc.getResult(); file.existsAsFile())
        {
            if (processorRef.loadBankFromFile (file))
            {
                populateBankCombo();
                populateProgramCombo();
            }
        }
    });
}

void OB8Editor::paint (juce::Graphics& g)
{
    using LF = OB8LookAndFeel;
    const auto fullBounds = getLocalBounds().toFloat();

    // ---- 1. Paper background + cached overlay texture -------------------
    g.fillAll (LF::paperBg());
    if (paperTexture.isValid())
        g.drawImageAt (paperTexture, 0, 0);

    // ---- Corner registration marks (handoff §6 - crosshair + circle) ----
    {
        const float r        = 5.0f;
        const float inset    = 12.0f;
        const float tickLen  = 7.0f;
        g.setColour (LF::hairFine());
        for (int corner = 0; corner < 4; ++corner)
        {
            const float cx = (corner & 1) ? fullBounds.getWidth() - inset : inset;
            const float cy = (corner & 2) ? fullBounds.getHeight() - inset : inset;
            g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 0.7f);
            g.drawLine (cx - tickLen, cy, cx + tickLen, cy, 0.7f);
            g.drawLine (cx, cy - tickLen, cx, cy + tickLen, 0.7f);
        }
    }

    // ---- 2. Header: HAIRLINE-VIII (Fraunces 30 / 600) + metadata --------
    const float kHeaderH = 56.0f;
    auto header = fullBounds.withHeight (kHeaderH);
    auto headerInner = header.reduced (26.0f, 14.0f);

    // Wordmark (serif)
    g.setColour (LF::ink());
    g.setFont   (LF::serifSemiBold (30.0f).withExtraKerningFactor (0.02f));
    const juce::String wordmark = "HAIRLINE-VIII";
    const int wordW = g.getCurrentFont().getStringWidth (wordmark);
    g.drawText (wordmark, headerInner, juce::Justification::centredLeft);

    // Series line in accent red, right of the wordmark
    auto seriesRect = headerInner;
    seriesRect.removeFromLeft (static_cast<float> (wordW) + 18.0f);
    g.setColour (LF::accent());
    g.setFont   (LF::monoRegular (9.5f).withExtraKerningFactor (0.12f));
    g.drawText (juce::String::fromUTF8 ("NO. 0427  \xc2\xb7  REV. C  \xc2\xb7  FADER CUT"),
                seriesRect, juce::Justification::centredLeft);

    // Tagline (italic mono, right edge)
    g.setColour (LF::inkDim());
    g.setFont   (LF::monoItalic (10.0f).withExtraKerningFactor (0.08f));
    g.drawText (juce::String::fromUTF8 ("eight-voice polyphonic  \xc2\xb7  drafted at 1:1"),
                headerInner, juce::Justification::centredRight);

    // Hairline rule under the header (bands separated by 1px hairlines only)
    g.setColour (LF::hairline());
    g.drawLine (26.0f, header.getBottom(),
                fullBounds.getWidth() - 26.0f, header.getBottom(), 1.0f);

    // ---- 3. Numbered section titles -- no box frames, just a row label --
    // The cells share band hairlines; we draw only the numbered title in
    // the top-left of each cell rect computed by layoutSection.
    for (auto& s : sections)
    {
        auto r = s.bounds.toFloat();
        if (r.isEmpty()) continue;

        const int idx = static_cast<int> (&s - &sections[0]) + 1;
        const auto indexStr = (idx < 10 ? juce::String ("0") + juce::String (idx)
                                        : juce::String (idx));

        auto titleStrip = r.removeFromTop (14.0f).reduced (4.0f, 0.0f);

        g.setColour (LF::inkFaint());
        g.setFont   (LF::monoRegular (9.0f).withExtraKerningFactor (0.20f));
        g.drawText (indexStr, titleStrip.removeFromLeft (20.0f),
                    juce::Justification::centredLeft);

        g.setColour (LF::ink());
        g.setFont   (LF::monoRegular (9.0f).withExtraKerningFactor (0.20f));
        g.drawText (s.title.toUpperCase(), titleStrip,
                    juce::Justification::centredLeft);

        // Vertical hair separator on the cell's right edge (skip for the
        // rightmost cells -- detected by being within a few pixels of the
        // panel right margin)
        if (s.bounds.getRight() < fullBounds.getWidth() - 32)
        {
            g.setColour (LF::hairFine());
            g.drawLine (s.bounds.getRight() + 0.5f, s.bounds.getY() + 4.0f,
                        s.bounds.getRight() + 0.5f, s.bounds.getBottom() - 4.0f,
                        1.0f);
        }
    }

    // ---- Footer (engineering title block) ------------------------------
    auto footer = fullBounds; // local copy of full editor bounds
    footer = footer.removeFromBottom (36.0f).reduced (12.0f, 2.0f);
    g.setColour (LF::panelDark());
    g.drawRect (footer, 0.8f);

    const char* fields[][2] = {
        { "TITLE",  "HAIRLINE-VIII"       },
        { "SERIES", "No. 0427 \xc2\xb7 rev. C" },
        { "SCALE",  "1 : 1"               },
        { "SHEET",  "01 / 01"             },
        { "REV",    "C"                   },
        { "DATE",   "2026\xc2\xb7""05"    },
    };
    constexpr int kNumFields = sizeof(fields) / sizeof(fields[0]);
    // TITLE and SERIES are 2x wider than the others (per spec)
    const float widths[kNumFields] = { 2.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    float widthSum = 0.0f;
    for (float w : widths) widthSum += w;
    const float unit = footer.getWidth() / widthSum;
    for (int i = 0; i < kNumFields; ++i)
    {
        auto cell = footer.removeFromLeft (widths[i] * unit);
        if (i > 0)
        {
            g.setColour (LF::hairFine());
            g.drawLine (cell.getX(), cell.getY() + 2.0f,
                        cell.getX(), cell.getBottom() - 2.0f, 1.0f);
        }
        auto cinner = cell.reduced (10.0f, 4.0f);
        g.setColour (LF::inkDim());
        g.setFont   (LF::monoRegular (7.5f).withExtraKerningFactor (0.20f));
        g.drawText (juce::String (fields[i][0]).toUpperCase(),
                    cinner.removeFromTop (10.0f),
                    juce::Justification::centredLeft);
        g.setColour (LF::ink());
        g.setFont   (LF::monoRegular (10.5f).withExtraKerningFactor (0.04f));
        g.drawText (juce::String::fromUTF8 (fields[i][1]),
                    cinner, juce::Justification::centredLeft);
    }
}

void OB8Editor::layoutSection (Section& s, juce::Rectangle<int> bounds, int cols)
{
    s.bounds = bounds;
    auto inner = bounds.reduced (6, 22);
    const int n    = (int) s.children.size();
    if (n == 0) return;
    const int rows = (n + cols - 1) / cols;
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
    // Rebuild the cached paper texture whenever the editor size changes
    // (constructor sets the initial size after construction, so the first
    // call here is what populates the image).
    buildPaperTexture();

    auto bounds = getLocalBounds();

    // View-mode selector sits in the top-right of the header band so it's
    // reachable from either layout.
    {
        const int viewW = 120, viewH = 22;
        const int x = getWidth() - 12 - viewW;
        viewModeLabel.setBounds (x - 50, 10, 50, viewH);
        viewModeCombo.setBounds (x,      10, viewW, viewH);
    }

    // Header (44 px) drawn in paint() -- no anchor tab row in this build
    // Header band (56 px) drawn in paint()
    bounds.removeFromTop (56);
    // Engineering title-block footer (36 px) drawn in paint()
    bounds.removeFromBottom (36);
    bounds = bounds.reduced (16, 8);

    // Reserve space at the bottom for the on-screen keyboard. Compact build:
    // cap at 76 px so the synth panel itself keeps a usable height even on a
    // 13" Mac display.
    const int kKbdH = juce::jmin (76, juce::jmax (60, bounds.getHeight() / 11));
    auto kbdBounds = bounds.removeFromBottom (kKbdH);

    // Octave shift controls share the keyboard row on the left side
    auto octBounds = kbdBounds.removeFromLeft (76);
    octBounds.reduce (3, 3);
    auto labelStrip = octBounds.removeFromTop (18);
    octaveLabel.setBounds (labelStrip);
    auto btnStrip   = octBounds.removeFromTop (26);
    octDownBtn.setBounds (btnStrip.removeFromLeft (btnStrip.getWidth() / 2).reduced (2));
    octUpBtn  .setBounds (btnStrip.reduced (2));

    bounds.removeFromBottom (6);

    // ---- SIMPLE view: 3 large macros, centred --------------------------
    const int mode = static_cast<int> (
        processorRef.apvts.getRawParameterValue (ParamID::viewMode)->load());
    if (mode == 1)
    {
        const int gap = 18;
        const int colW = (bounds.getWidth() - 2 * gap) / 3;
        const int reserve = juce::jmax (40, (bounds.getHeight() / 8));
        auto inner = bounds.reduced (gap, reserve);

        auto layout = [&] (OB8Knob& k, juce::Rectangle<int> r)
        {
            // Keep the macro knobs roughly square for a generous rotary
            const int side = juce::jmin (r.getWidth(), r.getHeight());
            r = r.withSizeKeepingCentre (side, r.getHeight());
            k.setBounds (r);
        };

        layout (*macroTone,   inner.removeFromLeft (colW));
        inner.removeFromLeft (gap);
        layout (*macroMotion, inner.removeFromLeft (colW));
        inner.removeFromLeft (gap);
        layout (*macroSpace,  inner);

        // Clear section bounds so paint() skips frames
        for (auto& s : sections) s.bounds = {};
        return;
    }

    keyboard.setBounds (kbdBounds);

    // Allocate the four rows proportionally so the window resizes cleanly.
    // Weights (30/24/26/20) keep the knob clusters legible while leaving
    // a reasonable strip for the new FX row at the bottom of the panel.
    const int kInterRowGap = 6;
    const int availH = bounds.getHeight() - 3 * kInterRowGap;
    const int rowH1  = (availH * 30) / 100;
    const int rowH2  = (availH * 24) / 100;
    const int rowH3  = (availH * 26) / 100;

    // Row 1: VCO1 | VCO2 | X-MOD | MIXER | FILTER | FILTER ENV
    auto row1 = bounds.removeFromTop (rowH1);

    const int wVCO1   = 170;
    const int wVCO2   = 210;
    const int wXMod   = 110;
    const int wMixer  = 200;
    const int wFilter = 320;

    layoutSection (sections[0], row1.removeFromLeft (wVCO1),   3);
    row1.removeFromLeft (8);
    layoutSection (sections[1], row1.removeFromLeft (wVCO2),   4);
    row1.removeFromLeft (8);
    layoutSection (sections[2], row1.removeFromLeft (wXMod),   1);
    row1.removeFromLeft (8);
    layoutSection (sections[3], row1.removeFromLeft (wMixer),  3);
    row1.removeFromLeft (8);
    layoutSection (sections[4], row1.removeFromLeft (wFilter), 3);
    row1.removeFromLeft (8);
    layoutSection (sections[5], row1,                          4);

    bounds.removeFromTop (kInterRowGap);

    // Row 2: AMP ENV | LFO | VELOCITY | VOICE
    auto row2 = bounds.removeFromTop (rowH2);
    const int wAEnv  = 240;
    const int wLfo   = 340;
    const int wVel   = 180;

    layoutSection (sections[6], row2.removeFromLeft (wAEnv), 4);
    row2.removeFromLeft (8);
    layoutSection (sections[7], row2.removeFromLeft (wLfo),  5);
    row2.removeFromLeft (8);
    layoutSection (sections[8], row2.removeFromLeft (wVel),  2);
    row2.removeFromLeft (8);
    // VOICE has 9 children (mode / uni-det / drift / volume / tune / bend
    // / glide / hold / rel-infinity). 9 columns keep them in a single row.
    layoutSection (sections[9], row2,                        9);

    bounds.removeFromTop (kInterRowGap);

    // Row 3: PAGE 2 | SPLIT/DOUBLE | PATCH BANK (hand-laid out)
    auto row3 = bounds.removeFromTop (rowH3);
    const int wPage2 = 620;
    const int wSplit = 220;

    layoutSection (sections[10], row3.removeFromLeft (wPage2), 5);
    row3.removeFromLeft (8);
    layoutSection (sections[11], row3.removeFromLeft (wSplit), 2);
    row3.removeFromLeft (8);

    // Patch bank section: get a bordered frame via the Section system
    auto patchSectionBounds = row3;
    sections[12].bounds = patchSectionBounds;

    auto patchArea = patchSectionBounds.reduced (8, 24);
    auto labelRow = patchArea.removeFromTop (16);
    bankLabel    .setBounds (labelRow.removeFromLeft (60));
    programLabel .setBounds (labelRow.removeFromLeft (140));
    patchNameLabel.setBounds (labelRow);

    auto comboRow = patchArea.removeFromTop (24);
    bankCombo    .setBounds (comboRow.removeFromLeft (60).reduced (1));
    programCombo .setBounds (comboRow.removeFromLeft (140).reduced (1));
    patchNameEdit.setBounds (comboRow.reduced (1));

    patchArea.removeFromTop (8);

    auto btnRow1 = patchArea.removeFromTop (28);
    storeBtn .setBounds (btnRow1.removeFromLeft (btnRow1.getWidth() / 2).reduced (2));
    recallBtn.setBounds (btnRow1.reduced (2));

    patchArea.removeFromTop (4);

    auto btnRow2 = patchArea.removeFromTop (28);
    saveBankBtn.setBounds (btnRow2.removeFromLeft (btnRow2.getWidth() / 2).reduced (2));
    loadBankBtn.setBounds (btnRow2.reduced (2));

    bounds.removeFromTop (kInterRowGap);

    // Row 4: DELAY | REVERB | GRANULAR  (post-effects, three sections side-by-side)
    auto row4 = bounds;
    const int totalCols = 6 + 7 + 7;          // controls per section
    const int row4w     = row4.getWidth() - 2 * 8;
    const int wDelay    = row4w * 6 / totalCols;
    const int wReverb   = row4w * 7 / totalCols;
    layoutSection (sections[13], row4.removeFromLeft (wDelay),  6);
    row4.removeFromLeft (8);
    layoutSection (sections[14], row4.removeFromLeft (wReverb), 7);
    row4.removeFromLeft (8);
    layoutSection (sections[15], row4,                          7);

    // ---- FILTER ModChips ------------------------------------------------
    // Position them inline below the CUTOFF and RES knob value displays.
    // Handoff §6.4: chip height 12 px; we lay them out side-by-side.
    if (cutoff != nullptr && chipCutoffE2 != nullptr)
    {
        auto place = [&] (ModChip& c, juce::Rectangle<int>& cursor)
        {
            const int w = c.preferredWidth();
            c.setBounds (cursor.removeFromLeft (w + 4).withTrimmedRight (4)
                                                       .withHeight (ModChip::kHeight)
                                                       .withY (cursor.getY()));
        };
        const int chipBaselineOffset = 4;
        auto cutoffBox = cutoff->getBounds()
                            .withTrimmedTop (cutoff->getHeight() - ModChip::kHeight - chipBaselineOffset)
                            .reduced (2, 0);
        place (*chipCutoffE2, cutoffBox);
        place (*chipCutoffL1, cutoffBox);
        place (*chipCutoffV1, cutoffBox);

        auto resBox = resonance->getBounds()
                         .withTrimmedTop (resonance->getHeight() - ModChip::kHeight - chipBaselineOffset)
                         .reduced (2, 0);
        place (*chipResE2, resBox);
    }
}

} // namespace ob8
