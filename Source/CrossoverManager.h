#pragma once
#include <JuceHeader.h>

/**
 * CrossoverManager
 * Implements a two-stage 4th-order Linkwitz-Riley crossover network.
 *
 * Band splits:
 *   Low  : 20Hz  - 200Hz
 *   Mid  : 200Hz - 3000Hz
 *   High : 3000Hz - 20kHz
 *
 * Inherits from juce::AudioProcessor for prepare/process lifecycle compatibility.
 * Used as a member of CoreLateAudioProcessor (composition, not inheritance).
 */
class CrossoverManager : public juce::AudioProcessor
{
public:
    CrossoverManager()
    {
        // Default crossover frequencies
        lowMidFreq  = 200.0f;
        midHighFreq = 3000.0f;
    }

    ~CrossoverManager() override = default;

    //==========================================================================
    // juce::AudioProcessor pure virtual overrides (stub – managed by host via
    // CoreLateAudioProcessor; these are only needed because of the base class).
    const juce::String getName() const override { return "CrossoverManager"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}

    //==========================================================================
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        currentSampleRate = spec.sampleRate;

        lowBuffer .setSize(2, static_cast<int>(spec.maximumBlockSize));
        midBuffer .setSize(2, static_cast<int>(spec.maximumBlockSize));
        highBuffer.setSize(2, static_cast<int>(spec.maximumBlockSize));

        updateFilters();

        for (auto& f : lowPassLM)   f.prepare(spec);
        for (auto& f : highPassLM)  f.prepare(spec);
        for (auto& f : lowPassMH)   f.prepare(spec);
        for (auto& f : highPassMH)  f.prepare(spec);

        reset();
    }

    /**
     * Split inputBuffer (stereo) into low / mid / high bands.
     * Each output buffer will contain 2 channels with numSamples samples.
     */
    void process(const juce::AudioBuffer<float>& inputBuffer)
    {
        const int numSamples  = inputBuffer.getNumSamples();
        const int numChannels = juce::jmin(inputBuffer.getNumChannels(), 2);

        // Resize output buffers if needed
        lowBuffer .setSize(2, numSamples, false, false, true);
        midBuffer .setSize(2, numSamples, false, false, true);
        highBuffer.setSize(2, numSamples, false, false, true);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            // --- Stage 1: Low-Mid split at lowMidFreq ---
            // LP → low band
            // HP → mid+high

            juce::AudioBuffer<float> lpBuf(1, numSamples);
            juce::AudioBuffer<float> hpBuf(1, numSamples);

            lpBuf.copyFrom(0, 0, inputBuffer, ch, 0, numSamples);
            hpBuf.copyFrom(0, 0, inputBuffer, ch, 0, numSamples);

            {
                juce::dsp::AudioBlock<float> lpBlock(lpBuf);
                juce::dsp::ProcessContextReplacing<float> ctx(lpBlock);
                lowPassLM[ch].process(ctx);
            }
            {
                juce::dsp::AudioBlock<float> hpBlock(hpBuf);
                juce::dsp::ProcessContextReplacing<float> ctx(hpBlock);
                highPassLM[ch].process(ctx);
            }

            // Low band is complete
            lowBuffer.copyFrom(ch, 0, lpBuf, 0, 0, numSamples);

            // --- Stage 2: Mid-High split at midHighFreq ---
            juce::AudioBuffer<float> midBuf (1, numSamples);
            juce::AudioBuffer<float> highBuf(1, numSamples);

            midBuf .copyFrom(0, 0, hpBuf, 0, 0, numSamples);
            highBuf.copyFrom(0, 0, hpBuf, 0, 0, numSamples);

            {
                juce::dsp::AudioBlock<float> midBlock(midBuf);
                juce::dsp::ProcessContextReplacing<float> ctx(midBlock);
                lowPassMH[ch].process(ctx);
            }
            {
                juce::dsp::AudioBlock<float> highBlock(highBuf);
                juce::dsp::ProcessContextReplacing<float> ctx(highBlock);
                highPassMH[ch].process(ctx);
            }

            midBuffer .copyFrom(ch, 0, midBuf,  0, 0, numSamples);
            highBuffer.copyFrom(ch, 0, highBuf, 0, 0, numSamples);
        }
    }

    const juce::AudioBuffer<float>& getLowBandBuffer()  const { return lowBuffer;  }
    const juce::AudioBuffer<float>& getMidBandBuffer()  const { return midBuffer;  }
    const juce::AudioBuffer<float>& getHighBandBuffer() const { return highBuffer; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
        spec.numChannels      = 2;
        prepare(spec);
    }

    void releaseResources() override { reset(); }

private:
    void updateFilters()
    {
        jassert(currentSampleRate > 0.0);

        // 4th-order L-R = two cascaded 2nd-order Butterworth
        auto makeLR4LP = [&](juce::dsp::LinkwitzRileyFilter<float>& f, float freq)
        {
            f.setCutoffFrequency(freq);
            f.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        };
        auto makeLR4HP = [&](juce::dsp::LinkwitzRileyFilter<float>& f, float freq)
        {
            f.setCutoffFrequency(freq);
            f.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        };

        for (int ch = 0; ch < 2; ++ch)
        {
            makeLR4LP(lowPassLM [ch], lowMidFreq);
            makeLR4HP(highPassLM[ch], lowMidFreq);
            makeLR4LP(lowPassMH [ch], midHighFreq);
            makeLR4HP(highPassMH[ch], midHighFreq);
        }
    }

    void reset()
    {
        for (auto& f : lowPassLM)  f.reset();
        for (auto& f : highPassLM) f.reset();
        for (auto& f : lowPassMH)  f.reset();
        for (auto& f : highPassMH) f.reset();
    }

    float  lowMidFreq      = 200.0f;
    float  midHighFreq     = 3000.0f;
    double currentSampleRate = 44100.0;

    // Per-channel filters (stereo → index 0=L, 1=R)
    juce::dsp::LinkwitzRileyFilter<float> lowPassLM [2];
    juce::dsp::LinkwitzRileyFilter<float> highPassLM[2];
    juce::dsp::LinkwitzRileyFilter<float> lowPassMH [2];
    juce::dsp::LinkwitzRileyFilter<float> highPassMH[2];

    juce::AudioBuffer<float> lowBuffer;
    juce::AudioBuffer<float> midBuffer;
    juce::AudioBuffer<float> highBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossoverManager)
};
