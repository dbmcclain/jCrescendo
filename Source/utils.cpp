//
//  utils.cpp
//  Crescendo-DSP-lib
//
//  Created by David McClain on 8/12/23.
//

#include <memory.h>
#include <stdlib.h>

#include "crescendo.h"


//-------------------------------------------------------------------

TPar_User::TPar_User()
{
    m_grp = dispatch_group_create();
    // m_q   = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
    m_q   = dispatch_queue_create("com.RAL.Crescendo.queue", DISPATCH_QUEUE_CONCURRENT);
}

TPar_User::~TPar_User()
{
    dispatch_release(m_q);
    dispatch_release(m_grp);
}

// -------------------------------------------------------------------------------------

void TFFT_User::resize_FFT()
{
    int fft_order = (get_blksize() == 512) ?   9 :   8;
    m_AudioFFT.realloc();
    m_AudioFFT->init(fft_order, IPP_FFT_DIV_FWD_BY_N);
    
    if(!get_DataWindow()) {
        init_windows();
        self_calibrate();
    }
}

// -------------------------------------------------------------------------------------

void TFFT_User::init_windows()
{
    // Hann Window over the whole block for power estimation
    resize_Windows();
    
    UInt32  blksize  = get_blksize();
    UInt32  hblksize = get_hblksize();
    UInt32  qblksize = get_qblksize();
    Float64 pif      = acos(-1.0) / blksize;
    Float64 *DataWindow = get_DataWindow();
    Float64 *HalfWindow = get_HalfWindow();
    
    DataWindow[0] = 0.0;
    DataWindow[hblksize] = 1.0;
    for(int ix = 1; ix < hblksize; ++ix)
    {
        Float64 v = sin(pif * ix);
        v *= v;
        DataWindow[ix]         = v;
        DataWindow[blksize-ix] = v;
    }
    
    // Hann Window over half block for filter formation
    HalfWindow[0] = 0.0;
    HalfWindow[qblksize] = 1.0;
    for(int ix = 1; ix < qblksize; ++ix)
    {
        Float64 v = sin(2.0 * pif * ix);
        v *= v;
        HalfWindow[ix]          = v;
        HalfWindow[hblksize-ix] = v;
    }
}

// -------------------------------------------------------------------------------------
// Windowed FFT Self Calibration for 0 dBFS

Float64 TFFT_User::total_fft_pwr(UInt32 hblksize, Float64 *pwr_spectrum)
{
    // Compute spectrum and sum over the signal power.
    fwd_FFT(pwr_spectrum);
    
    // DC and Nyquiet cells get twice amplitude levels from the FFT.
    // All the rest of the cells have half their signal in the positive
    // Freq cells, and we are ignoring the negative Freq cells due to
    // real valued signals going to a symmetric real and anti-symmetric
    // imaginary spectrum.
    //
    // So this really only measures half the actual power level in the FFT.
    //
    Float64 pwrsum = 0.0;
    for(int ix = 0; ix < hblksize; ++ix)
        pwrsum += get_FT_power(pwr_spectrum, ix);
    return pwrsum;
}

void TFFT_User::self_calibrate()
{
    int  ix;
    
    // Self calibration for power estimation.
    //
    // The character of the FFT routine, its scaling,
    // and the effects of data windowing may modify the
    // result from expected ideal values.
    //
    // This routine measures that variation and generates a
    // correction dB term for use during the running measurements
    // of signal power.
    
    UInt32 blksize  = get_blksize();
    UInt32 hblksize = get_hblksize();
    Float64 *pwin   = get_DataWindow();
    Float64 pwr_spectrum[blksize];
    
    // Construct a windowed unit amplitude SINE-wave exactly in center of FFT bin
    // at quarter sample-rate frequency
    
    for(ix = 0; ix < blksize; ix += 2) pwr_spectrum[ix] = 0.0;
    for(ix = 1; ix < blksize; ix += 4) pwr_spectrum[ix] = pwin[ix];
    for(ix = 3; ix < blksize; ix += 4) pwr_spectrum[ix] = -pwin[ix];
    
    Float64 pwrsum = total_fft_pwr(hblksize, pwr_spectrum);
    
    // Construct a windowed unit amplitude COSINE-wave exactly in center of FFT bin
    // at quarter sample-rate frequency
    
    for(ix = 1; ix < blksize; ix += 2) pwr_spectrum[ix] = 0.0;
    for(ix = 0; ix < blksize; ix += 4) pwr_spectrum[ix] = pwin[ix];
    for(ix = 2; ix < blksize; ix += 4) pwr_spectrum[ix] = -pwin[ix];

    pwrsum += total_fft_pwr(hblksize, pwr_spectrum);
    
    // We have 0 dBFS amplitude SINE and COSINE waves.
    // We add their power together to get double the ideal average value of each.
    // Both of them should have a net power level of 0.5 = -3 dBFS. The sum of
    // their powers represents 0 dBFS power.
    //
    // By subtracting the twice dB power level measured for these two 0 dBFS signals
    // from the measured dB power of any other signal, we get their power corrected
    // such that a 0 dBFS sinewave would measure -3 dBFS average power.
    //
    // Save the factor which represents this twice total power for a
    // 0 dBFS signal level. This removes any inadvertent FFT and Window
    // scaling on the power measurements.
    set_selfCalSF(db10(pwrsum));
}

