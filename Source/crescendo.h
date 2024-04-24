//-------------------------------------------------------------------
// Crescendo.h -- Crescendo for Mac VST and Mac Audio Units
// Stereo plugin which applies Left/Right Crescendo Hearing Corrections
// (C) 2005, Refined Audiometrics Laboratory, LLC. All rights reserved.
// DM/RAL  01/05-11/16
//-------------------------------------------------------------------
/* -----------------------------------------------------------------------------
 Copyright (c) 2016 Refined Audiometrics Laboratory, LLC
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 3. The names of the authors and contributors may not be used to endorse
 or promote products derived from this software without specific prior
 written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGE.
 ------------------------------------------------------------------------------- */

#ifndef __CRESCENDO_H__
#define __CRESCENDO_H__

#include "Version.h"
#include "useful_math.h"
#include "ipp_intf.h"
#include "smart_ptr.h"
// #include "TFilter.h"
#include "hdpheq.h"
#include "vTuningParams.h"
// #include "crossover.h"

// -------------------------------------------------------------
// The Crescendo 3D Algorithm
// One band of correction for every Bark band
//
#if BARK_SQUARE
#define NFBANDS         25
#define NSUBBANDS        4
#define NBKCHANS        (NFBANDS*NSUBBANDS)
#endif

#if BARK_TRI
#define NBR_BARK_CHANS  24
#define NBKCHANS        (NBR_BARK_CHANS+1)
#define BARKBW          (24.0/NBR_BARK_CHANS)
#endif

#if BARK_SMOOTH
#define NBR_BARK_CHANS  24
#define NBKCHANS        11
#define BARKBW          2.5
#endif

#define USE_ORIGINAL_FITS            0
#define USE_MATHEMATICA_ASONES_FITS  0
#define USE_EARSPRING_SONES_FITS     1

#define USE_EARSPRING  1
#if USE_EARSPRING
#define SONES     es_sones
#define INV_SONES inv_es_sones
#else // using approximation
#define SONES     asones
#define INV_SONES inv_asones
#endif

// -------------------------------------------------------------
// Approximations good to < 2% above 35 phon

inline Float64 asones(Float64 dbhl) {
    return pow(10.0, (dbhl - 40.0)/30.0);
}

inline Float64 inv_asones(Float64 s) {
    return (40.0 + 30.0 * log10(s));
}

// -------------------------------------------------------------

extern Float64 es_sones(Float64 phons);
extern Float64 inv_es_sones(Float64 sones);

// -------------------------------------------------------------
//
class TCrescendo_bark_channel;

// -------------------------------------------------------------
// Generic class for subclasses that utilize FFT's and parallel dispatch

class TPar_User
{
    dispatch_group_t m_grp;
    dispatch_queue_t m_q;
    
public:
    TPar_User();
    virtual ~TPar_User();
    
    void par(dispatch_block_t block)
    {
        dispatch_group_async(m_grp, m_q, block);
    }
    
    void wait()
    {
        dispatch_group_wait(m_grp, DISPATCH_TIME_FOREVER);
    }
};

// -------------------------------------------------------------
//
#define G_VAR(type,name) \
type m_##name; \
inline type get_##name() { return m_##name; }

#define DZ_VAR(name) \
DZPtr m_##name; \
inline Float64* get_##name() { return m_##name(); }

#define GS_VAR(type,name) \
type m_##name; \
inline type get_##name() { return m_##name; } \
inline void set_##name(type arg) { m_##name = arg; }

// ----------------------------------------------------

typedef struct {
  Float64 fkHz;
  int     gix;
  Float64 a;
  Float64 b;
  Float64 c;
} TGainInterp;

typedef struct {
    Float64  m_fmid;
    int      m_start;
    int      m_end;
    Float64  m_erb;
    Float64 *m_pcoffs;
} Bark_Band_T;

// -------------------------------------------------------------

class TCrescendo: public TPar_User
{
    TPtr<TCrescendo_bark_channel> LeftChannel;
    TPtr<TCrescendo_bark_channel> RightChannel;

    inline void discard_DataWindow() {
        m_DataWindow.discard();
    }

    // TPtr<TCrossOver> HdphX;
    
    Float64 m_InvATH[129];
    
    GS_VAR(Float32, sampleRate);
    inline bool sampleRate_changed(Float32 new_sampleRate) {
        return !within(get_sampleRate(), new_sampleRate, 1.0f);
    }
    
    GS_VAR(Float32, releaseFast_ms);
    GS_VAR(Float32, releaseSlow_ms);
    
    Float32 audL[10];
    Float32 audR[10];
    Float32 dhL[10];
    Float32 dhR[10];
    
