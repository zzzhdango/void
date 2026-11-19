#include "PluginProcessor.h"
#include "PluginEditor.h"

ExperimentalSpaceEchoAudioProcessorEditor::ExperimentalSpaceEchoAudioProcessorEditor (ExperimentalSpaceEchoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (840, 460);

    // Left Module: Reverb
    setupRotary (revMixSlider,      revMixLabel,      "MIX");
    setupRotary (revSizeSlider,     revSizeLabel,     "SIZE / DECAY");
    setupRotary (revDistanceSlider, revDistanceLabel, "DISTANCE");
    setupRotary (revWidthSlider,    revWidthLabel,    "WIDTH");
    setupRotary (revPreDelaySlider, revPreDelayLabel, "PRE-DELAY");

    revAlgoBox.addItemList ({ "Room", "Chamber", "Plate", "Super Cosmic" }, 1);
    addAndMakeVisible (revAlgoBox);

    revMixAtt      = std::make_unique<Attachment> (audioProcessor.apvts, "revMix", revMixSlider);
    revSizeAtt     = std::make_unique<Attachment> (audioProcessor.apvts, "revSize", revSizeSlider);
    revDistanceAtt = std::make_unique<Attachment> (audioProcessor.apvts, "revDistance", revDistanceSlider);
    revWidthAtt    = std::make_unique<Attachment> (audioProcessor.apvts, "revWidth", revWidthSlider);
    revPreDelayAtt = std::make_unique<Attachment> (audioProcessor.apvts, "revPreDelay", revPreDelaySlider);
    revAlgoAtt     = std::make_unique<BoxAttachment> (audioProcessor.apvts, "revAlgo", revAlgoBox);

    // Right Module: Delay
    setupRotary (delTimeSlider,     delTimeLabel,     "TIME");
    setupRotary (delFeedbackSlider, delFeedbackLabel, "FEEDBACK");
    setupRotary (delMixSlider,      delMixLabel,      "MIX");

    delAlgoBox.addItemList ({ "Echoplex Tape", "Space Warp", "Quantum Reverse", "Glitch Granular" }, 1);
    addAndMakeVisible (delAlgoBox);

    delTimeAtt     = std::make_unique<Attachment> (audioProcessor.apvts, "delTime", delTimeSlider);
    delFeedbackAtt = std::make_unique<Attachment> (audioProcessor.apvts, "delFeedback", delFeedbackSlider);
    delMixAtt      = std::make_unique<Attachment> (audioProcessor.apvts, "delMix", delMixSlider);
    delAlgoAtt     = std::make_unique<BoxAttachment> (audioProcessor.apvts, "delAlgo", delAlgoBox);

    // Global Controls
    setupRotary (masterMixSlider, masterMixLabel, "MASTER MIX");
    masterMixAtt = std::make_unique<Attachment> (audioProcessor.apvts, "masterMix", masterMixSlider);

    randomizeButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (214, 69, 80));
    randomizeButton.onClick = [this]() { audioProcessor.randomizeParameters(); };
    addAndMakeVisible (randomizeButton);
}

ExperimentalSpaceEchoAudioProcessorEditor::~ExperimentalSpaceEchoAudioProcessorEditor() {}

void ExperimentalSpaceEchoAudioProcessorEditor::setupRotary (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB (92, 225, 230));
    slider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (label);
}

void ExperimentalSpaceEchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (18, 20, 26));

    // Left Panel: Reverb
    g.setColour (juce::Colour::fromRGB (28, 32, 42));
    g.fillRoundedRectangle (16, 56, 440, 388, 12.0f);
    g.setColour (juce::Colour::fromRGB (92, 225, 230));
    g.drawRoundedRectangle (16, 56, 440, 388, 12.0f, 1.5f);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText ("REVERB // SPACE ENGINE", 28, 66, 260, 24, juce::Justification::left);

    // Right Panel: Delay
    g.setColour (juce::Colour::fromRGB (28, 32, 42));
    g.fillRoundedRectangle (472, 56, 352, 250, 12.0f);
    g.setColour (juce::Colour::fromRGB (189, 147, 249));
    g.drawRoundedRectangle (472, 56, 352, 250, 12.0f, 1.5f);
    g.drawText ("DELAY // EXPERIMENTAL ECHO", 484, 66, 280, 24, juce::Justification::left);

    // Global Panel
    g.setColour (juce::Colour::fromRGB (34, 38, 50));
    g.fillRoundedRectangle (472, 318, 352, 126, 12.0f);
    g.setColour (juce::Colours::white);
    g.drawText ("EXPERIMENTAL SPACE ECHO v1.0", 20, 16, 400, 28, juce::Justification::left);
}

void ExperimentalSpaceEchoAudioProcessorEditor::resized()
{
    // Reverb Algo Selector
    revAlgoBox.setBounds (290, 66, 150, 26);

    // Reverb Knobs (Row 1: Mix, Size, Pre-Delay | Row 2: Distance, Width)
    int kw = 110, kh = 110;
    revMixLabel.setBounds      (35,  108, kw, 18); revMixSlider.setBounds      (35,  126, kw, kh);
    revSizeLabel.setBounds     (180, 108, kw, 18); revSizeSlider.setBounds     (180, 126, kw, kh);
    revPreDelayLabel.setBounds (325, 108, kw, 18); revPreDelaySlider.setBounds (325, 126, kw, kh);

    revDistanceLabel.setBounds (105, 265, kw, 18); revDistanceSlider.setBounds (105, 283, kw, kh);
    revWidthLabel.setBounds    (255, 265, kw, 18); revWidthSlider.setBounds    (255, 283, kw, kh);

    // Delay Algo Selector & Knobs
    delAlgoBox.setBounds (655, 66, 155, 26);
    delTimeLabel.setBounds     (485, 125, 100, 18); delTimeSlider.setBounds     (485, 143, 100, 105);
    delFeedbackLabel.setBounds (598, 125, 100, 18); delFeedbackSlider.setBounds (598, 143, 100, 105);
    delMixLabel.setBounds      (710, 125, 100, 18); delMixSlider.setBounds      (710, 143, 100, 105);

    // Global Master Mix + Randomizer
    masterMixLabel.setBounds (495, 324, 100, 18);
    masterMixSlider.setBounds (495, 342, 100, 96);
    randomizeButton.setBounds (625, 355, 175, 52);
}
