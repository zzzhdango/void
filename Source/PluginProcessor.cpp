#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <random>

ExperimentalSpaceEchoAudioProcessor::ExperimentalSpaceEchoAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

ExperimentalSpaceEchoAudioProcessor::~ExperimentalSpaceEchoAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout ExperimentalSpaceEchoAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // LEFT MODULE: REVERB
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("revMix", "Reverb Mix", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("revSize", "Room Size / Decay", 0.0f, 1.0f, 0.65f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("revDistance", "Distance (Near/Far)", 0.0f, 1.0f, 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("revWidth", "Stereo Width", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("revPreDelay", "Pre-Delay (ms)", 0.0f, 500.0f, 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("revAlgo", "Reverb Algorithm",
        juce::StringArray { "Room", "Chamber", "Plate", "Super Cosmic" }, 3));

    // RIGHT MODULE: DELAY
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("delTime", "Delay Time (ms)", 10.0f, 2000.0f, 420.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("delFeedback", "Delay Feedback", 0.0f, 1.15f, 0.55f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("delMix", "Delay Mix", 0.0f, 1.0f, 0.45f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("delAlgo", "Delay Algorithm",
        juce::StringArray { "Echoplex Tape", "Space Warp", "Quantum Reverse", "Glitch Granular" }, 0));

    // GLOBAL CONTROLS
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("masterMix", "Master Mix", 0.0f, 1.0f, 0.75f));

    return { params.begin(), params.end() };
}

void ExperimentalSpaceEchoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 2;

    reverb.setSampleRate (sampleRate);
    reverb.reset();

    preDelayLine.prepare (spec);
    preDelayLine.setMaximumDelayInSamples (static_cast<int> (sampleRate * 1.0));
    preDelayLine.reset();

    distanceLPF.prepare (spec);
    distanceLPF.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    distanceLPF.reset();

    cosmicShimmerFilter.prepare (spec);
    cosmicShimmerFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
    cosmicShimmerFilter.reset();

    delayBufferL.assign (maxDelaySamples, 0.0f);
    delayBufferR.assign (maxDelaySamples, 0.0f);
    delayWritePos = 0;
    lpfStateL = lpfStateR = hpfStateL = hpfStateR = 0.0f;
}

void ExperimentalSpaceEchoAudioProcessor::releaseResources() {}

bool ExperimentalSpaceEchoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ExperimentalSpaceEchoAudioProcessor::randomizeParameters()
{
    std::random_device rd;
    std::mt19937 gen (rd());
    std::uniform_real_distribution<float> dist01 (0.0f, 1.0f);

    for (auto* param : getParameters())
    {
        if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            // Exclude Master Mix from randomization
            if (rangedParam->getParameterID() != "masterMix")
            {
                rangedParam->setValueNotifyingHost (dist01 (gen));
            }
        }
    }
}

void ExperimentalSpaceEchoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();

    // Fetch parameters
    float revMix      = apvts.getRawParameterValue ("revMix")->load();
    float revSize     = apvts.getRawParameterValue ("revSize")->load();
    float revDistance = apvts.getRawParameterValue ("revDistance")->load();
    float revWidth    = apvts.getRawParameterValue ("revWidth")->load();
    float revPreDelay = apvts.getRawParameterValue ("revPreDelay")->load();
    int   revAlgo     = static_cast<int> (apvts.getRawParameterValue ("revAlgo")->load());

    float delTimeMs   = apvts.getRawParameterValue ("delTime")->load();
    float delFeedback = apvts.getRawParameterValue ("delFeedback")->load();
    float delMix      = apvts.getRawParameterValue ("delMix")->load();
    int   delAlgo     = static_cast<int> (apvts.getRawParameterValue ("delAlgo")->load());

    float masterMix   = apvts.getRawParameterValue ("masterMix")->load();

    // Keep dry copy for Master Mix
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf (buffer);

    // =========================================================================
    // 1. LEFT MODULE: REVERB
    // =========================================================================
    juce::AudioBuffer<float> revBuffer;
    revBuffer.makeCopyOf (buffer);

    // Pre-delay processing
    float preDelaySamples = (revPreDelay / 1000.0f) * static_cast<float> (currentSampleRate);
    preDelayLine.setDelay (preDelaySamples);

    for (int ch = 0; ch < 2; ++ch)
    {
        auto* data = revBuffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float in = data[i];
            preDelayLine.pushSample (ch, in);
            data[i] = preDelayLine.popSample (ch);
        }
    }

    // Configure Reverb Algorithm
    reverbParams.dryLevel = 0.0f;
    reverbParams.wetLevel = 1.0f;
    reverbParams.width    = 1.0f; // Handled manually via Mid/Side for 0..200% range

    switch (revAlgo)
    {
        case 0: // Room
            reverbParams.roomSize = 0.25f + revSize * 0.45f;
            reverbParams.damping  = 0.65f;
            break;
        case 1: // Chamber
            reverbParams.roomSize = 0.45f + revSize * 0.45f;
            reverbParams.damping  = 0.45f;
            break;
        case 2: // Plate
            reverbParams.roomSize = 0.60f + revSize * 0.35f;
            reverbParams.damping  = 0.20f;
            break;
        case 3: // Super Cosmic (Infinite-ish tail + pitch shimmer modulation)
        default:
            reverbParams.roomSize = 0.82f + revSize * 0.175f;
            reverbParams.damping  = 0.10f;
            break;
    }

    reverb.setParameters (reverbParams);
    reverb.processStereo (revBuffer.getWritePointer (0), revBuffer.getWritePointer (1), numSamples);

    // Super Cosmic harmonic shimmer + Distance Air Absorption + M/S Stereo Width
    float cutoffFreq = 20000.0f * std::pow (0.06f, revDistance); // 20kHz down to 1.2kHz as distance increases
    distanceLPF.setCutoffFrequency (cutoffFreq);

    auto* revL = revBuffer.getWritePointer (0);
    auto* revR = revBuffer.getWritePointer (1);
    auto* mainL = buffer.getWritePointer (0);
    auto* mainR = buffer.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        float l = revL[i];
        float r = revR[i];

        if (revAlgo == 3) // Super Cosmic octave/fifth harmonic ring shimmer
        {
            shimmerPhase += 0.015f;
            if (shimmerPhase > juce::MathConstants<float>::twoPi)
                shimmerPhase -= juce::MathConstants<float>::twoPi;
            float mod = 0.5f + 0.5f * std::sin (shimmerPhase);
            float octaveHarmonic = std::tanh ((l + r) * 2.5f) * 0.25f * mod;
            l += octaveHarmonic;
            r -= octaveHarmonic;
        }

        // Distance filtering (simulates air absorption + direct-to-reverberant ratio)
        l = distanceLPF.processSample (0, l);
        r = distanceLPF.processSample (1, r);

        // Mid/Side Stereo Width (0 = Mono, 1 = Normal, 2 = Super Wide)
        float mid  = 0.5f * (l + r);
        float side = 0.5f * (l - r) * revWidth;
        float outL = mid + side;
        float outR = mid - side;

        // Distance pushes dry signal back slightly while elevating wet tail
        float distanceDirectAtten = 1.0f - (revDistance * 0.45f * revMix);
        mainL[i] = mainL[i] * distanceDirectAtten + outL * revMix;
        mainR[i] = mainR[i] * distanceDirectAtten + outR * revMix;
    }

    // =========================================================================
    // 2. RIGHT MODULE: EXPERIMENTAL DELAY
    // =========================================================================
    float baseDelaySamples = (delTimeMs / 1000.0f) * static_cast<float> (currentSampleRate);
    baseDelaySamples = juce::jlimit (10.0f, static_cast<float> (maxDelaySamples - 4), baseDelaySamples);

    for (int i = 0; i < numSamples; ++i)
    {
        float inL = mainL[i];
        float inR = mainR[i];

        wowFlutterPhase += 0.00035f;
        if (wowFlutterPhase > juce::MathConstants<float>::twoPi)
            wowFlutterPhase -= juce::MathConstants<float>::twoPi;

        granulatorPhase += 1.0f / (baseDelaySamples + 1.0f);
        if (granulatorPhase >= 1.0f)
            granulatorPhase -= 1.0f;

        float modDelayL = baseDelaySamples;
        float modDelayR = baseDelaySamples;

        switch (delAlgo)
        {
            case 0: // Echoplex Tape (warm tape wow/flutter + slight stereo offset)
            {
                float wow = std::sin (wowFlutterPhase) * 14.0f + std::sin (wowFlutterPhase * 7.3f) * 3.5f;
                modDelayL += wow;
                modDelayR += wow * 1.15f + 22.0f;
                break;
            }
            case 1: // Space Warp (elastic pitch-bending LFO sweep + ping-pong)
            {
                float warp = std::sin (wowFlutterPhase * 2.5f) * (baseDelaySamples * 0.18f);
                modDelayL += warp;
                modDelayR = baseDelaySamples * 0.75f - warp;
                break;
            }
            case 2: // Quantum Reverse (sawtooth-scanned reverse grain buffer)
            {
                float revScan = (1.0f - granulatorPhase) * baseDelaySamples;
                modDelayL = juce::jlimit (10.0f, static_cast<float> (maxDelaySamples - 4), revScan);
                modDelayR = juce::jlimit (10.0f, static_cast<float> (maxDelaySamples - 4), revScan * 0.85f);
                break;
            }
            case 3: // Glitch Granular (stepped sample-and-hold delay time jumps)
            {
                float step = std::floor (granulatorPhase * 4.0f) / 4.0f;
                modDelayL = baseDelaySamples * (0.25f + step * 0.75f);
                modDelayR = baseDelaySamples * (1.0f - step * 0.5f);
                break;
            }
        }

        auto readInterp = [&](const std::vector<float>& buf, float delaySamps) -> float
        {
            float readPos = static_cast<float> (delayWritePos) - delaySamps;
            while (readPos < 0.0f) readPos += static_cast<float> (maxDelaySamples);
            int idx0 = static_cast<int> (readPos) % maxDelaySamples;
            int idx1 = (idx0 + 1) % maxDelaySamples;
            float frac = readPos - std::floor (readPos);
            return buf[idx0] * (1.0f - frac) + buf[idx1] * frac;
        };

        float tapL = readInterp (delayBufferL, modDelayL);
        float tapR = readInterp (delayBufferR, modDelayR);

        // Tape saturation & filtering in feedback path
        lpfStateL += 0.35f * (tapL - lpfStateL);
        lpfStateR += 0.35f * (tapR - lpfStateR);
        float fbSignalL = std::tanh (lpfStateL * 1.3f);
        float fbSignalR = std::tanh (lpfStateR * 1.3f);

        // Cross-feedback for Space Warp
        if (delAlgo == 1)
        {
            delayBufferL[delayWritePos] = inL + fbSignalR * delFeedback;
            delayBufferR[delayWritePos] = inR + fbSignalL * delFeedback;
        }
        else
        {
            delayBufferL[delayWritePos] = inL + fbSignalL * delFeedback;
            delayBufferR[delayWritePos] = inR + fbSignalR * delFeedback;
        }

        delayWritePos = (delayWritePos + 1) % maxDelaySamples;

        float processedL = inL + tapL * delMix;
        float processedR = inR + tapR * delMix;

        // =====================================================================
        // 3. GLOBAL MASTER MIX
        // =====================================================================
        float dryL = dryBuffer.getSample (0, i);
        float dryR = dryBuffer.getSample (1, i);

        mainL[i] = dryL * (1.0f - masterMix) + processedL * masterMix;
        mainR[i] = dryR * (1.0f - masterMix) + processedR * masterMix;
    }
}

bool ExperimentalSpaceEchoAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* ExperimentalSpaceEchoAudioProcessor::createEditor()
{
    return new ExperimentalSpaceEchoAudioProcessorEditor (*this);
}

const juce::String ExperimentalSpaceEchoAudioProcessor::getName() const { return JucePlugin_Name; }
bool ExperimentalSpaceEchoAudioProcessor::acceptsMidi() const { return false; }
bool ExperimentalSpaceEchoAudioProcessor::producesMidi() const { return false; }
bool ExperimentalSpaceEchoAudioProcessor::isMidiEffect() const { return false; }
double ExperimentalSpaceEchoAudioProcessor::getTailLengthSeconds() const { return 5.0; }
int ExperimentalSpaceEchoAudioProcessor::getNumPrograms() { return 1; }
int ExperimentalSpaceEchoAudioProcessor::getCurrentProgram() { return 0; }
void ExperimentalSpaceEchoAudioProcessor::setCurrentProgram (int) {}
const juce::String ExperimentalSpaceEchoAudioProcessor::getProgramName (int) { return {}; }
void ExperimentalSpaceEchoAudioProcessor::changeProgramName (int, const juce::String&) {}

void ExperimentalSpaceEchoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ExperimentalSpaceEchoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ExperimentalSpaceEchoAudioProcessor();
}