    Float64 compute_tc(Float32 tms);
    void    fill_bark_interpolation_tables();
    void    fill_bark_tables();
    void    fill_ft_tables();
    void    interpolate_FrqStruct(t_FrqStruct *eqtbl, Float64 *dst, tAmplFn *pfn, bool norm1kHz = true);
    void    compute_inverse_ATH_filter();
    
    inline void set_block_sizes(UInt32 blksize) {
        set_blksize(blksize);
        set_hblksize(blksize >> 1);
        set_qblksize(blksize >> 2);
    }
    inline Float64 blkSR()
    { return ((Float64)get_sampleRate() / get_hblksize()); }
    
    // -------------------------------------------------------------------
    
public:
    TCrescendo(Float32 sampleRate);
    virtual ~TCrescendo();
    
    GS_VAR(bool,    Processing);
    
    GS_VAR(UInt32,  blksize);
    GS_VAR(UInt32,  hblksize);
    GS_VAR(UInt32,  qblksize);
    
    GS_VAR(UInt32,  HoldCt);
    GS_VAR(Float64, ReleaseFast_tc);
    GS_VAR(Float64, ReleaseSlow_tc);
    GS_VAR(Float64, GainRelease_tc);
    
    GS_VAR(Float64, MaxGain);
    GS_VAR(Float64, CaldBSPL);
    GS_VAR(Float64, CalLUFS);
    GS_VAR(Float64, selfCalSF);
    GS_VAR(Float64, brighten);
    GS_VAR(Float64, outLimit);
    GS_VAR(SInt32,  maxProcDur);
    GS_VAR(UInt32,  lastNSamp);
    
    GS_VAR(Bark_Band_T*, pBarkFilts);
    GS_VAR(Float64*,     pZTerp);
    GS_VAR(TGainInterp*, pGainInterp);
    GS_VAR(UInt32,       bin16k);
    
    DZ_VAR(DataWindow);
    DZ_VAR(HalfWindow);

    inline void resize_Windows() {
         m_DataWindow.realloc(get_blksize());
         m_HalfWindow.realloc(get_blksize());
     }
    
    void    SetSampleRate(Float32 sampleRate);
    void    set_releases(Float32 releaseFast_ms, Float32 releaseSlow_ms);
    void    set_audiology(tVTuningParams *parms);
    void    render(Float32 *pinL, Float32 *pinR,
                   Float32 *poutL, Float32 *poutR,
                   UInt32 nel, tVTuningParams *parms);
    Float64 get_power();
    void    get_levels(Float64 &lrms, Float64 &rrms);
    UInt32  get_latency_samples();
    
    inline Float64* get_InvATH()
    { return m_InvATH; }
    
    inline Float64 convert_dBFS_to_dBSPL(Float64 pdb)
    // account for +3 dB added to LUFS meters
    // -23 dBFS peak Sine has RMS -26 dBFS, but shows LUFS -23
    { return (pdb + get_CaldBSPL() - (get_CalLUFS() - 3.0)); }
    
    void set_bright_audiology(Float64 bright, tVTuningParams *parms);
};

// -------------------------------------------------------------

#define REF_PARENT(type, name) \
inline type get_##name() { return (type)(get_parent()->get_##name()); }

#define RW_PARENT(type, name) \
REF_PARENT(type, name); \
inline void set_##name(type val) { get_parent()->set_##name(val); }

// -------------------------------------------------------------
// TCrescendo_User - parent class that refers to a TCrescendo instance

class TCrescendo_User
{
    GS_VAR(TCrescendo*, parent);
    
public:
    TCrescendo_User(TCrescendo* parent)
    { set_parent(parent); }
    
    virtual ~TCrescendo_User()
    {}
    
    REF_PARENT(UInt32,   blksize);
    REF_PARENT(UInt32,   hblksize);
    REF_PARENT(UInt32,   qblksize);
    
    REF_PARENT(bool,     Processing);
    REF_PARENT(Float64,  MaxGain);
    REF_PARENT(UInt32,   HoldCt);
    REF_PARENT(Float64,  ReleaseFast_tc);
    REF_PARENT(Float64,  ReleaseSlow_tc);
    REF_PARENT(Float64,  GainRelease_tc);
    REF_PARENT(Float64*, InvATH);
    REF_PARENT(Float64,  brighten);
    REF_PARENT(Float64,  outLimit);

    RW_PARENT(Float64,   selfCalSF);
    
    REF_PARENT(Float64*, DataWindow);
    REF_PARENT(Float64*, HalfWindow);
    
    REF_PARENT(Bark_Band_T*, pBarkFilts);
    REF_PARENT(Float64*,     pZTerp);
    REF_PARENT(TGainInterp*, pGainInterp);
    REF_PARENT(UInt32,       bin16k);

