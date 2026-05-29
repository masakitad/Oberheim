#include "WaveformPreview.h"
#include "OB8LookAndFeel.h"

namespace ob8 {

namespace {

// Build a 88x36 unit-space path for one of the five canonical waveforms.
// Paths copied verbatim from design_handoff_hairline_viii/synth-controls-refined.jsx.
juce::Path buildWaveformPath (WaveformPreview::Kind kind)
{
    using Kind = WaveformPreview::Kind;
    juce::Path p;
    auto moveTo = [&] (float x, float y) { p.startNewSubPath (x, y); };
    auto lineTo = [&] (float x, float y) { p.lineTo (x, y); };

    switch (kind)
    {
        case Kind::Sine:
        {
            // M 4,18 Q 12,2 20,18 T 36,18 T 52,18 T 68,18 T 84,18
            // Smooth-curve quadratic chain. JUCE has Path::quadraticTo that
            // takes an explicit control point; we use the JS "T" smoothing
            // (reflect previous control around end point) manually.
            moveTo (4.0f, 18.0f);
            p.quadraticTo (12.0f, 2.0f, 20.0f, 18.0f);
            // T reflects the last control point through the current point.
            // Last control was (12,2), endpoint (20,18) -> next control (28, 34)
            p.quadraticTo (28.0f, 34.0f, 36.0f, 18.0f);
            p.quadraticTo (44.0f,  2.0f, 52.0f, 18.0f);
            p.quadraticTo (60.0f, 34.0f, 68.0f, 18.0f);
            p.quadraticTo (76.0f,  2.0f, 84.0f, 18.0f);
            break;
        }
        case Kind::Saw:
        {
            moveTo (4.0f, 30.0f);
            const float xs[] = { 14.0f, 24.0f, 34.0f, 44.0f, 54.0f, 64.0f, 74.0f };
            for (float x : xs) { lineTo (x, 6.0f); lineTo (x, 30.0f); }
            lineTo (84.0f, 6.0f);
            break;
        }
        case Kind::Square:
        {
            moveTo (4.0f, 30.0f);
            lineTo (4.0f, 6.0f);
            const float xs[] = { 14.0f, 14.0f, 24.0f, 24.0f, 34.0f, 34.0f,
                                  44.0f, 44.0f, 54.0f, 54.0f, 64.0f, 64.0f,
                                  74.0f, 74.0f, 84.0f };
            const float ys[] = {  6.0f, 30.0f, 30.0f,  6.0f,  6.0f, 30.0f,
                                  30.0f,  6.0f,  6.0f, 30.0f, 30.0f,  6.0f,
                                   6.0f, 30.0f, 30.0f };
            for (size_t i = 0; i < sizeof (xs) / sizeof (xs[0]); ++i)
                lineTo (xs[i], ys[i]);
            break;
        }
        case Kind::Tri:
        {
            moveTo (4.0f, 18.0f);
            lineTo (14.0f,  4.0f);
            lineTo (24.0f, 32.0f);
            lineTo (34.0f,  4.0f);
            lineTo (44.0f, 32.0f);
            lineTo (54.0f,  4.0f);
            lineTo (64.0f, 32.0f);
            lineTo (74.0f,  4.0f);
            lineTo (84.0f, 18.0f);
            break;
        }
        case Kind::Noise:
        {
            const float pts[][2] = {
                {  4.0f, 22.0f }, {  8.0f, 12.0f }, { 12.0f, 28.0f },
                { 16.0f,  8.0f }, { 20.0f, 24.0f }, { 24.0f, 14.0f },
                { 28.0f, 30.0f }, { 32.0f, 10.0f }, { 36.0f, 26.0f },
                { 40.0f, 16.0f }, { 44.0f,  8.0f }, { 48.0f, 22.0f },
                { 52.0f, 14.0f }, { 56.0f, 28.0f }, { 60.0f, 18.0f },
                { 64.0f, 10.0f }, { 68.0f, 26.0f }, { 72.0f, 14.0f },
                { 76.0f, 22.0f }, { 80.0f, 12.0f }, { 84.0f, 18.0f },
            };
            moveTo (pts[0][0], pts[0][1]);
            for (size_t i = 1; i < sizeof (pts) / sizeof (pts[0]); ++i)
                lineTo (pts[i][0], pts[i][1]);
            break;
        }
    }
    return p;
}

} // namespace

WaveformPreview::WaveformPreview (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& paramID,
                                  const juce::String& labelText,
                                  std::vector<Kind> choiceToKind)
    : apvtsRef (apvts), paramId (paramID), kinds (std::move (choiceToKind))
{
    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (OB8LookAndFeel::monoRegular (9.0f).withExtraKerningFactor (0.18f));
    label.setColour (juce::Label::textColourId, OB8LookAndFeel::ink());
    addAndMakeVisible (label);

    apvtsRef.addParameterListener (paramId, this);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

WaveformPreview::~WaveformPreview()
{
    apvtsRef.removeParameterListener (paramId, this);
}

void WaveformPreview::resized()
{
    auto bounds = getLocalBounds();
    label.setBounds (bounds.removeFromTop (14));

    // ‹ [preview] ›
    const int arrowW = juce::jmin (14, bounds.getWidth() / 6);
    leftArrowBox  = bounds.removeFromLeft  (arrowW);
    rightArrowBox = bounds.removeFromRight (arrowW);

    // Constrain the preview to the design's 88x36 (~2.44:1) aspect ratio,
    // centred within the available area. Without this clamp the path gets
    // stretched into 5:1 verticals in tall narrow cells.
    auto avail = bounds.reduced (2, 2);
    constexpr float kAspect = 88.0f / 36.0f;
    int targetW = avail.getWidth();
    int targetH = static_cast<int> (std::round (targetW / kAspect));
    if (targetH > avail.getHeight())
    {
        targetH = avail.getHeight();
        targetW = static_cast<int> (std::round (targetH * kAspect));
    }
    previewBox = juce::Rectangle<int> (
        avail.getCentreX() - targetW / 2,
        avail.getCentreY() - targetH / 2,
        targetW, targetH);
}

void WaveformPreview::paint (juce::Graphics& g)
{
    using LF = OB8LookAndFeel;

    // ---- 1. Frame (88x36 spec, mapped to previewBox) ------------------------
    auto frame = previewBox.toFloat().reduced (0.5f);
    g.setColour (LF::hairFine());
    g.drawRect (frame, 1.0f);

    // ---- 2. Centre dashed mid-line (2px dash / 2px gap) ---------------------
    {
        const float midY = frame.getCentreY();
        g.setColour (LF::hairFine());
        for (float x = frame.getX() + 1.0f; x < frame.getRight(); x += 4.0f)
            g.drawLine (x, midY, juce::jmin (x + 2.0f, frame.getRight()), midY, 1.0f);
    }

    // ---- 3. Waveform path (accent, 1.5px) -----------------------------------
    {
        auto path = buildWaveformPath (currentKind());
        // SVG viewBox is 0..88 x 0..36 with 4..84 / 4..32 padding. Scale to the
        // frame with a small inset so the path doesn't kiss the border.
        const float vbW = 88.0f, vbH = 36.0f;
        const float sx = frame.getWidth()  / vbW;
        const float sy = frame.getHeight() / vbH;
        juce::AffineTransform t;
        t = t.scaled (sx, sy).translated (frame.getX(), frame.getY());
        path.applyTransform (t);
        g.setColour (LF::accent());
        g.strokePath (path, juce::PathStrokeType (1.5f,
                            juce::PathStrokeType::curved,
                            juce::PathStrokeType::rounded));
    }

    // ---- 4. Side arrows -----------------------------------------------------
    g.setColour (LF::inkDim());
    g.setFont   (LF::monoRegular (11.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"),  // ‹
                leftArrowBox, juce::Justification::centred, false);
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"),  // ›
                rightArrowBox, juce::Justification::centred, false);
}

void WaveformPreview::mouseDown (const juce::MouseEvent& e)
{
    if (leftArrowBox.contains (e.x, e.y))       cycle (-1);
    else if (rightArrowBox.contains (e.x, e.y)) cycle (+1);
    else if (previewBox.contains (e.x, e.y))    cycle (+1);
}

int WaveformPreview::currentIndex() const noexcept
{
    if (auto* p = apvtsRef.getParameter (paramId))
    {
        const int n = juce::jmax (1, static_cast<int> (kinds.size()));
        const float norm = p->getValue();
        return juce::jlimit (0, n - 1, static_cast<int> (std::round (norm * (n - 1))));
    }
    return 0;
}

WaveformPreview::Kind WaveformPreview::currentKind() const noexcept
{
    const int idx = currentIndex();
    if (idx >= 0 && idx < static_cast<int> (kinds.size()))
        return kinds[static_cast<size_t> (idx)];
    return Kind::Sine;
}

void WaveformPreview::cycle (int delta)
{
    if (auto* p = apvtsRef.getParameter (paramId))
    {
        const int n = juce::jmax (1, static_cast<int> (kinds.size()));
        int idx = (currentIndex() + delta + n) % n;
        const float norm = (n == 1) ? 0.0f : static_cast<float> (idx) / static_cast<float> (n - 1);
        p->beginChangeGesture();
        p->setValueNotifyingHost (norm);
        p->endChangeGesture();
    }
}

void WaveformPreview::parameterChanged (const juce::String&, float)
{
    juce::MessageManager::callAsync (
        [safe = juce::Component::SafePointer (this)]
    {
        if (safe != nullptr) safe->repaint();
    });
}

} // namespace ob8
