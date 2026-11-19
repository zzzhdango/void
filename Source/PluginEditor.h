#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ExperimentalSpaceEchoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    ExperimentalSpaceEchoAudioProcessorEditor (ExperimentalSpaceEchoAudioProcessor&);
    ~ExperimentalSpaceEchoAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ExperimentalSpaceEchoAudioProcessor& audioProcessor;

    // Reverb controls
    juce::Slider revMixSlider, revSizeSlider, revDistanceSlider, revWidthSlider, revPreDelaySlider;
    juce::Label  revMixLabel,  revSizeLabel,  revDistanceLabel,  revWidthLabel,  revPreDelayLabel;
    juce::ComboBox revAlgoBox;

    // Delay controls
    juce::Slider delTimeSlider, delFeedbackSlider, delMixSlider;
    juce::Label  delTimeLabel,  delFeedbackLabel,  delMixLabel;
    juce::ComboBox delAlgoBox;

    // Master & Randomizer
    juce::Slider masterMixSlider;
    juce::Label  masterMixLabel;
    juce::TextButton randomizeButton { "RANDOMIZE ALL" };

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<Attachment> revMixAtt, revSizeAtt, revDistanceAtt, revWidthAtt, revPreDelayAtt;
    std::unique_ptr<BoxAttachment> revAlgoAtt;

    std::unique_ptr<Attachment> delTimeAtt, delFeedbackAtt, delMixAtt;
    std::unique_ptr<BoxAttachment> delAlgoAtt;

    std::unique_ptr<Attachment> masterMixAtt;

    void setupRotary (juce::Slider& slider, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExperimentalSpaceEchoAudioProcessorEditor)
};