    inline void resize_Windows() {
        get_parent()->resize_Windows();
    }
    
    inline Float64 convert_dBFS_to_dBSPL(Float64 pdb)
    { return get_parent()->convert_dBFS_to_dBSPL(pdb); }
    
};

// -------------------------------------------------------------
// FFT's are not reentrant, so be sure to make private FFT objects for each thread.

class TFFT_User : public TCrescendo_User
{
    TPtr<ipp_fft>   m_AudioFFT;
    inline ipp_fft* get_FFT()
    {  return m_AudioFFT(); }
    
    void    init_windows();
    void    self_calibrate();
    Float64 total_fft_pwr(UInt32 hblksize, Float64 *pwr_spectrum);

public:
    TFFT_User(TCrescendo *parent)
    : TCrescendo_User(parent)
    {};
    virtual ~TFFT_User() {};


    // ---------------------------------------
    // High level, unified access to FFT cells
    
    void resize_FFT();
    
    inline void fwd_FFT(Float64 *pdata) {
        get_FFT()->fwd(pdata);
    }
    
    inline void inv_FFT(Float64 *pdata) {
        get_FFT()->inv(pdata);
    }
    
    inline void windowed_FFT(Float64 *psrc, Float64 *pdst) {
        dmul3(get_DataWindow(), psrc, pdst, get_blksize());
        fwd_FFT(pdst);
    }
    
    inline void halfWindowed_FFT(Float64 *psrc, Float64 *pdst) {
        Float64 *pwin     = get_HalfWindow();
        UInt32   qblksize = get_qblksize();
        UInt32   hblksize = get_hblksize();
        dmul3(pwin + qblksize, psrc, pdst, qblksize);
        dzero(pdst + qblksize, hblksize);
        dmul3(pwin, psrc + 3*qblksize, pdst + 3*qblksize, qblksize);
        fwd_FFT(pdst);
    }
    
    inline void get_FT_cell(Float64* ft, UInt32 ix, Float64 &re, Float64 &im) {
        // Assumes 0 <= ix < hblksize
        get_FFT()->get_FT_cell(ft, ix, re, im);
    }
    
    inline void get_FT_DC(Float64* ft, Float64 &re) {
        get_FFT()->get_FT_DC(ft, re);
    }
    
    inline void get_FT_Nyquist(Float64* ft, Float64 &re) {
        get_FFT()->get_FT_Nyquist(ft, re);
    }

    
    inline void set_FT_cell(Float64* ft, UInt32 ix, Float64 re, Float64 im) {
        get_FFT()->set_FT_cell(ft, ix, re, im);
    }
    
    inline void set_FT_DC(Float64* ft, Float64 re) {
        get_FFT()->set_FT_DC(ft, re);
    }
    
    inline void set_FT_Nyquist(Float64* ft, Float64 re) {
        get_FFT()->set_FT_Nyquist(ft, re);
    }
    
    inline void zero_FT_cells(Float64 *ft, UInt32 start, UInt32 nel) {
        get_FFT()->zero_FT_cells(ft, start, nel);
    }
    
    inline void mulSpec(Float64 *spec1, Float64 *spec2, Float64 *dst) {
        get_FFT()->mulSpec(spec1, spec2, dst);
    }
    
    inline Float64 get_FT_power(Float64 *spec, UInt32 ix) {
        // Warning: Mac-specific (vDSP) code. All real parts are laid out
        // from index 0 upto hblksize, imag parts from hblksize upto blksize.
        //
        // There is no imag part for DC and Nyquist, but real Nyquist part is
        // stored at index hblksize, where imag DC would go if there were one.
        //
        // get_FT_cell() just fetches from index ix for real part, and index
        // ix + hblksize for imag part. At ix = 0 we get DC in real, and Nyquist
        // in imag.
        //
        // So, serendipitously, we can just use get_FT_cell() over all indices
        // from 0 upto hblksize. At ix = 0 we get contributions of DC and Nyquist.
        Float64 re, im;
        get_FT_cell(spec, ix, re, im);
        Float64 pwr = (re * re + im * im);
        return (ix > 0) ? pwr + pwr : pwr;
    }
};

// -------------------------------------------------------------------------

class TCrescendo_bark_channel: public TPar_User, public TFFT_User
{
    // -----------------------------------------------------
    // DataFlow:
    //
    //   Audio Inp --> hBuf --> circBuf --+--> PowerSpectrum --> Filter
    //                                    |
    //                                    +--> AudBuf --> hiBuf --> Audio Outp
	//
    // ----------------------------------------------------
        
