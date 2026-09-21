#pragma once
#include <JuceHeader.h>

/**
 * AudioFIFO
 * A simple lock-free stereo audio FIFO used to pass audio blocks
 * from the audio thread to the FFTAnalyzer worker thread.
 *
 * Pushes interleaved (L+R averaged) mono samples into FFTAnalyzer.
 */
class AudioFIFO
{
public:
    explicit AudioFIFO(FFTAnalyzer& analyzer)
        : fftAnalyzer(analyzer)
    {}

    /** Called from the audio thread with the full stereo buffer. */
    void pushBuffer(const juce::AudioBuffer<float>& buffer)
    {
        const int numSamples  = buffer.getNumSamples();
        const int numChannels = juce::jmin(buffer.getNumChannels(), 2);

        const float* L = buffer.getReadPointer(0);
        const float* R = (numChannels > 1) ? buffer.getReadPointer(1) : L;

        for (int i = 0; i < numSamples; ++i)
        {
            // Use mono mix (L+R / 2) for spectrum display
            const float mono = (L[i] + R[i]) * 0.5f;
            fftAnalyzer.pushNextSampleIntoFifo(mono);
        }
    }

private:
    FFTAnalyzer& fftAnalyzer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFIFO)
};
