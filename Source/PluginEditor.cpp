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
    keyboard.setLowestVisibleKey (36);            // start at C2
    keyboard.setOctaveForMiddleC (4);             // C4 = MIDI 60
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
                                                masterGain.get(), masterTune.get(), bendRange.get() } });

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

    for (auto& s : sections)
        for (auto* c : s.children)
            addAndMakeVisible (c);

    setSize (1280, 880);
}

OB8Editor::~OB8Editor()
{
    setLookAndFeel (nullptr);
}

void OB8Editor::visibilityChanged()
{
    // Once the editor is on-screen, hand keyboard focus to the on-screen
    // keyboard so PC key events go directly to it.
    if (isShowing())
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer (this)]
        {
            if (safe != nullptr) safe->keyboard.grabKeyboardFocus();
        });
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
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient bg (OB8LookAndFeel::panelBlue().brighter (0.05f),
                             0, 0,
                             OB8LookAndFeel::panelBlue().darker (0.4f),
                             0, bounds.getHeight(),
                             false);
    g.setGradientFill (bg);
    g.fillRect (bounds);

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

    for (auto& s : sections)
    {
        auto r = s.bounds.toFloat();
        if (r.isEmpty()) continue;
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
    bounds.removeFromTop (44);
    bounds = bounds.reduced (10);

    // Reserve space at the bottom for the on-screen keyboard. Done first so
    // the existing top-down section layout keeps working against `bounds`.
    const int kKbdH = 100;
    auto kbdBounds = bounds.removeFromBottom (kKbdH);
    bounds.removeFromBottom (8);
    keyboard.setBounds (kbdBounds);

    const int rowH1 = 230;
    const int rowH2 = 200;

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
    layoutSection (sections[9], row2,                        3);

    bounds.removeFromTop (8);

    // Row 3: PAGE 2 | SPLIT/DOUBLE | PATCH BANK (hand-laid out)
    auto row3 = bounds;
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
}

} // namespace ob8
