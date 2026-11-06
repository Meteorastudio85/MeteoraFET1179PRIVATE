#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace
{
    constexpr float kW = 1239.0f, kH = 864.0f;
    constexpr int kDefaultWidth = 1050;                // si apre a 1050 x 732
    constexpr const char* kSizeKey = "uiWidthV2";      // chiave nuova: ignora le misure piccole salvate per errore prima
    // mode: 0 = puntino scuro su pomello cromato, 1 = puntino chiaro su pomello nero, 2 = pomello che ruota
    struct ChronoKnob { const char* id; const char* name; float cx, cy, r; int mode; float baked; };
    const ChronoKnob kKnobs[8] = {
        { "in",    "Input",             488.0f, 304.0f, 90.0f, 2, 0.0f },
        { "out",   "Output",            806.0f, 305.0f, 86.0f, 2, 0.0f },
        { "color", "Curve",             814.0f, 546.0f, 45.0f, 2, 0.0f },   // pomello CURVE nella banda blu (Pultec + Tube)
        { "mix",   "Dry/Wet",           222.0f, 676.0f, 30.0f, 0, 0.0f },   // a sinistra dell'"11": dry/wet totale
        { "rel",   "Release",           389.0f, 539.0f, 30.0f, 1, 0.0f },
        { "atk",   "Attack",            567.0f, 541.0f, 30.0f, 0, 0.0f },
        { "hpf",   "Hi-Pass Filter",   1098.0f, 406.0f, 30.0f, 0, 0.0f },
        { "lpf",   "Lo-Pass Filter",   1097.0f, 540.0f, 30.0f, 0, 0.0f },
    };
    const float kRatioX[5] = { 512.0f, 570.0f, 628.0f, 685.0f, 742.0f };
    constexpr float kRatioY = 672.0f;
    constexpr float kSwX = 1085.0f, kSwY = 661.0f;
}

ChronoEditor::ChronoEditor (ChronoProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    bg = juce::ImageCache::getFromMemory (BinaryData::ui_jpg, BinaryData::ui_jpgSize);
    glowImg = juce::ImageCache::getFromMemory (BinaryData::logo_glow_png, BinaryData::logo_glow_pngSize);
    leverOn  = juce::ImageCache::getFromMemory (BinaryData::lever_on_png,  BinaryData::lever_on_pngSize);
    glassImg = juce::ImageCache::getFromMemory (BinaryData::vu_glass_png,  BinaryData::vu_glass_pngSize);
    leverOff = juce::ImageCache::getFromMemory (BinaryData::lever_off_png, BinaryData::lever_off_pngSize);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& s = knobs[i];
        s.setLookAndFeel (&lnf);
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setMouseDragSensitivity (240);
        addAndMakeVisible (s);
        atts.push_back (std::make_unique<juce::SliderParameterAttachment> (*proc.apvts.getParameter (kKnobs[i].id), s));
    }
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i].onValueChange = [this, i] { lastKnob = (int) i; repaint(); };

    // la dimensione va letta PRIMA dei limiti: impostarli fa subito un primo ridimensionamento
    // (era questo che faceva aprire il plugin piccolissimo)
    const int w = juce::jlimit (620, 1859, (int) proc.apvts.state.getProperty (kSizeKey, kDefaultWidth));
    setResizable (true, true);
    setResizeLimits (620, 432, 1859, 1296);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (w, juce::roundToInt ((float) w * kH / kW));
    sizeReady = true;
    resized();
    startTimerHz (30);
}

ChronoEditor::~ChronoEditor()
{
    for (auto& s : knobs) s.setLookAndFeel (nullptr);
}

void ChronoEditor::resized()
{
    if (getWidth() <= 0 || getHeight() <= 0) return;
    if (sizeReady) proc.apvts.state.setProperty (kSizeKey, getWidth(), nullptr);   // la dimensione scelta viene ricordata
    bgK = juce::jmax (1.0f, juce::Component::getApproximateScaleFactorForComponent (this));
    bgScaled = bg.rescaled (juce::roundToInt ((float) getWidth() * bgK), juce::roundToInt ((float) getHeight() * bgK),
                            juce::Graphics::highResamplingQuality);
    const float s = (float) getWidth() / kW;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto& k = kKnobs[i];
        knobs[i].setBounds (juce::Rectangle<float> ((k.cx - k.r) * s, (k.cy - k.r) * s, 2.0f * k.r * s, 2.0f * k.r * s).toNearestInt());
    }
}

void ChronoEditor::setParam (const char* id, float plainValue)
{
    if (auto* prm = proc.apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
        prm->endChangeGesture();
    }
}

