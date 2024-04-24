/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin editor.
 
 ==============================================================================
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"

class jCrescendoProcessorEditorListener : public juce::ChangeListener
{
    jCrescendoAudioProcessorEditor *ed;
    juce::ChangeBroadcaster        *bcast;
    
public:
    jCrescendoProcessorEditorListener(jCrescendoAudioProcessor &pproc, jCrescendoAudioProcessorEditor *ped)
    : ed(ped), bcast(&pproc.notifier)
    {
        bcast->addChangeListener(this);
    }
    
    ~jCrescendoProcessorEditorListener()
    {
        bcast->removeChangeListener(this);
    }
    
    void changeListenerCallback (juce::ChangeBroadcaster* source)
    {
        if(source == bcast)
            ed->update();
    }
};

// ----------------------------------------------------------------

class AudSlider : public juce::Slider
{
    int m_grp;
    int m_ix;
    jCrescendoAudioProcessor &audioProcessor;
    
public:
    AudSlider(jCrescendoAudioProcessor &p, int grp, int ix)
    : juce::Slider(juce::Slider::LinearVertical, juce::Slider::TextBoxBelow),
    m_grp(grp),
    m_ix(ix),
    audioProcessor(p)
    {
        auto param = p.get_aud_ref(grp, ix);
        auto rng   = param->getNormalisableRange();
        setRange(rng.start, rng.end, rng.interval);
    
        switch(grp) {
            case DHL:
            case DHR:
                setSliderStyle(juce::Slider::IncDecButtons);
                break;
        }

        onValueChange = [&](){
            audioProcessor.set_aud_parm(m_grp, m_ix, getValue()); };
    }
    
    void update() {
            setValue(audioProcessor.get_aud_parm(m_grp, m_ix), juce::NotificationType::dontSendNotification);
    }
};

void jCrescendoAudioProcessorEditor::install_horiz_slider(juce::Slider &widget, juce::AudioParameterFloat *parm)
{
    auto rng = parm->getNormalisableRange();
    widget.setRange(rng.start, rng.end, rng.interval);
    addAndMakeVisible (widget);
}

//==============================================================================

jCrescendoAudioProcessorEditor::jCrescendoAudioProcessorEditor (jCrescendoAudioProcessor& p)
: AudioProcessorEditor (&p), audioProcessor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    bypassCheckbox.onStateChange = [&](){
        audioProcessor.set_bypass(bypassCheckbox.getToggleState());
    };
    addAndMakeVisible (bypassCheckbox);
    
#define HWD(widget, var) \
    install_horiz_slider(widget, p.var); \
    widget.onValueChange = [&](){ audioProcessor.set_##var(widget.getValue()); };

    HWD(dBSPLCalSlider,    cal_dBSPL);
    HWD(dBFSCalSlider,     cal_dBFS);
    HWD(brightenSlider,    brighten);
    HWD(releaseFastSlider, releaseFast);
    HWD(releaseSlowSlider, releaseSlow);
#undef HWD

    for(int grp = 0; grp < 4; ++grp)
        for(int ix = 0; ix < 10; ++ix) {
            juce::Slider *slider = new AudSlider(p, grp, ix);
            addAndMakeVisible(slider);
            audiologySlider[grp][ix] = slider;
        }

    listener = new jCrescendoProcessorEditorListener(p, this);
    update();
    setSize (420, 590);
}

jCrescendoAudioProcessorEditor::~jCrescendoAudioProcessorEditor()
{
    for(int grp = 0; grp < 4; ++grp)
        for(int ix = 0; ix < 10; ++ix)
            delete audiologySlider[grp][ix];
}

//==============================================================================
// C-Macros are so friggin primitive...

#define VIEW_BORDER_WIDTH        10
#define view_border_trimmed()   reduced(VIEW_BORDER_WIDTH)
#define VIEW_BOUNDS             getLocalBounds().view_border_trimmed()

#define WIDGET_BORDER_WIDTH       5
#define widget_border_trimmed() reduced(WIDGET_BORDER_WIDTH)

#define VSLIDER_WIDTH            40
#define VSLIDER_HEIGHT          140
#define VSLIDER_BOUNDS          row.removeFromLeft(VSLIDER_WIDTH).widget_border_trimmed()

#define HSLIDER_WIDTH           200
#define HSLIDER_HEIGHT           40
#define HSLIDER_BOUNDS          row.removeFromLeft(HSLIDER_WIDTH).widget_border_trimmed()

#define TEXTBOX_HEIGHT           50

#define BYPASS_CHECKBOX_WIDTH    80
#define BYPASS_CHECKBOX_BOUNDS  row.removeFromRight(BYPASS_CHECKBOX_WIDTH).widget_border_trimmed()

