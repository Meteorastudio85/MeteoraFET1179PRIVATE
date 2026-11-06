#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

ChronoProcessor::ChronoProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ChronoProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<F> (juce::ParameterID { "in", 1 },    "Input",           juce::NormalisableRange<float> (-24.0f, 24.0f), 0.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "out", 1 },   "Output",          juce::NormalisableRange<float> (-24.0f, 24.0f), 0.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "atk", 1 },   "Attack",          juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    l.add (std::make_unique<F> (juce::ParameterID { "rel", 1 },   "Release",         juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    l.add (std::make_unique<F> (juce::ParameterID { "hpf", 1 },   "Hi-Pass Filter",  juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "lpf", 1 },   "Lo-Pass Filter",  juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "color", 1 }, "Curve", juce::NormalisableRange<float> (0.0f, 100.0f), 30.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "mix", 1 },   "Dry/Wet",         juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "ratio", 1 }, "Ratio",
                                                         juce::StringArray { "4:1", "8:1", "12:1", "20:1", "All Buttons" }, 0));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "power", 1 }, "Power", true));
    return l;
}

bool ChronoProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono())
           && out == l.getMainInputChannelSet();
}

void ChronoProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    engine.prepare (sr);
    runSm = P ("power") > 0.5f ? 1.0 : 0.0;
}

void ChronoProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    bypassed.store (false);
    lastProcessMs.store (juce::Time::getMillisecondCounter());
    const int n = buf.getNumSamples();
    const int nch = juce::jmin (2, buf.getNumChannels());
    if (nch == 0 || n == 0) return;

    engine.setParams (P ("in"), P ("hpf"), P ("lpf"), (int) std::lround (P ("ratio")), P ("atk"), P ("rel"),
                      P ("color") / 100.0, P ("out"), P ("mix") / 100.0);
    const double runTarget = P ("power") > 0.5f ? 1.0 : 0.0;
    const double step = 1.0 / (0.03 * sr);
    engine.preE = engine.postE = 0.0;

    float* l = buf.getWritePointer (0);
    float* r = nch > 1 ? buf.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        float x[2] = { l[i], r != nullptr ? r[i] : l[i] };
        runSm += juce::jlimit (-step, step, runTarget - runSm);
        engine.process (x, runSm);
        l[i] = x[0];
        if (r != nullptr) r[i] = x[1];
    }
    const double gr = engine.preE > 1.0e-9 ? 10.0 * std::log10 (juce::jmax (engine.postE, 1.0e-20) / engine.preE) : 0.0;
    grDb.store ((float) juce::jmin (0.0, gr));
}

juce::AudioProcessorEditor* ChronoProcessor::createEditor() { return new ChronoEditor (*this); }

void ChronoProcessor::getStateInformation (juce::MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, d);
}

void ChronoProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChronoProcessor(); }