juce::String ChronoEditor::valueText (int i) const
{
    const double v = knobs[(size_t) i].getValue();
    const juce::String id (kKnobs[i].id);
    auto hz = [] (double f) { return f >= 1000.0 ? juce::String (f / 1000.0, 1) + " kHz" : juce::String ((int) f) + " Hz"; };
    if (id == "in" || id == "out") return (v >= 0.0 ? "+" : "") + juce::String (v, 1) + " dB";
    if (id == "atk") return "~" + juce::String (ChronoEngine::atkMs (v), 1) + " ms";
    if (id == "rel") { const double ms = ChronoEngine::relMs (v); return "~" + (ms >= 1000.0 ? juce::String (ms / 1000.0, 2) + " s" : juce::String ((int) ms) + " ms"); }
    if (id == "hpf") return v < 0.02 ? juce::String ("OFF") : hz (20.0 * std::pow (1000.0, (v - 0.02) / 0.98));
    if (id == "lpf") return v < 0.02 ? juce::String ("OFF") : hz (20000.0 * std::pow (0.001, (v - 0.02) / 0.98));
    return juce::String ((int) std::lround (v)) + " %";
}

void ChronoEditor::paint (juce::Graphics& g)
{
    const float s = (float) getWidth() / kW;
    auto sc = [s] (float v) { return v * s; };
    const auto base = juce::AffineTransform::scale (1.0f / bgK);
    g.drawImageTransformed (bgScaled, base);

    // logo: si accende come il filamento di una valvola quando il plugin lavora (insieme al VU)
    if (tube > 0.003f)
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.setOpacity (juce::jlimit (0.0f, 1.0f, std::pow (tube, 1.4f) * flick));
        g.drawImage (glowImg, juce::Rectangle<float> (sc (112.0f), sc (239.5f), sc (220.0f), sc (220.5f)));
    }

    // spia rossa
    {
        const float cx = sc (225.0f), cy = sc (192.0f);
        if (glow < 0.999f)
        {
            g.setColour (juce::Colour (0xff120808).withAlpha (0.65f * (1.0f - glow)));
            g.fillEllipse (cx - sc (13.0f), cy - sc (13.0f), sc (26.0f), sc (26.0f));
        }
        if (glow > 0.001f)
        {
            juce::ColourGradient cg (juce::Colour (0xffff3020).withAlpha (0.38f * glow), cx, cy, juce::Colour (0xffff3020).withAlpha (0.0f), cx + sc (30.0f), cy, true);
            g.setGradientFill (cg);
            g.fillEllipse (cx - sc (30.0f), cy - sc (30.0f), sc (60.0f), sc (60.0f));
        }
    }

    // manopole
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto& k = kKnobs[i];
        const double prop = knobs[i].valueToProportionOfLength (knobs[i].getValue());
        const float deg = (float) (-135.0 + 270.0 * prop);
        if (k.mode == 2)
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path c;
            c.addEllipse (sc (k.cx - k.r), sc (k.cy - k.r), sc (2.0f * k.r), sc (2.0f * k.r));
            g.reduceClipRegion (c);
            g.drawImageTransformed (bgScaled, base.rotated (juce::degreesToRadians (deg - k.baked), sc (k.cx), sc (k.cy)));
        }
        else
        {
            const float a = juce::degreesToRadians (deg);
            const float x = sc (k.cx + std::sin (a) * 17.0f), y = sc (k.cy - std::cos (a) * 17.0f);
            g.setColour (k.mode == 1 ? juce::Colours::black.withAlpha (0.5f) : juce::Colours::white.withAlpha (0.22f));
            g.fillEllipse (x - sc (3.3f), y - sc (3.0f), sc (6.6f), sc (6.6f));
            g.setColour (k.mode == 1 ? juce::Colour (0xffe6dfcf) : juce::Colour (0xff17140f));
            g.fillEllipse (x - sc (2.5f), y - sc (2.5f), sc (5.0f), sc (5.0f));
        }
    }

    // tasti del rapporto: quello scelto si illumina
    {
        const int ratio = juce::jlimit (0, 4, (int) std::lround (P ("ratio")));
        for (int i = 0; i < 5; ++i)
        {
            const auto r = juce::Rectangle<float> (sc (52.0f), sc (56.0f)).withCentre ({ sc (kRatioX[i]), sc (kRatioY) });
            if (i == ratio)
            {
                juce::ColourGradient cg (juce::Colour (0xffffe2b0).withAlpha (0.30f), r.getCentreX(), r.getCentreY(),
                                         juce::Colour (0xffffe2b0).withAlpha (0.04f), r.getCentreX() + sc (30.0f), r.getCentreY(), true);
                g.setGradientFill (cg);
                g.fillRoundedRectangle (r.reduced (sc (3.0f)), sc (5.0f));
                g.setColour (juce::Colour (0xffffd59a).withAlpha (0.55f));
                g.drawRoundedRectangle (r.reduced (sc (3.0f)), sc (5.0f), sc (1.2f));
            }
        }
    }

    // VU: riduzione di guadagno (a riposo sullo 0, scende a sinistra quando comprime)
    {
        const juce::Rectangle<float> face (sc (997.0f), sc (185.0f), sc (186.0f), sc (100.0f));
        const float p = juce::jlimit (0.0f, 1.12f, std::pow (10.0f, needle / 20.0f) / 1.4125f);
        const float a = juce::degreesToRadians (-53.9f + 100.0f * p);
        const float px = 1087.0f, py = 298.0f;
        auto pt = [&] (float rr) { return juce::Point<float> (sc (px + std::sin (a) * rr), sc (py - std::cos (a) * rr)); };
        const juce::Point<float> off (sc (1.5f), sc (1.5f));
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawLine (juce::Line<float> (pt (0.0f) + off, pt (80.0f) + off), sc (1.6f));
        g.setColour (juce::Colour (0xff1d150f));
        g.drawLine (juce::Line<float> (pt (0.0f), pt (80.0f)), sc (1.6f));
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path hub;
            hub.addEllipse (sc (1065.0f), sc (269.0f), sc (42.0f), sc (22.0f));
            g.reduceClipRegion (hub);
            g.drawImageTransformed (bgScaled, base);
        }
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (juce::Rectangle<float> (sc (1046.0f), sc (285.0f), sc (84.0f), sc (30.0f)).getSmallestIntegerContainer());
            g.drawImageTransformed (bgScaled, base);
        }
        // retroilluminazione spenta: si scuriscono insieme quadrante, lancetta e perno
        if (glow < 0.999f)
        {
            g.setColour (juce::Colour (0xff0e0c0a).withAlpha (0.72f * (1.0f - glow)));
            g.fillRoundedRectangle (face, sc (8.0f));
        }
        // vetro davanti al quadrante: riflessi, ombra della cornice, un filo di polvere
        g.drawImage (glassImg, juce::Rectangle<float> (sc (998.5f), sc (187.0f), sc (182.0f), sc (95.5f)));
    }

    // interruttore POWER: la levetta originale della foto (a destra = acceso, a sinistra = spento)
    {
        const bool on = P ("power") > 0.5f;
        g.drawImage (on ? leverOn : leverOff, juce::Rectangle<float> (sc (1020.0f), sc (628.0f), sc (130.0f), sc (72.0f)));
    }

    if (lastKnob >= 0)
    {
        auto d = juce::Rectangle<float> (sc (440.0f), sc (40.0f), sc (360.0f), sc (32.0f));
        g.setColour (juce::Colour (0xcc111310));
        g.fillRoundedRectangle (d, sc (6.0f));
        g.setColour (juce::Colour (0xffe8c49a));
        g.setFont (sc (17.0f));
        g.drawText (juce::String (kKnobs[lastKnob].name) + "   " + valueText (lastKnob), d.toNearestInt(), juce::Justification::centred);
    }
}

