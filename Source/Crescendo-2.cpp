//-------------------------------------------------------------------
// Crescendo.cpp -- Crescendo for Mac VST and Mac Audio Units
// Stereo plugin which applies Left/Right Crescendo Hearing Corrections
// (C) 2005-2016, Refined Audiometrics Laboratory, LLC. All rights reserved.
// DM/RAL  01/05-12/16
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

#include <memory.h>
#include <stdlib.h>

#include "crescendo.h"
#include "fletch.h"
#include "crescendo_polys.h"
#include "ipp_intf.h"
#include "old-dither.h"

// -------------------------------------------------------------
//
// -------------------------------------------------------------
//
Float64 e_folding(Float64 tcms, Float64 fsamp)
{
    // tcms in ms
    // fsamp in Hz
    
    if(tcms <= 0.0)
        return 1.0;
    else
        return (1.0 - exp(-1.0e3/(tcms * fsamp)));
}

//-------------------------------------------------------------------
//
TCrescendo::TCrescendo(Float32 sampleRate)
: TPar_User()
{
    set_MaxGain(50.0);  // Phon
    set_CalLUFS(-23.0); // dBFS
    set_CaldBSPL(77.0); // dBSPL
    set_releaseFast_ms(34.0f);  // ms
    set_releaseSlow_ms(155.0f); // ms
    set_brighten(0.0);
    set_outLimit(20.0);
    set_maxProcDur(-1);
    set_lastNSamp(0);
    
	set_Processing(true);

    memset(audL, 0, sizeof(audL));
    memset(audR, 0, sizeof(audR));
    memset(dhL,  0, sizeof(dhL));
    memset(dhR,  0, sizeof(dhR));
    
    set_blksize(0);       // samp
    set_sampleRate(0.0f); // Hz

    LeftChannel  = new TCrescendo_bark_channel(this);
    RightChannel = new TCrescendo_bark_channel(this);
    
	SetSampleRate(sampleRate);
}

TCrescendo::~TCrescendo()
{ }

//-------------------------------------------------------------------
//
Float64 TCrescendo::compute_tc(Float32 tms)
{
    return e_folding(tms, blkSR());
}

void TCrescendo::SetSampleRate(Float32 sampleRate)
{
    if(sampleRate_changed(sampleRate))
    {
        set_sampleRate(sampleRate);
        int blksize = blksize_for_sampleRate(sampleRate);
        if(blksize != get_blksize())
        {
            set_block_sizes(blksize);
            discard_DataWindow();
            
            LeftChannel ->Resize();
            RightChannel->Resize();
        }

        set_HoldCt((UInt32)ceil(10.0e-3 * blkSR())); // sec
        set_releases(get_releaseFast_ms(), get_releaseSlow_ms());
        set_GainRelease_tc(compute_tc(20.0f));  // ms
        
        fill_bark_interpolation_tables();
        compute_inverse_ATH_filter();
    }
}

void TCrescendo::set_bright_audiology(Float64 bright, tVTuningParams *parms) {
    LeftChannel ->set_bright_audiology(bright, parms->aud[AUDL], parms->aud[DHL]);
    RightChannel->set_bright_audiology(bright, parms->aud[AUDR], parms->aud[DHR]);
}

void TCrescendo::set_audiology(tVTuningParams *parms)
{
    LeftChannel ->set_audiology(parms->aud[AUDL], parms->aud[DHL]);
    RightChannel->set_audiology(parms->aud[AUDR], parms->aud[DHR]);
}

void TCrescendo::set_releases(Float32 releaseFast_ms, Float32 releaseSlow_ms)
{
    set_releaseFast_ms(releaseFast_ms);
    set_releaseSlow_ms(releaseSlow_ms);
    set_ReleaseSlow_tc(compute_tc(releaseSlow_ms));
    set_ReleaseFast_tc(compute_tc(releaseFast_ms));
}

void TCrescendo::render(Float32 *pinL, Float32 *pinR,
                        Float32 *poutL, Float32 *poutR,
                        UInt32 nsamp, tVTuningParams *parms)
{
    DAZFZ env;
    
    Float32 *pspecL = &parms->specL[0];
    Float32 *pspecR = &parms->specR[0];
    
    // Process Left/Right channels in parallel
    if(pinL && poutL) {
        if(pinR && poutR) {
            // par(^{ RightChannel->Process(pinR, poutR, nsamp, pspecR); } );
            RightChannel->Process(pinR, poutR, nsamp, pspecR);
            LeftChannel->Process(pinL, poutL, nsamp, pspecL);
            // wait();
        } else
            LeftChannel->Process(pinL, poutL, nsamp, pspecL);
    }
}

void TCrescendo::get_levels(Float64 &lrms, Float64 &rrms)
{
    lrms = LeftChannel->get_level();
    rrms = RightChannel->get_level();
}

Float64 TCrescendo::get_power()
{
    return LeftChannel->get_level();
}

UInt32 TCrescendo::get_latency_samples() {
    return (7 * get_qblksize());
}

// -- end of Crescendo.cpp -- //
