/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#pragma once

#include <JuceHeader.h>
#include "vTuningParams.h"
#include "crescendo.h"
#include "smart_ptr.h"

//==============================================================================

class jCrescendoAudioProcessor;

class jCLiveInfoUser
{
    jCrescendoAudioProcessor* m_pproc;
public:
    jCLiveInfoUser(jCrescendoAudioProcessor* pproc)
    : m_pproc(pproc)
    {}
    virtual ~jCLiveInfoUser()
    {}
    
    void grab_info(void *dst);
};

class jCTelemeter : public juce::Thread,
                    public jCLiveInfoUser
{
    juce::StreamingSocket*    m_sock;
    jCTelemeter*              m_next;
    
public:
    jCTelemeter(class jCrescendoAudioProcessor* pproc, juce::StreamingSocket *cnx, jCTelemeter *next);
    virtual ~jCTelemeter();
    
    void run();
    
    jCTelemeter* get_next() {
        return m_next;
    }
};

// -----------------------------------------------

class jCTelemetry : public juce::Thread
{
    juce::StreamingSocket      m_sock;
    jCrescendoAudioProcessor*  m_pproc;
    jCTelemeter*               m_telemeters;
 
public:
    jCTelemetry(int cresc_id, class jCrescendoAudioProcessor* pproc);
    virtual ~jCTelemetry();
    
    void run();
};

// -----------------------------------------------

class jCUDPTelemeter :  public juce::Thread,
                        public jCLiveInfoUser
{
    juce::DatagramSocket      m_sock;

public:
    jCUDPTelemeter(int cresc_id, class jCrescendoAudioProcessor* pproc);
    virtual ~jCUDPTelemeter();
    
    void run();
};

// -----------------------------------------------

class jCrescendoAudioProcessor  : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
, public juce::AudioProcessorARAExtension
#endif
{
public:
    //==============================================================================
    jCrescendoAudioProcessor();
    ~jCrescendoAudioProcessor() override;
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif
    
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    //==============================================================================
    const juce::String getName() const override;
    
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    
    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    
public:
    juce::ChangeBroadcaster notifier;
#if 1
    TPtr<jCTelemetry>       m_telemetry;
#endif
#if 1
    TPtr<jCUDPTelemeter>    m_udp_telemeter;
#endif
    
    juce::AudioParameterBool *bypass;
    inline bool get_bypass() {
        return *bypass;
    }
    inline void set_bypass(bool newVal) {
        *bypass = newVal;
        pcresc->set_Processing(!newVal);
    }
    
#define RWPARMF(name, fn)                  \
    juce::AudioParameterFloat *name;       \
    inline float get_##name() {            \
        return *name;                      \
    };                                     \
    inline void set_##name(float newVal) { \
        *name       = newVal;              \
        fn();                              \
    };
    
    RWPARMF(cal_dBSPL,   set_cal);
    RWPARMF(cal_dBFS,    set_cal);
    RWPARMF(brighten,    adj_brightness);
    RWPARMF(releaseFast, set_releases);
    RWPARMF(releaseSlow, set_releases);
#undef RWPARMF
    
#define RWVAR(type, name)                  \
    type name;                             \
    inline type get_##name() {            \
        return name;                       \
    };                                     \
    inline void set_##name(type newVal) {  \
        name = newVal;                     \
    };
    
    RWVAR(int,      id);
    RWVAR(Float32*, liveInfo);
#undef RWVAR
    juce::ReadWriteLock busyLock;
    
    juce::AudioParameterFloat* aud[4][10];

    inline juce::AudioParameterFloat* get_aud_ref(int grp_ix, int parm_ix) {
        return aud[grp_ix][parm_ix];
    }
    inline float get_aud_parm(int grp_ix, int parm_ix) {
        return *aud[grp_ix][parm_ix];
    }
    inline void set_aud_parm(int grp_ix, int parm_ix, float newVal) {
        *aud[grp_ix][parm_ix]       = newVal;
        params.aud[grp_ix][parm_ix] = newVal;
        set_audiology();
    }
    
private:
    TPtr<TCrescendo> pcresc;
    tVTuningParams   params;
    
    // Helper to avoid the textual diarreah of C/C++ syntax.
    // If a C macro can't do it, maybe the templating facility of C++ will work...
    //
    // -- NOTE: Use JUCE_FORCE_USE_LEGACY_PARAM_IDS=1 to avoid jumbled presentations in default parameter
    //          editors. !!!
    inline juce::AudioParameterBool *install_bool_parm(const char* id, const char* title, bool defv) {
        auto thing = new juce::AudioParameterBool(juce::ParameterID(id, 1), title, defv);
        addParameter(thing);
        return thing;
    }
    
    inline juce::AudioParameterFloat *install_float_parm(const char* id, const char* title, juce::NormalisableRange<float> range, float defv) {
        auto thing = new juce::AudioParameterFloat(juce::ParameterID(id, 1), title, range, defv);
        addParameter(thing);
        return thing;
    }

    void install_audiology(int grp, const char* id, const char* title, juce::NormalisableRange<float> range);
    
    bool ignore_changes = false;
    
    void set_cal() {
        pcresc->set_CaldBSPL(*cal_dBSPL);
        pcresc->set_CalLUFS(*cal_dBFS);
    }
    
    void adj_brightness();
    
    void set_releases() {
        if(!ignore_changes)
            pcresc->set_releases(*releaseFast, *releaseSlow);
    }
    
    void set_audiology() {
        if(!ignore_changes)
            pcresc->set_audiology(&params);
    }
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (jCrescendoAudioProcessor)
};


