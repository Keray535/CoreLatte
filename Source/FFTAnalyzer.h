#pragma once
#include <JuceHeader.h>

/**
 * FFTAnalyzer
 * Background worker thread that consumes audio samples via a lock-free FIFO,
 * performs Blackman-Harris windowed FFT analysis (2048 samples), and exposes
 * the magnitude spectrum for GUI consumption.
 *
 * Audio thread   →  pushNextSampleIntoFifo()  →  AbstractFifo ring buffer
 * Worker thread  →  performFFT()              →  fftData[]  (atomic swap)
 * GUI thread     →  getNextFFTBlock()          →  reads fftData[]
 */
class FFTAnalyzer : public juce::Thread
{
public:
    static constexpr int fftOrder      = 11;          // 2^11 = 2048
    static constexpr int fftSize       = 1 << fftOrder;
    static constexpr int scopeSize     = fftSize / 2; // unique bins
    static constexpr int fifoCapacity  = 4096;

    FFTAnalyzer()
        : juce::Thread("FFTAnalyzer"),
          forwardFFT(fftOrder),
          window(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris),
          fifo(fifoCapacity),
          nextFFTBlockReady(false)
    {
        fifoBuffer.fill(0.0f);
        fftData.fill(0.0f);
    }

    ~FFTAnalyzer() override
    {
        stopThread(2000);
    }

    //=========================================================================
    /** Called from the audio thread – lock-free. */
    void pushNextSampleIntoFifo(float sample) noexcept
    {
        int start1, size1, start2, size2;
        fifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0) fifoBuffer[static_cast<size_t>(start1)] = sample;
        else if (size2 > 0) fifoBuffer[static_cast<size_t>(start2)] = sample;

        fifo.finishedWrite(size1 + size2);
    }

    //=========================================================================
    /** Called from the GUI thread. Returns true and fills destData if a new
     *  FFT block is available, otherwise returns false. */
    bool getNextFFTBlock(float* destData) noexcept
    {
        if (!nextFFTBlockReady.load(std::memory_order_acquire))
            return false;

        std::copy(fftData.begin(), fftData.begin() + scopeSize, destData);
        nextFFTBlockReady.store(false, std::memory_order_release);
        return true;
    }

    //=========================================================================
    /** juce::Thread entry point */
    void run() override
    {
        std::array<float, fftSize> workBuffer{};

        while (!threadShouldExit())
        {
            // Drain FIFO into workBuffer
            const int available = fifo.getNumReady();
            if (available >= fftSize)
            {
                int start1, size1, start2, size2;
                fifo.prepareToRead(fftSize, start1, size1, start2, size2);

                for (int i = 0; i < size1; ++i)
                    workBuffer[static_cast<size_t>(i)] = fifoBuffer[static_cast<size_t>(start1 + i)];
                for (int i = 0; i < size2; ++i)
                    workBuffer[static_cast<size_t>(size1 + i)] = fifoBuffer[static_cast<size_t>(start2 + i)];

                fifo.finishedRead(size1 + size2);

                // Copy to fftData and perform windowed FFT
                std::copy(workBuffer.begin(), workBuffer.end(), fftData.begin());
                window.multiplyWithWindowingTable(fftData.data(), static_cast<size_t>(fftSize));
                forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

                // Convert to dB magnitude, normalised
                for (int i = 0; i < scopeSize; ++i)
                {
                    float mag = fftData[static_cast<size_t>(i)];
                    fftData[static_cast<size_t>(i)] = juce::jlimit(0.0f, 1.0f,
                        (juce::Decibels::gainToDecibels(mag) + 100.0f) / 100.0f);
                }

                nextFFTBlockReady.store(true, std::memory_order_release);
            }
            else
            {
                wait(5); // sleep until more data arrives
            }
        }
    }

private:
    juce::dsp::FFT                        forwardFFT;
    juce::dsp::WindowingFunction<float>   window;

    // Lock-free ring buffer
    juce::AbstractFifo                    fifo;
    std::array<float, fifoCapacity>       fifoBuffer;

    // Double-buffer for FFT result (written by worker, read by GUI)
    std::array<float, fftSize>            fftData;
    std::atomic<bool>                     nextFFTBlockReady;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTAnalyzer)
};
