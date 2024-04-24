//
//  channel.cpp
//  Crescendo-DSP-lib
//
//  Created by David McClain on 8/12/23.
//

#include <stdio.h>

#include <memory.h>
#include <stdlib.h>

#include "crescendo.h"
#include "fletch.h"
#include "crescendo_polys.h"
#include "ipp_intf.h"
#include "old-dither.h"

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------

// ------------------------------------------------------------------
// The Crescendo 3D Algorithm
// FFT-based 1 Correction Band per Critical Band
//

// --------------------------------------------------------------------------

Float64 TCrescendo_bark_channel::compute_hcgain(UInt32 zbark, Float64 dbel, bark_rec *pbark)
{
    Float64 dbgain = 0.0;
    if(dbel > 100.0)
        dbel = 100.0;
    Float64 phon  = dbel_to_phon(zbark, dbel);
    // Float64 dbelb = dbel - get_brighten();
    // Float64 phonb = dbel_to_phon(zbark, dbelb);
    
    Float32 *pspec = get_monitored_pspec();
    if(pspec)
        pspec[zbark] = (Float32)phon;
    
    if(get_Processing())
    {
        Float64 gprev  = pbark->prev_gain;
        // provide 10 dB hysteresis at noise floor
        if((phon > 30.0) ||
           ((gprev > 0.0) && (phon > 20.0)))
        {
            Float64 dh = pbark->dh;
            if(dh < 0.0)
                // for Hyper-Recruitment we pre-attenuate the input signal
                phon += dh;
            
            dbgain = phon_to_dbel(zbark, INV_SONES(pbark->inv_frac_live * (SONES(phon) + pbark->sdbel)) + dh) - dbel;
            dbgain = min(get_MaxGain(), dbgain);

            // slow rise, immediate drop
            if(dbgain > gprev)
            {
                gDither.safe_relax(gprev, dbgain, get_GainRelease_tc());
                dbgain = gprev;
            }
        }
    }
    pbark->prev_gain = dbgain;

    if(pspec)
        pspec[zbark+12] = (Float32)dbgain;
    
    return dbgain;
}

void TCrescendo_bark_channel::compute_bark_gain(UInt32 bix)
{
    // now compute Bark channel output gain
    Float64   bpwr        = compute_bark_power(bix);
    bark_rec *pbark       = get_bark_rec(bix);
    Float64   releaseSlow = get_ReleaseSlow_tc();
    Float64   releaseFast = get_ReleaseFast_tc();
    UInt32    holdct      = get_HoldCt();
    
    // -----------------------------------------------------------------------
    // compute attack and release on measured power
    // uses a fast attack and slower release, plus a hold
    
    // Dynamics in linear power space, not dB space.
    // Exponential decay in linear space is linear dB decline with time.
    Float64 mn = pbark->mean_pwr;
    gDither.safe_relax(mn, bpwr, releaseSlow);
    pbark->mean_pwr = mn;
    
    Float64 prev = pbark->prev_pwr;
    
    if(bpwr > 4.0 * prev) { // 6 dB
        pbark->release = releaseFast; // 50 ms
        pbark->holdctr = holdct;
        prev = bpwr;
    }
    else if(bpwr > prev) {
        gDither.safe_relax(prev, bpwr, releaseFast);
        if(pbark->holdctr > 0)
            pbark->holdctr = holdct;
    }
    else if(pbark->holdctr > 0)
        // hold until hold counter empties
        --pbark->holdctr;
    else
    {
        // if releasing from strong impulse (> 6 dB above mean levels)
        // then use a fast release until we fall below 3 dB above mean levels
        //
        // Otherwise if we are within 3 dB of mean levels use a slow release
        // We have hysteresis to avoid toggling between fast and slow release.
        //
        // If we are in-between these two cases, then we continue using the release
        // we last used.
        //
        gDither.safe_relax(prev, bpwr, pbark->release);
        if (prev < 2.0 * mn) // 3 dB
            pbark->release = releaseSlow; // 200 ms
    }
    pbark->prev_pwr = prev;
    Float64 pwrhl = convert_dBFS_to_dBSPL(db10(prev) - get_selfCalSF()); // ATH already subtracted
    
    // -----------------------------------------------------------------------
    // now compute HC gains...
    
    // the dB power in the cochlea, including masking contributions
    // and fast attack, slow release with hold
    
    // amount of HC gain needed
    Float64 dbgain = compute_hcgain(bix, pwrhl, pbark);
    m_gains[bix] = dbgain;
}

void TCrescendo_bark_channel::compute_bark_gains()
{
#if 0
    // dzero(m_gains, NBKCHANS+1);
    for(int bix = NBKCHANS-1; --bix >= 0;)
        par(^{ compute_bark_gain(bix); });
    compute_bark_gain(NBKCHANS-1);
    wait();
#else
    for(int bix = NBKCHANS; --bix >= 0;)
        compute_bark_gain(bix);
#endif
}

//-------------------------------------------------------------------
#define USE_FIR 0

