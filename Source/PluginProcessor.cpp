#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CoreLateAudioProcessor::CoreLateAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    fifoToEditor = std::make_unique<AudioFIFO>(fftAnalyzer);
}

CoreLateAudioProcessor::~CoreLateAudioProcessor()
{
    fftAnalyzer.stopThread(2000);
}

//==============================================================================
const juce::String CoreLateAudioProcessor::getName() const { return JucePlugin_Name; }
bool CoreLateAudioProcessor::acceptsMidi() const           { return false; }
bool CoreLateAudioProcessor::producesMidi() const          { return false; }
bool CoreLateAudioProcessor::isMidiEffect() const          { return false; }
double CoreLateAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int CoreLateAudioProcessor::getNumPrograms()               { return 1; }
int CoreLateAudioProcessor::getCurrentProgram()            { return 0; }
void CoreLateAudioProcessor::setCurrentProgram(int)        {}
const juce::String CoreLateAudioProcessor::getProgramName(int) { return {}; }
void CoreLateAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void CoreLateAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Prepare crossover filters
    crossoverManager.prepareToPlay(sampleRate, samplesPerBlock);

    // Reset all correlation analyzers and tune smoothing to sample rate
    for (auto& ca : correlationAnalyzers)
    {
        ca.reset();
        // Alpha roughly corresponds to ~50ms averaging at given sample rate
        const float alpha = 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.05f));
        ca.setAlpha(alpha);
    }

    // Start FFT worker thread
    if (!fftAnalyzer.isThreadRunning())
        fftAnalyzer.startThread(juce::Thread::Priority::low);
}

void CoreLateAudioProcessor::releaseResources()
{
    crossoverManager.releaseResources();
    fftAnalyzer.stopThread(1000);
    for (auto& ca : correlationAnalyzers) ca.reset();
}

bool CoreLateAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Must be stereo in & out
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

//==============================================================================
void CoreLateAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Feed raw stereo signal to FFT FIFO (mono mix)
    fifoToEditor->pushBuffer(buffer);

    // Split into frequency bands
    crossoverManager.process(buffer);

    // Measure per-band correlation
    correlationAnalyzers[0].process(crossoverManager.getLowBandBuffer());
    correlationAnalyzers[1].process(crossoverManager.getMidBandBuffer());
    correlationAnalyzers[2].process(crossoverManager.getHighBandBuffer());

    // CoreLate is an analyzer-only plugin — pass audio through unmodified.
    // (buffer is already the host's output buffer; no modification needed.)
}

//==============================================================================
bool CoreLateAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* CoreLateAudioProcessor::createEditor()
{
    return new AudioEditor(*this);
}

//==============================================================================
void CoreLateAudioProcessor::getStateInformation(juce::MemoryBlock& /*destData*/) {}
void CoreLateAudioProcessor::setStateInformation(const void* /*data*/, int /*sizeInBytes*/) {}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CoreLateAudioProcessor();
}
