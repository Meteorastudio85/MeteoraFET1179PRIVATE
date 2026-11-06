#pragma once
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class ChronoInvisibleLnF : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override {}
};

class ChronoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ChronoEditor (ChronoProcessor&);
    ~ChronoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::String valueText (int i) const;
    float P (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void setParam (const char* id, float plainValue);

    ChronoProcessor& proc;
    ChronoInvisibleLnF lnf;
    juce::Image bg, bgScaled, glowImg, leverOn, leverOff, glassImg;
    float bgK = 1.0f;
    std::array<juce::Slider, 8> knobs;
    std::vector<std::unique_ptr<juce::SliderParameterAttachment>> atts;
    int lastKnob = -1, lastRatio = -1;
    float glow = 0.0f, needle = 0.0f, tube = 0.0f, flick = 1.0f;
    juce::Random rnd;
    bool lastPower = false, sizeReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChronoEditor)
};
