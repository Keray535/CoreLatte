#include "PluginEditor.h"

//==============================================================================
AudioEditor::AudioEditor(CoreLateAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      spectrumVisualizer(p.getFFTAnalyzer()),
      lowBandMeter (juce::Colour(0xFFFF3333)),  // Red  — Low
      midBandMeter (juce::Colour(0xFF33FF66)),  // Green — Mid
      highBandMeter(juce::Colour(0xFF3399FF))   // Blue  — High
{
    setSize(editorWidth, editorHeight);
    setResizable(false, false);

    addAndMakeVisible(spectrumVisualizer);
    addAndMakeVisible(lowBandMeter);
    addAndMakeVisible(midBandMeter);
    addAndMakeVisible(highBandMeter);

    // Timer at 60 fps for meter polling
    startTimerHz(60);
}

AudioEditor::~AudioEditor()
{
    stopTimer();
}

//==============================================================================
void AudioEditor::paint(juce::Graphics& g)
{
    // Overall background — very dark purple/black
    g.fillAll(juce::Colour(0xFF0D0020));

    // Plugin name / header
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font("Helvetica Neue", 14.0f, juce::Font::bold));
    g.drawText("CORE-LATE  |  Correlation & Spectrum Analyzer",
               40, 8, editorWidth - 80, 22, juce::Justification::centredLeft);

    // Band labels above meters
    const int meterY = editorHeight - 35 - 30; // bottom margin 35, meter height 30

    // lowBand label
    drawBandLabel(g, "LOW",   90, meterY - 16, 300, 14);
    // midBand label
    const int midX = 90 + 300 + 100;
    drawBandLabel(g, "MID",   midX, meterY - 16, 300, 14);
    // highBand label
    const int highX = midX + 300 + 100;
    drawBandLabel(g, "HIGH",  highX, meterY - 16, 300, 14);
}

void AudioEditor::drawBandLabel(juce::Graphics& g, const juce::String& text,
                                 int x, int y, int w, int h)
{
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(text, x, y, w, h, juce::Justification::centred);
}

//==============================================================================
void AudioEditor::resized()
{
    // ── Spectrum Visualizer ──────────────────────────────────────────────────
    // Area: 1200 x 600; margin L/R: 40, top: 35, bottom: 20
    spectrumVisualizer.setBounds(40, 35, 1200, 600);

    // ── Correlation Meters ───────────────────────────────────────────────────
    // Size: 300 x 30 each
    // lowBand  : margin-left 90px
    // midBand  : 100px gap after lowBand  → x = 90 + 300 + 100 = 490
    // highBand : 100px gap after midBand  → x = 490 + 300 + 100 = 890
    // All meters: margin-bottom 35px from bottom → y = 720 - 35 - 30 = 655

    const int meterH = 30;
    const int meterY = editorHeight - 35 - meterH; // 655

    lowBandMeter .setBounds(90,              meterY, 300, meterH);
    midBandMeter .setBounds(90 + 300 + 100,  meterY, 300, meterH);
    highBandMeter.setBounds(90 + 300 + 100 + 300 + 100, meterY, 300, meterH);
}

//==============================================================================
void AudioEditor::timerCallback()
{
    // Poll correlation values from the processor and push to meters
    lowBandMeter .updateValue(processor.getLowCorrelation());
    midBandMeter .updateValue(processor.getMidCorrelation());
    highBandMeter.updateValue(processor.getHighCorrelation());

    // SpectrumVisualizer has its own internal timer; no extra polling needed here.
}
