#pragma once
#include <JuceHeader.h>
#include "CrossoverManager.h"
#include "CorrelationAnalyzer.h"
#include "FFTAnalyzer.h"
#include "AudioFIFO.h"

/**
 * CoreLateAudioProcessor
 *
 * Main DSP processor for the CoreLate correlation meter plugin.
 * - Receives stereo input
 * - Splits into 3 bands via CrossoverManager (Linkwitz-Riley)
 * - Measures per-band correlation via CorrelationAnalyzer[3]
 * - Feeds FFT data to FFTAnalyzer via a lock-free AudioFIFO
 */
class CoreLateAudioProcessor : public juce::AudioProcessor
{
public:
    //==========================================================================
    CoreLateAudioProcessor();
    ~CoreLateAudioProcessor() override;

    //==========================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==========================================================================
    const juce::String getName() const override;

    bool   acceptsMidi() const override;
    bool   producesMidi() const override;
    bool   isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==========================================================================
    int  getNumPrograms() override;
    int  getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==========================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    // Public accessors for GUI
    float getLowCorrelation()  const { return correlationAnalyzers[0].getCorrelationValue(); }
    float getMidCorrelation()  const { return correlationAnalyzers[1].getCorrelationValue(); }
    float getHighCorrelation() const { return correlationAnalyzers[2].getCorrelationValue(); }

    FFTAnalyzer& getFFTAnalyzer() { return fftAnalyzer; }

private:
    CrossoverManager   crossoverManager;
    FFTAnalyzer        fftAnalyzer;
    CorrelationAnalyzer correlationAnalyzers[3]; // [0]=low, [1]=mid, [2]=high

    // AudioFIFO is constructed after fftAnalyzer, using a pointer to avoid
    // initialization-order issues
    std::unique_ptr<AudioFIFO> fifoToEditor;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CoreLateAudioProcessor)
};