#define LABEL_JUSTIFICATION     juce::Justification::centred

#define DH_LEGEND_WIDTH          50
#define DH_LEGEND_BOUNDS        row.removeFromLeft(DH_LEGEND_WIDTH).widget_border_trimmed()

juce::Rectangle<int> topRemoved(juce::Rectangle<int> view, int height) {
    view.removeFromTop(height);
    return view;
}

juce::Rectangle<int> get_row(juce::Rectangle<int> view, int vpos, int height) {
    return topRemoved(view, vpos).removeFromTop(height);
}

// -----------------------------------------------------------------

void jCrescendoAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    
    auto view = VIEW_BOUNDS;
    
#define TTL0(text, bounds, just) \
    g.drawFittedText(text, bounds, just, 1);
    
    auto row = get_row(view, 0, 1);
    TTL0("-- Crescendo --",  row, juce::Justification::centred);
    TTL0("[240410.0952]",    row, juce::Justification::right);
    
#define TTL1(text, bounds) \
    TTL0(text, bounds, LABEL_JUSTIFICATION);
    
#define TTL(text) \
    TTL1(text, VSLIDER_BOUNDS);
    
    row = get_row(view, 0, HSLIDER_HEIGHT);
    TTL("250");
    TTL("500");
    TTL("750");
    TTL("1000");
    TTL("1500");
    TTL("2000");
    TTL("3000");
    TTL("4000");
    TTL("6000");
    TTL("8000");
#undef TTL
    
#define TTL(text) \
    TTL1(text, HSLIDER_BOUNDS);
    
    row = get_row(view, 395, HSLIDER_HEIGHT);
    TTL("dBSPL Cal");
    TTL("dBFS Cal");
    
    row = get_row(view, 450, HSLIDER_HEIGHT);
    TTL("Release Fast");
    TTL("Release Slow");
    
    row = get_row(view, 510, HSLIDER_HEIGHT);
    TTL("Brighten");
#undef TTL
    
#define TTL(text) \
    TTL1(text, DH_LEGEND_BOUNDS);
    
    row = get_row(view, 190, HSLIDER_HEIGHT);
    TTL("(+D/-H)");
#undef TTL
#undef TTL1
#undef TTL0
}

void jCrescendoAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto view = VIEW_BOUNDS;
    
    auto row = get_row(view, 20, VSLIDER_HEIGHT);
    for(int ix = 0; ix < 10; ++ix)
        audiologySlider[AUDL][ix]->setBounds(VSLIDER_BOUNDS);
    
    row = get_row(view, 150, TEXTBOX_HEIGHT);
    for(int ix = 0; ix < 10; ++ix)
        audiologySlider[DHL][ix]->setBounds(VSLIDER_BOUNDS);
    
    row = get_row(view, 220, VSLIDER_HEIGHT);
    for(int ix = 0; ix < 10; ++ix)
        audiologySlider[AUDR][ix]->setBounds(VSLIDER_BOUNDS);
    
    row = get_row(view, 350, TEXTBOX_HEIGHT);
    for(int ix = 0; ix < 10; ++ix)
        audiologySlider[DHR][ix]->setBounds(VSLIDER_BOUNDS);
    
#define HBND(widget) \
    widget.setBounds(HSLIDER_BOUNDS);
    
    row = get_row(view, 420, HSLIDER_HEIGHT);
    HBND(dBSPLCalSlider);
    HBND(dBFSCalSlider);
    
    row = get_row(view, 480, HSLIDER_HEIGHT);
    HBND(releaseFastSlider);
    HBND(releaseSlowSlider);
    
    row = get_row(view, 540, HSLIDER_HEIGHT);
    HBND(brightenSlider);
#undef HBND
    
    row = get_row(view, 195, HSLIDER_HEIGHT);
    bypassCheckbox.setBounds(BYPASS_CHECKBOX_BOUNDS);
}

void jCrescendoAudioProcessorEditor::update() {
    
    bypassCheckbox.setToggleState(audioProcessor.get_bypass(), juce::NotificationType::dontSendNotification);

#define UPD(widget, var) \
    widget.setValue(audioProcessor.get_##var(), juce::NotificationType::dontSendNotification);
    
    UPD(dBSPLCalSlider,    cal_dBSPL);
    UPD(dBFSCalSlider,     cal_dBFS);
    UPD(brightenSlider,    brighten);
    UPD(releaseFastSlider, releaseFast);
    UPD(releaseSlowSlider, releaseSlow);
#undef UPD

    for(int grp = 0; grp < 4; ++grp)
        for(int ix = 0; ix < 10; ++ix)
            ((AudSlider*)(audiologySlider[grp][ix]))->update();
}
