/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <syslog.h>

//==============================================================================
#define GET_TIMING 1

static int id_count = 0;

jCTelemetry::jCTelemetry(int cresc_id, class jCrescendoAudioProcessor* pproc)
: Thread(juce::String("Crescendo Telemetry Server"))
{
    m_pproc      = pproc;
    m_telemeters = 0;
    
    if(m_sock.createListener(65200 + cresc_id))
        startThread(/*juce::Thread::Priority::high*/);
}

jCTelemetry::~jCTelemetry() {
    if(isThreadRunning()) {
        m_sock.close();
        stopThread(10);
        jCTelemeter* p = m_telemeters;
        while(p)
        {
            jCTelemeter* q = p->get_next();
            delete p;
            p = q;
        }
    }
}

void jCTelemetry::run() {
    while(!threadShouldExit()) {
        juce::StreamingSocket *cnx = m_sock.waitForNextConnection();
        if(0 != cnx) {
            m_telemeters = new jCTelemeter(m_pproc, cnx, m_telemeters);
        }
    }
    m_sock.close();
}

// ----------------------------------------------------------

void jCLiveInfoUser::grab_info(void *dst)
{
    const juce::ScopedReadLock myScopedLock(m_pproc->busyLock);
    memcpy(dst, m_pproc->get_liveInfo(), 4*12*sizeof(Float32));
}

jCTelemeter::jCTelemeter(jCrescendoAudioProcessor* pproc, juce::StreamingSocket *cnx, jCTelemeter *next)
: Thread(juce::String("Telemeter")),
  jCLiveInfoUser(pproc)
{
    m_sock  = cnx;
    m_next  = next;
    startThread(/*juce::Thread::Priority::high*/);
}

jCTelemeter::~jCTelemeter() {
    m_sock->close();
    stopThread(10);
    waitForThreadToExit(10);
    delete m_sock;
}

void jCTelemeter::run() {
    char buf[16];

    while(!threadShouldExit() && (m_sock->read(buf, 1, true) > 0)) {
        Float32 staging[4*12];
        grab_info(staging);
        // syslog(1, "Crescendo Telemetry Writing %lu bytes", sizeof(staging));
        m_sock->write(staging, sizeof(staging));
    }
    m_sock->close();
}

// ----------------------------------------------------------

jCUDPTelemeter::jCUDPTelemeter(int cresc_id, class jCrescendoAudioProcessor* pproc)
: Thread(juce::String("Crescendo UDP Telemetry Service")),
  jCLiveInfoUser(pproc)
{
    if(m_sock.bindToPort(65300 + cresc_id))
        startThread(/*juce::Thread::Priority::high*/);
}

jCUDPTelemeter::~jCUDPTelemeter() {
    if(isThreadRunning()) {
        m_sock.shutdown();
        stopThread(10);
    }
}

void jCUDPTelemeter::run() {
    char         buf[2];
    juce::String sender_ip;
    int          sender_port;
    
    while(!threadShouldExit() && (m_sock.read(buf, 1, true, sender_ip, sender_port) > 0)) {
        Float32 staging[4*12];
        grab_info(staging);
        m_sock.write(sender_ip, sender_port, staging, sizeof(staging));
    }
    m_sock.shutdown();
}

// ----------------------------------------------------------

jCrescendoAudioProcessor::jCrescendoAudioProcessor()
:
#ifndef JucePlugin_PreferredChannelConfigurations
AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                )
#endif
{
    set_id(++id_count);
    pcresc = new TCrescendo(48000.0f);
    setLatencySamples(pcresc->get_latency_samples());
    memset(&params, 0, sizeof(params));
    set_liveInfo(params.specL);
    
#if 1
    m_telemetry     = new jCTelemetry(get_id(),    this);
#endif
#if 1
    m_udp_telemeter = new jCUDPTelemeter(get_id(), this);
#endif
    
    bypass      = install_bool_parm("bypass_id", "Bypass", false);
    
    cal_dBSPL   = install_float_parm("cal_dBSPL_id",   "Cal dBSPL",    { 60.0,  85.0, 1.0},  77.0);
    cal_dBFS    = install_float_parm("cal_dBFS_id",    "Cal dBFS",     {-40.0, -10.0, 1.0}, -23.0);
    brighten    = install_float_parm("brighten_id",    "Brighten",     {  0.0,  85.0, 1.0},   0.0);
    releaseFast = install_float_parm("releaseFast_id", "Release Fast", {  0.0, 100.0, 1.0},  34.0);
    releaseSlow = install_float_parm("releaseSlow_id", "Release Slow", { 10.0, 500.0, 1.0}, 155.0);

    install_audiology(AUDL, "aud_L", "AudL", {  0.0, 90.0, 1.0});
    install_audiology(AUDR, "aud_R", "AudR", {  0.0, 90.0, 1.0});
    install_audiology(DHL,  "dh_L",  "DHL",  {-20.0, 20.0, 1.0});
    install_audiology(DHR,  "dh_R",  "DHR",  {-20.0, 20.0, 1.0});
    
    syslog(1, "Crescendo Timing: Construct Instance %d", get_id());
}

jCrescendoAudioProcessor::~jCrescendoAudioProcessor()
{ 
    syslog(1, "Crescendo Timing: Discarding Crescendo: inst %d", get_id());
}


