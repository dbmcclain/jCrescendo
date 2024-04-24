/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "vTuningParams.h"

//==============================================================================
/**
*/

// -----------------------------------------------------------------------------

class jCrescendoAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    jCrescendoAudioProcessorEditor (jCrescendoAudioProcessor&);
    ~jCrescendoAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    jCrescendoAudioProcessor& audioProcessor;
    
    juce::ToggleButton bypassCheckbox{ "Bypass"};
    
#define HSLD(name) \
    juce::Slider name  { juce::Slider::LinearHorizontal, juce::Slider::TextBoxLeft };

    HSLD(dBSPLCalSlider);
    HSLD(dBFSCalSlider);
    HSLD(brightenSlider);
    HSLD(releaseFastSlider);
    HSLD(releaseSlowSlider);
#undef HSLD
    
    juce::Slider* audiologySlider[4][10];
    
    
    TPtr<juce::ChangeListener> listener;
    
    void update();
    
    void install_horiz_slider(juce::Slider &widget, juce::AudioParameterFloat *parm);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (jCrescendoAudioProcessorEditor)
};