void ChronoEditor::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        const std::array<int, 6> pcts { 50, 67, 75, 100, 125, 150 };
        juce::PopupMenu m;
        for (int i = 0; i < (int) pcts.size(); ++i)
        {
            const int w = juce::roundToInt (kW * (float) pcts[(size_t) i] / 100.0f);
            m.addItem (i + 1, "Size " + juce::String (pcts[(size_t) i]) + " %", true, std::abs (getWidth() - w) < 4);
        }
        juce::Component::SafePointer<ChronoEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withMousePosition(), [safe, pcts] (int r)
        {
            if (safe == nullptr || r <= 0) return;
            const int w = juce::roundToInt (kW * (float) pcts[(size_t) (r - 1)] / 100.0f);
            safe->setSize (w, juce::roundToInt ((float) w * kH / kW));
        });
        return;
    }
    const auto p = e.position / ((float) getWidth() / kW);
    for (int i = 0; i < 5; ++i)
        if (juce::Rectangle<float> (54.0f, 58.0f).withCentre ({ kRatioX[i], kRatioY }).contains (p)) { setParam ("ratio", (float) i); repaint(); return; }
    if (juce::Rectangle<float> (165.0f, 80.0f).withCentre ({ kSwX, kSwY }).contains (p))
    {
        setParam ("power", P ("power") > 0.5f ? 0.0f : 1.0f);
        repaint();
    }
}

void ChronoEditor::timerCallback()
{
    const bool running = proc.isRunning();
    const float prevGlow = glow, prevNeedle = needle;
    glow += ((running ? 1.0f : 0.0f) - glow) * 0.12f;
    // valvola: si scalda in circa un secondo e mezzo, si spegne piu' in fretta, e da accesa tremola appena
    const float prevTube = tube, prevFlick = flick;
    if (running) tube += (1.0f - tube) * 0.045f; else tube *= 0.88f;
    if (tube < 0.002f) tube = 0.0f;
    flick = running ? 0.96f + 0.04f * rnd.nextFloat() : 1.0f;
    if (std::abs (tube - prevTube) > 0.002f || std::abs (flick - prevFlick) > 0.004f)
    {
        const float sl = (float) getWidth() / kW;
        repaint (juce::Rectangle<float> (112.0f * sl, 239.5f * sl, 220.0f * sl, 220.5f * sl).getSmallestIntegerContainer());
    }
    needle += ((running ? proc.grDb.load() : 0.0f) - needle) * 0.3f;
    const bool power = P ("power") > 0.5f;
    const int ratio = (int) std::lround (P ("ratio"));
    if (std::abs (glow - prevGlow) > 0.003f || std::abs (needle - prevNeedle) > 0.02f || power != lastPower || ratio != lastRatio)
        repaint();
    lastPower = power;
    lastRatio = ratio;
}
