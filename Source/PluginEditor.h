#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectrumVisualizer.h"
#include "CorrelationMeter.h"


class AudioEditor : public juce::AudioProcessorEditor,
                    public juce::Timer
{
public:
    explicit AudioEditor(CoreLateAudioProcessor& p);
    ~AudioEditor() override;

    //=========================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    CoreLateAudioProcessor& processor;

    SpectrumVisualizer spectrumVisualizer;

    // Band meters
    CorrelationMeter lowBandMeter;
    CorrelationMeter midBandMeter;
    CorrelationMeter highBandMeter;

    // Band label text
    void drawBandLabel(juce::Graphics& g, const juce::String& text,
                       int x, int y, int w, int h);

    static constexpr int editorWidth  = 1280;
    static constexpr int editorHeight = 720;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEditor)
};