#if !USE_FIR
void TCrescendo_bark_channel::compute_filter()
{
    compute_ft_gains();
    Float64 *pfilter = get_Filter();
    inv_FFT(pfilter);
    halfWindowed_FFT(pfilter, pfilter);
    set_FT_Nyquist(pfilter, 0.0);
}
#else
void TCrescendo_bark_channel::compute_filter()
{
    compute_ft_gains();
    Float64 *pfilter = get_Filter();
    inv_FFT(pfilter);
    
    Float64 *pwin     = get_HalfWindow();
    UInt32   qblksize = get_qblksize();
    dmul3(pwin + qblksize, pfilter, pfilter+qblksize, qblksize);
    dmul3(pwin, pfilter + 3*qblksize, pfilter, qblksize);
}
#endif

void TCrescendo_bark_channel::compute_power_spectrum()
{
    Float64 *pwr_spectrum = get_PowerSpectrum();
    
    select_data(2, pwr_spectrum);
    windowed_FFT(pwr_spectrum, pwr_spectrum);
    set_FT_Nyquist(pwr_spectrum, 0.0);
}

void TCrescendo_bark_channel::update_filter()
{
    compute_power_spectrum();
    compute_bark_gains();
    if(get_Processing())
        compute_filter();
}

// ------------------------------------------------------------------------

void TCrescendo_bark_channel::select_data(UInt ix, Float64* pdata)
{
    // pin points to a buffer of 3*hblksize input samples
    // m_ioff is the index of half blocks into pin, at which the latest input had been saved
    // delay is 1/2 block = 2.67 ms from input @ 48 kHz
    UInt32 hblksize = get_hblksize();
    Float64 *pin    = get_circBuf();
    switch((get_iCtr() + ix) % 3)
    {
        case 0:
            dcopy(pin, pdata, 2*hblksize);
            break;
            
        case 1:
            dcopy(pin + hblksize, pdata, 2*hblksize);
            break;
            
        case 2:
            dcopy(pin + 2*hblksize, pdata, hblksize);
            dcopy(pin, pdata + hblksize, hblksize);
            break;
    }
}

#if !USE_FIR
void TCrescendo_bark_channel::render_samples()
{
    update_filter();
    
    // non-windowed transform for data
    // overlap-save convolution does not use data windowing
    // 1/2 block delay from filter center = 2.67 ms at 48 kHz
    Float64 *pAud = get_AudBuf();
    select_data(1, pAud);
    if(get_Processing())
    {
        fwd_FFT(pAud);
        mulSpec(get_Filter(), pAud, pAud);
        inv_FFT(pAud);
    }
}
#else
void TCrescendo_bark_channel::render_samples()
{
    update_filter();
    
    // non-windowed transform for data
    // overlap-save convolution does not use data windowing
    // 1/2 block delay from filter center = 2.67 ms at 48 kHz
    Float64 *pAud = get_AudBuf();
    select_data(1, pAud);
    if(get_Processing())
    {
        Float64 *pfilt = get_Filter();
        UInt32  lim = get_hblksize();
        for(int ix = 0; ix < lim; ++ix) {
            Float64 v = 0.0;
            vDSP_dotprD(pfilt, 1, pAud+ix, 1, &v, lim);
            pAud[ix] = v / 256;
        }
    }
}
#endif

//-------------------------------------------------------------------
//
void TCrescendo_bark_channel::Resize()
{
#if 0
    // ----------------------------------
    fprintf(stderr,"Setting sample rate: %F\n", sampleRate);
    fflush(stderr);
    // ----------------------------------
#endif
    resize_FFT();
    resize_WrkBuf();
    resize_Filter();
    resize_circBuf();
}

// -------------------------------------------------------------------------------------

TCrescendo_bark_channel::TCrescendo_bark_channel(TCrescendo *parent)
: TPar_User(), TFFT_User(parent)
{
#if 0
    // ----------------------------------
    fprintf(stderr,"Initialize for sample rate: %F\n", sampleRate);
    fflush(stderr);
    // ----------------------------------
#endif
    dzero(m_gains,  NBKCHANS+1);
    dzero(m_d2ydx2, NBKCHANS+1);
}

TCrescendo_bark_channel::~TCrescendo_bark_channel()
{}

// -------------------------------------------------------

void TCrescendo_bark_channel::Process(Float32 *pin, Float32 *pout, UInt32 nsamp, Float32 *pspec)
{
    set_monitored_pspec(pspec);
    
    TDither dith(nsamp);
    
    Float64 *pobuf    = get_obufptr();
    UInt32   hblksize = get_hblksize();
    int      iScrap   = get_iScrap();
  
    while(nsamp + iScrap >= hblksize)
    {
        Float64 *pibuf = get_ibufptr();
        UInt32 nel = hblksize - iScrap;
        copy_ftod(pin, pibuf + iScrap, nel);
        pin   += nel;
        copy_dtof(pobuf + iScrap, pout, nel);
        pout  += nel;
        nsamp -= nel;
        iScrap = 0;
        render_samples();
        incr_iCtr(); // step to next half-block
    }
    if(nsamp > 0)
    {
        Float64 *pibuf = get_ibufptr();
        copy_ftod(pin, pibuf + iScrap, nsamp);
        copy_dtof(pobuf + iScrap, pout, nsamp);
        iScrap += nsamp;
    }
    set_iScrap(iScrap);
}