void jCrescendoAudioProcessor::install_audiology(int grp_ix, const char* id, const char* title, juce::NormalisableRange<float> range) {
    char   widget_id[64];
    char   widget_title[64];
    const  char* frqs[10] = {"250", "500", "750", "1000", "1500", "2000", "3000", "4000", "6000", "8000"};
    
    for(int ix = 0; ix < 10; ++ix) {
        snprintf(widget_id,    sizeof(widget_id),    "%s%s_id", id,    frqs[ix]);
        snprintf(widget_title, sizeof(widget_title), "%s %s",   title, frqs[ix]);
        juce::AudioParameterFloat *thing = install_float_parm(widget_id, widget_title, range, 0.0f);
        aud[grp_ix][ix] = thing;
    }
}

//==============================================================================
const juce::String jCrescendoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool jCrescendoAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool jCrescendoAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool jCrescendoAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double jCrescendoAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int jCrescendoAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int jCrescendoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void jCrescendoAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String jCrescendoAudioProcessor::getProgramName (int index)
{
    return {};
}

void jCrescendoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================

void jCrescendoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    params.FSamp = sampleRate;
    pcresc->SetSampleRate(sampleRate);
#if GET_TIMING
    syslog(1, "--- Crescendo Timing Start --- SR = %7.0f Hz, BlkSize = %d samp  (inst %d)",
           sampleRate, samplesPerBlock, get_id());
#endif
}

void jCrescendoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool jCrescendoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void jCrescendoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto nsamp = buffer.getNumSamples();
    if(nsamp != pcresc->get_lastNSamp()) {
        pcresc->set_maxProcDur(-1);
        pcresc->set_lastNSamp(nsamp);
    }
    juce::ScopedNoDenormals noDenormals;
#if GET_TIMING
    struct timeval start;
    struct timeval stop;
    gettimeofday(&start, NULL);
#endif
    // if(m_id == id_count) 
    {
        auto totalNumInputChannels  = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();
        
        if(totalNumInputChannels > 2)
            totalNumInputChannels = 2;
        
        // In case we have more outputs than inputs, this code clears any output
        // channels that didn't contain input data, (because these aren't
        // guaranteed to be empty - they may contain garbage).
        // This is here to avoid people getting screaming feedback
        // when they first compile a plugin, but obviously you don't need to keep
        // this code if your algorithm always overwrites all the output channels.
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear (i, 0, nsamp);
        
        // This is the place where you'd normally do the guts of your plugin's
        // audio processing...
        // Make sure to reset the state if your inner loop is processing
        // the samples and the outer loop is handling the channels.
        // Alternatively, you can process the samples with the channels
        // interleaved by keeping the same state.
        
        Float32 *pinL = buffer.getWritePointer(0);
        Float32 *pinR = (totalNumInputChannels > 1) ? buffer.getWritePointer(1) : 0;
        
        {
            const juce::ScopedWriteLock myScopedLock(busyLock);
            pcresc->render(pinL, pinR, pinL, pinR, nsamp, &params);
        }
    }
#if GET_TIMING
    gettimeofday(&stop, NULL);
    timersub(&stop, &start, &stop);
    if(stop.tv_usec > pcresc->get_maxProcDur())
    {
        pcresc->set_maxProcDur(stop.tv_usec);
        Float64 usec_limit = 1.0e6 * nsamp / getSampleRate();
        syslog(1, "Crescendo Timing = %d usec, %d samp, need < %8.1f usec, using %5.1f%% (inst %d)",
               stop.tv_usec, nsamp, usec_limit, 100*stop.tv_usec/usec_limit, get_id());
    }
#endif
}

//==============================================================================
bool jCrescendoAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* jCrescendoAudioProcessor::createEditor()
{
    return new jCrescendoAudioProcessorEditor (*this);
}

//==============================================================================
void jCrescendoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    syslog(1, "Crescendo Timing: getStateInformation()  inst: %d", get_id());
    
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream stream(destData, true);

    stream.writeBool(get_bypass());
    
#define SWR(name) \
    stream.writeFloat(get_##name());

    SWR(cal_dBSPL);
    SWR(cal_dBFS);
    SWR(brighten);
    SWR(releaseFast);
    SWR(releaseSlow);
#undef SWR
    
    for(int grp = 0; grp < 4; ++grp) {
        for(int ix = 0; ix < 10; ++ix) {
            stream.writeFloat(get_aud_parm(grp, ix));
        }
    }
}

void jCrescendoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    syslog(1, "Crescendo Timing: setStateInformation()  inst: %d", get_id());
    
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    
    ignore_changes = true;
    set_bypass(stream.readBool());
    
#define SRD(name) \
    set_##name(stream.readFloat());

    SRD(cal_dBSPL   );
    SRD(cal_dBFS    );
    SRD(brighten    );
    SRD(releaseFast );
    SRD(releaseSlow );
#undef SRD

    for(int grp = 0; grp < 4; ++grp) {
        for(int ix = 0; ix < 10; ++ix) {
            set_aud_parm(grp, ix, stream.readFloat());
        }
    }

    ignore_changes = false;
    set_releases();
    set_audiology();
    notifier.sendChangeMessage();
 }

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new jCrescendoAudioProcessor();
}

// -------------------------------------------------------------------------------

void jCrescendoAudioProcessor::adj_brightness() {
    if(!ignore_changes) {
        pcresc->set_bright_audiology(get_brighten(), &params);
        ignore_changes = true;
        for(int ix = 0; ix < 10; ++ix) {
            set_aud_parm(AUDL, ix, params.aud[AUDL][ix]);
            set_aud_parm(AUDR, ix, params.aud[AUDR][ix]);
        }
        ignore_changes = false;
        notifier.sendChangeMessage();
    }
}
