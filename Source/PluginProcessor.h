#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class ExperimentalSpaceEchoAudioProcessor : public juce::AudioProcessor
{
public:
    ExperimentalSpaceEchoAudioProcessor();
    ~ExperimentalSpaceEchoAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void randomizeParameters();

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Reverb DSP components
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayLine { 96000 };
    juce::dsp::StateVariableTPTFilter<float> distanceLPF;
    juce::dsp::StateVariableTPTFilter<float> cosmicShimmerFilter;
    float shimmerPhase = 0.0f;

    // Experimental Delay DSP components
    static constexpr int maxDelaySamples = 192000 * 2;
    std::vector<float> delayBufferL, delayBufferR;
    int delayWritePos = 0;
    float wowFlutterPhase = 0.0f;
    float lpfStateL = 0.0f, lpfStateR = 0.0f;
    float hpfStateL = 0.0f, hpfStateR = 0.0f;
    float granulatorPhase = 0.0f;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExperimentalSpaceEchoAudioProcessor)
};