    // --------------------------------------------------------
    // Buffer used to grab chunks of hblksize Float32 input audio converted to Float64
    // input chunk buffer - always hblksize
    
    GS_VAR(UInt32, iScrap);
    
    Float64* get_ibufptr() {
        return get_circBuf() + get_iCtr() * get_hblksize();
    }
    
    Float64* get_obufptr() {
        return get_AudBuf() + get_qblksize();
    }
    
    // ----------------------------------------------------
    // the buffer used as circular buffer of 3 hblksize chunks
    
    GS_VAR(UInt32, iCtr);
    inline void incr_iCtr()
    { set_iCtr((1 + get_iCtr()) % 3); }
    
    DZ_VAR(circBuf);
    inline void resize_circBuf() {
        set_iCtr(0);
        set_iScrap(0);
        set_level(0.0);
        m_circBuf.reallocz(3*get_hblksize()); // zero filled
    }

    // -----------------------------------------------------
    // Buffer shared between PowerSpectrum and AudBuf duties.
    
    DZ_VAR(WrkBuf);
    inline void resize_WrkBuf()
    { m_WrkBuf.reallocz(2*get_blksize()); }
    
    inline Float64* get_PowerSpectrum()
    { return get_WrkBuf(); }
    inline void resize_PowerSpectrum() { }
    
    inline Float64* get_AudBuf()
    { return get_WrkBuf(); }
    inline void resize_AudBuf() { }
    
    // -------------------------------------------------------
    // Used to construct correction filter
    
    DZ_VAR(Filter);
    inline void resize_Filter() {
        m_Filter.reallocz(2*get_blksize());
    }
    
    // ----------------------------------------------------
    
    GS_VAR(Float32*, monitored_pspec);

    // data for each Bark band
    struct bark_rec {
        // the following are dynamically updated during processing
        Float64  prev_pwr;
        Float64  mean_pwr;
        Float64  release;
        SInt32   holdctr;
        Float64  prev_gain;
        
        // the following set by audiology for each Bark band
        Float64  inv_frac_live;
        Float64  sdbel;
        Float64  dh;
        
        bark_rec()
        : prev_pwr(0.0), mean_pwr(0.0), release(0.0), holdctr(0),
            prev_gain(0.0), inv_frac_live(1.0), sdbel(0.0), dh(0.0)
        { }
        
        virtual ~bark_rec()
        { }
    };
    bark_rec m_bark[NBKCHANS];
    inline bark_rec* get_bark_rec(int ix) {
        return &m_bark[ix];
    }
    
    Float64 m_gains[NBKCHANS+1];
    Float64 m_d2ydx2[NBKCHANS+1];
    
    void    render_samples();
    void    update_filter();
    void    compute_power_spectrum();
    void    compute_filter();
    Float64 compute_bark_power(UInt32 bix);
    void    compute_ft_gains();
    void    compute_spline();
    Float64 compute_hcgain(UInt32 zbark, Float64 dbpwr, bark_rec *pbark);
    void    compute_bark_gains();
    void    compute_bark_gain(UInt32 ix);
    void    select_data(UInt32 ix, Float64 *pdata);
    void    make_same_vtuning(TCrescendo_bark_channel *pchan);
    void    fill_chan_aud(UInt32 dst_ix, Float32 aud, Float32 dh);
    void    saturate(Float64 *pAud, int nel);
    Float64 gain_at_bin(int bix);
    void    fill_last_chan(Float32 *pAud, Float32 *pDH);

public:
    GS_VAR(Float64, level);
    
	TCrescendo_bark_channel(TCrescendo *parent);
    virtual ~TCrescendo_bark_channel();

    void Resize();
    void set_bright_audiology(Float64 bright, Float32 *pAud, Float32 *pDH);
    void set_audiology(Float32 *pAud, Float32 *pDH);
    void Process(Float32 *pin, Float32* pout, UInt32 nsamp, Float32 *pspec);
 };


// ---------------------------------------------

enum {
    kEngine_Single,
    kEngine_Dual,
    kEngine_Dual_Pre,
    kEngine_Dual_Post
};

extern Float64 dbel_to_phon(UInt32 zbark, Float64 dbspl);
extern Float64 phon_to_dbel(UInt32 zbark, Float64 phon);

inline int blksize_for_sampleRate(Float32 sampleRate) {
    return (sampleRate > 50e3f) ? 512 : 256;
}

#undef G_VAR
#undef DZ_VAR
#undef GS_VAR
#undef SHADOWED_VAR
#undef SHADOWED_VEC
#undef REF_PARENT
#undef RW_PARENT

#endif // __CRESCENDO_H__

// -- end of Crescendo.h -- //
