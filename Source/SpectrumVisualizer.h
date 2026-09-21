#pragma once
#include <JuceHeader.h>
#include "FFTAnalyzer.h"

/**
 * SpectrumVisualizer
 * Renders the real-time FFT spectrum as a filled path.
 *
 * Visual spec:
 *   Total area : 1200 x 600 px inside parent (AudioEditor places it correctly)
 *   Background : Dark purple
 *   Spectrum   : Semi-transparent filled path, same purple family
 *   Alignment  : Within parent, left/right margin=40, top=35, bottom=20
 */
class SpectrumVisualizer : public juce::Component,
                           public juce::Timer
{
public:
    explicit SpectrumVisualizer(FFTAnalyzer& analyzer)
        : fftAnalyzer(analyzer),
          spectrumColor(juce::Colour(0xAA9966FF)) // purple, ~67% alpha
    {
        fftData.fill(0.0f);
        startTimerHz(60); // 60 fps timer (GUI update rate)
    }

    ~SpectrumVisualizer() override { stopTimer(); }

    //=========================================================================
    void updateFFTData(const float* newData)
    {
        std::copy(newData, newData + FFTAnalyzer::scopeSize, fftData.begin());
        createPath();
        repaint();
    }

    //=========================================================================
    void paint(juce::Graphics& g) override
    {
        // Background
        g.fillAll(juce::Colour(0xFF1A0033)); // deep dark purple

        // Grid lines (subtle)
        g.setColour(juce::Colour(0x22FFFFFF));
        const auto bounds = getLocalBounds().toFloat();
        for (int i = 1; i < 4; ++i)
        {
            float x = bounds.getWidth() * i / 4.0f;
            g.drawVerticalLine(static_cast<int>(x), 0.0f, bounds.getHeight());
        }
        for (int i = 1; i < 4; ++i)
        {
            float y = bounds.getHeight() * i / 4.0f;
            g.drawHorizontalLine(static_cast<int>(y), 0.0f, bounds.getWidth());
        }

        // Spectrum fill
        if (!spectrumPath.isEmpty())
        {
            // Gradient: bright purple at top, darker at bottom
            juce::ColourGradient grad(
                juce::Colour(0xCC9966FF), 0.0f, 0.0f,
                juce::Colour(0x44330066), 0.0f, bounds.getHeight(),
                false);
            g.setGradientFill(grad);
            g.fillPath(spectrumPath);

            // Top outline
            g.setColour(juce::Colour(0xFFBB99FF));
            g.strokePath(spectrumPath, juce::PathStrokeType(1.2f));
        }
    }

    void resized() override { createPath(); }

    //=========================================================================
    void timerCallback() override
    {
        float tempData[FFTAnalyzer::scopeSize];
        if (fftAnalyzer.getNextFFTBlock(tempData))
            updateFFTData(tempData);
    }

private:
    void createPath()
    {
        const auto bounds = getLocalBounds().toFloat();
        if (bounds.isEmpty()) return;

        spectrumPath.clear();
        spectrumPath.startNewSubPath(0.0f, bounds.getHeight());

        for (int i = 0; i < FFTAnalyzer::scopeSize; ++i)
        {
            const float x = bounds.getWidth()  * static_cast<float>(i) / static_cast<float>(FFTAnalyzer::scopeSize - 1);
            const float y = bounds.getHeight() * (1.0f - fftData[static_cast<size_t>(i)]);
            spectrumPath.lineTo(x, y);
        }

        spectrumPath.lineTo(bounds.getWidth(), bounds.getHeight());
        spectrumPath.closeSubPath();
    }

    FFTAnalyzer&                       fftAnalyzer;
    juce::Colour                       spectrumColor;
    juce::Path                         spectrumPath;
    std::array<float, FFTAnalyzer::scopeSize> fftData;
    juce::Rectangle<int>               area;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumVisualizer)
};
