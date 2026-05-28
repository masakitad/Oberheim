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
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,
                        OB8LookAndFeel::panelCream());
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId,
                        OB8LookAndFeel::panelDark());
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,
                        OB8LookAndFeel::panelDark().withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                        OB8LookAndFeel::panelOrange().withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
                        OB8LookAndFeel::panelOrange().withAlpha (0.75f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId,
                        OB8LookAndFeel::panelDark());

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

    // Performance
    glide.reset (new OB8Knob   (apvts, ParamID::glide, "GLIDE"));
    hold .reset (new OB8Toggle (apvts, ParamID::hold,  "HOLD"));

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

    // Patch management
    addAndMakeVisible (bankLabel);
    addAndMakeVisible (programLabel);
    addAndMakeVisible (patchNameLabel);
    bankLabel.setColour    (juce::Label::textColourId, OB8LookAndFeel::panelAccent());
    programLabel.setColour (juce::Label::textColourId, OB8LookAndFeel::panelAccent());
    patchNameLabel.setColour (juce::Label::textColourId, OB8LookAndFeel::panelAccent());
    bankLabel.setFont    (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    programLabel.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    patchNameLabel.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));

    addAndMakeVisible (bankCombo);
    addAndMakeVisible (programCombo);
    addAndMakeVisible (patchNameEdit);
    addAndMakeVisible (storeBtn);
    addAndMakeVisible (recallBtn);
    addAndMakeVisible (saveBankBtn);
    addAndMakeVisible (loadBankBtn);

    patchNameEdit.setText (processorRef.currentPatchName);
    patchNameEdit.setColour (juce::TextEditor::backgroundColourId, OB8LookAndFeel::panelDark());
    patchNameEdit.setColour (juce::TextEditor::textColourId,       OB8LookAndFeel::panelCream());
    patchNameEdit.setColour (juce::TextEditor::outlineColourId,    OB8LookAndFeel::panelCream().withAlpha (0.4f));

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
                                                glide.get(), hold.get() } });

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

    for (auto& s : sections)
        for (auto* c : s.children)
            addAndMakeVisible (c);

    // Octave shift buttons next to the keyboard
    addAndMakeVisible (octDownBtn);
    addAndMakeVisible (octUpBtn);
    addAndMakeVisible (octaveLabel);
    octaveLabel.setJustificationType (juce::Justification::centred);
    octaveLabel.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
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

    // Resizable so the editor fits laptop screens. The resized() layout
    // scales the rows proportionally inside the new bounds, so it stays
    // usable down to the minimum size. Default size accommodates the new
    // header / tab anchor row / engineering title-block footer in addition
    // to the four control rows + keyboard.
    setResizable (true, true);
    setResizeLimits (1180, 960, 1920, 1400);
    setSize (1280, 1060);
}

void OB8Editor::updateOctaveLabel()
{
    // Show the octave that the PC keyboard's 'A' key plays (in scientific
    // pitch notation, e.g. C3 when pcKeyboardBaseOctave == 4).
    const int oct = pcKeyboardBaseOctave - 1; // setOctaveForMiddleC(4) means MIDI 60 = C4
    octaveLabel.setText (juce::String ("C") + juce::String (oct),
                         juce::dontSendNotification);
}

