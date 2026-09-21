#pragma once
#include <JuceHeader.h>

/**
 * CorrelationAnalyzer
 * Measures phase similarity (correlation) between left and right channels
 * of a stereo audio buffer in a given frequency band.
 * Output range: -1 (inverse) to +1 (positive correlation)
 *
 * Uses running RMS-based formula:
 *   correlation = sumLR / sqrt(sumL2 * sumR2)
 * with exponential smoothing (alpha).
 */
class CorrelationAnalyzer
{
public:
    CorrelationAnalyzer()
        : currentCorrelation(0.0f),
          alpha(0.05f),   // smoothing factor (lower = slower response)
          sumLR(0.0f),
          sumL2(0.0f),
          sumR2(0.0f)
    {}

    /**
     * Process a band-separated stereo buffer.
     * bandBuffer must have exactly 2 channels (L, R).
     */
    void process(const juce::AudioBuffer<float>& bandBuffer)
    {
        jassert(bandBuffer.getNumChannels() >= 2);

        const int numSamples = bandBuffer.getNumSamples();
        const float* L = bandBuffer.getReadPointer(0);
        const float* R = bandBuffer.getReadPointer(1);

        float instantLR = 0.0f;
        float instantL2 = 0.0f;
        float instantR2 = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            instantLR += L[i] * R[i];
            instantL2 += L[i] * L[i];
            instantR2 += R[i] * R[i];
        }

        instantLR /= static_cast<float>(numSamples);
        instantL2 /= static_cast<float>(numSamples);
        instantR2 /= static_cast<float>(numSamples);

        // Exponential moving average
        sumLR = alpha * instantLR + (1.0f - alpha) * sumLR;
        sumL2 = alpha * instantL2 + (1.0f - alpha) * sumL2;
        sumR2 = alpha * instantR2 + (1.0f - alpha) * sumR2;

        const float denom = std::sqrt(sumL2 * sumR2);

        if (denom > 1e-10f)
            currentCorrelation = juce::jlimit(-1.0f, 1.0f, sumLR / denom);
        else
            currentCorrelation = 0.0f;
    }

    float getCorrelationValue() const { return currentCorrelation; }

    void reset()
    {
        currentCorrelation = 0.0f;
        sumLR = 0.0f;
        sumL2 = 0.0f;
        sumR2 = 0.0f;
    }

    /** Adjust smoothing: 0.0 (max smooth) to 1.0 (no smoothing) */
    void setAlpha(float newAlpha) { alpha = juce::jlimit(0.001f, 1.0f, newAlpha); }

private:
    float currentCorrelation;
    float alpha;
    float sumLR;
    float sumL2;
    float sumR2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CorrelationAnalyzer)
};