OB8Editor::~OB8Editor()
{
    setLookAndFeel (nullptr);
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
    auto bounds = getLocalBounds().toFloat();

    // ---- Paper background -----------------------------------------------
    g.fillAll (LF::panelCream());

    // ---- Header ----------------------------------------------------------
    auto header = bounds.removeFromTop (52.0f);
    g.setColour (LF::panelDark());
    g.setFont (juce::Font (juce::FontOptions (24.0f).withTypefaceStyle ("Bold")));
    g.drawText ("OB-8  NATIVE", header.reduced (18.0f, 0.0f),
                juce::Justification::centredLeft);

    g.setColour (LF::panelAccent());
    g.setFont (juce::Font (juce::FontOptions (12.0f).withTypefaceStyle ("Bold")));
    auto subRect = header.reduced (18.0f, 0.0f);
    subRect.removeFromLeft (220.0f);
    g.drawText ("NO. 0427  REV. C  FADER CUT", subRect,
                juce::Justification::centredLeft);

    g.setColour (LF::panelMute());
    g.setFont (juce::Font (juce::FontOptions (12.0f).withTypefaceStyle ("Italic")));
    g.drawText ("eight-voice polyphonic  -  slider edition",
                header.reduced (18.0f, 0.0f), juce::Justification::centredRight);

    // Header hairline
    g.setColour (LF::panelDark());
    g.drawLine (12.0f, header.getBottom() + 1.0f,
                bounds.getWidth() - 12.0f, header.getBottom() + 1.0f, 1.0f);

    // ---- Top tab bar (anchor labels, decorative for now) ----------------
    auto tabRow = bounds.removeFromTop (28.0f).reduced (12.0f, 4.0f);
    const juce::StringArray tabs { "PROGRAM", "OSC", "FILTER", "ENV", "MOD", "FX", "PATCH" };
    const float tabW = tabRow.getWidth() / 14.0f;
    g.setFont (juce::Font (juce::FontOptions (11.0f).withTypefaceStyle ("Bold")));
    auto tabCursor = tabRow.removeFromLeft (tabW * 7.0f);
    for (int i = 0; i < tabs.size(); ++i)
    {
        auto cell = tabCursor.removeFromLeft (tabW);
        const bool selected = (i == 1); // OSC highlighted as the "current" anchor
        g.setColour (selected ? LF::panelAccent() : LF::panelDark());
        g.drawText (tabs[i], cell, juce::Justification::centredLeft);
    }

    // Section frames
    for (auto& s : sections)
    {
        auto r = s.bounds.toFloat();
        if (r.isEmpty()) continue;

        g.setColour (LF::panelCream());
        g.fillRect (r);
        g.setColour (LF::panelDark());
        g.drawRect (r, 1.0f);

        // Numbered title bar at the top of each section
        auto titleStrip = r.removeFromTop (18.0f).reduced (6.0f, 0.0f);
        // Index number in accent red, label in dark
        const int idx = static_cast<int> (&s - &sections[0]) + 1;
        const auto indexStr = (idx < 10 ? juce::String ("0") + juce::String (idx)
                                        : juce::String (idx));

        g.setColour (LF::panelAccent());
        g.setFont (juce::Font (juce::FontOptions (10.5f).withTypefaceStyle ("Bold")));
        g.drawText (indexStr, titleStrip.removeFromLeft (20.0f),
                    juce::Justification::centredLeft);

        g.setColour (LF::panelDark());
        g.setFont (juce::Font (juce::FontOptions (10.5f).withTypefaceStyle ("Bold")));
        g.drawText (s.title, titleStrip, juce::Justification::centredLeft);
    }

    // ---- Footer (engineering title block) -------------------------------
    auto fullBounds = getLocalBounds().toFloat();
    auto footer = fullBounds.removeFromBottom (32.0f).reduced (12.0f, 4.0f);
    g.setColour (LF::panelDark());
    g.drawRect (footer, 1.0f);

    const char* fields[][2] = {
        { "TITLE",  "OB-8 NATIVE" },
        { "SERIES", "No. 0427  Rev. C  Fader Cut" },
        { "SCALE",  "1 : 1" },
        { "REV",    "C" },
        { "DATE",   "2026.05" },
    };
    const float colW = footer.getWidth() / 5.0f;
    g.setFont (juce::Font (juce::FontOptions (8.5f).withTypefaceStyle ("Bold")));
    for (int i = 0; i < 5; ++i)
    {
        auto cell = footer.removeFromLeft (colW);
        if (i > 0)
        {
            g.setColour (LF::panelDark());
            g.drawLine (cell.getX(), cell.getY(), cell.getX(), cell.getBottom(), 0.6f);
        }
        auto inner = cell.reduced (6.0f, 4.0f);
        g.setColour (LF::panelMute());
        g.drawText (fields[i][0], inner.removeFromTop (10.0f),
                    juce::Justification::centredLeft);
        g.setColour (LF::panelDark());
        g.setFont (juce::Font (juce::FontOptions (10.5f).withTypefaceStyle ("Bold")));
        g.drawText (fields[i][1], inner, juce::Justification::centredLeft);
        g.setFont (juce::Font (juce::FontOptions (8.5f).withTypefaceStyle ("Bold")));
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
    auto bounds = getLocalBounds();
    // Header (52 px) + tab anchor row (28 px) drawn in paint()
    bounds.removeFromTop (80);
    // Engineering title-block footer (32 px) drawn in paint()
    bounds.removeFromBottom (32);
    bounds = bounds.reduced (10);

    // Reserve space at the bottom for the on-screen keyboard. Done first so
    // the existing top-down section layout keeps working against `bounds`.
    const int kKbdH = juce::jmin (100, juce::jmax (70, bounds.getHeight() / 9));
    auto kbdBounds = bounds.removeFromBottom (kKbdH);

    // Octave shift controls share the keyboard row on the left side
    auto octBounds = kbdBounds.removeFromLeft (90);
    octBounds.reduce (4, 4);
    auto labelStrip = octBounds.removeFromTop (24);
    octaveLabel.setBounds (labelStrip);
    auto btnStrip   = octBounds.removeFromTop (32);
    octDownBtn.setBounds (btnStrip.removeFromLeft (btnStrip.getWidth() / 2).reduced (2));
    octUpBtn  .setBounds (btnStrip.reduced (2));

    bounds.removeFromBottom (8);
    keyboard.setBounds (kbdBounds);

    // Allocate the four rows proportionally so the window resizes cleanly.
    // Weights (30/24/26/20) keep the knob clusters legible while leaving
    // a reasonable strip for the new FX row at the bottom of the panel.
    const int availH = bounds.getHeight() - 3 * 8;   // minus three inter-row gaps
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

    bounds.removeFromTop (8);

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
    layoutSection (sections[9], row2,                        4);

    bounds.removeFromTop (8);

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

    bounds.removeFromTop (8);

    // Row 4: DELAY | REVERB (post-effects)
    auto row4 = bounds;
    const int wDelay = (row4.getWidth() - 8) * 6 / 13;  // 6 controls vs 7
    layoutSection (sections[13], row4.removeFromLeft (wDelay), 6);
    row4.removeFromLeft (8);
    layoutSection (sections[14], row4, 7);
}

} // namespace ob8
