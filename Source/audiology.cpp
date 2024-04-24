// audiology.cpp -- Audiology handling for Crescendo
// DM/RAL  06/11
// --------------------------------------------------
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

#include "crescendo.h"
#define GEN_POLY_TABLES 1
#include "crescendo_polys.h"

//-------------------------------------------------------------------
//
// -------------------------------------------------------------------------------------
inline Float32 clip_to_range(Float32 val, Float32 vmin, Float32 vmax)
{
	return max(vmin, min(val, vmax));
}

// -------------------------------------------------------------------------------------
#if BARK_SQUARE
#if 0
void TCrescendo_bark_channel::set_vtuning(Float32 vtune)
{
    // vtune is dB / Bark
    
	// slope factor is roughly 75 dB over 19 bark, which corresponds to about 6 kHz.
    // Bark 2.5 = 250 Hz. Assume all bands 250 Hz and below need no correction
    
    int start = roundf( 2.5f*NSUBBANDS); // about 250 Hz
    int stop  = roundf(20.0f*NSUBBANDS); // about 6 kHz
    Float32 vtunx = vtune / NSUBBANDS;   // rate per Bark subband
	
    for(int ix = 0; ix <= NSUBBANDS*NFBANDS; ++ix)
	{
		Float32 v = 0.0f;
        if(ix > start)
        {
            if(ix < stop)
                v = vtunx * (ix - start);
            else
                v = vtunx * (stop - start);
        }
		// if the predicted threshold elevation is above our limiting max value
		// then ramp abruptly down to zero to avoid gratuitous noise generation
#if 0
		if(v > 90.0)
			v = 0.0;
		else
#endif
			v = clip_to_range(v, 0.0f, 80.0);

		Float32 df = v / 5.0f;
		UInt32   jx = (UInt32)floor(df);
		df -= jx;

		m_bark[ix].interp_frac = df;
		m_bark[ix].pcoff1 = gfits[jx];
		m_bark[ix].pcoff2 = gfits[jx+1];
    }
}
#else
void TCrescendo_bark_channel::set_vtuning(Float32 vtune)
{
    // vtune is threshold elevation in dBHL at 4kHz
    Float32 slope = 3.575f / NSUBBANDS;
#if 0
    int start = roundf( 2.5f*NSUBBANDS); // about 250 Hz
    int stop  = roundf(20.0f*NSUBBANDS); // about 6 kHz
#endif
    
    for(int ix = 0; ix <= NSUBBANDS*NFBANDS; ++ix)
    {
        int fx = ix;
#if 0
        if(fx < start)
            fx = start;
        else if(fx > stop)
            fx = stop;
#endif
        
        // 17.5 zbark = 4 kHz
        Float32 v = vtune + slope * (fx - 17.5f*NSUBBANDS);
        if(!(0.0f < v && v <= 75.0f))
            v = 0.0f;
        // v = clip_to_range(v, 0.0f, 80.0f);
        
        Float32 df = v / 5.0f;
        UInt32   jx = (UInt32)floor(df);
        df -= jx;
        
        m_bark[ix].interp_frac = df;
        m_bark[ix].pcoff1 = gfits[jx];
        m_bark[ix].pcoff2 = gfits[jx+1];
        m_bark[ix].dbel = v;
    }
}
#endif // 0
#endif // BARK_SQUARE
// ----------------------------------------------------------
#if BARK_TRI

void TCrescendo_bark_channel::set_vtuning(Float32 vtune)
{
    for(int ix = 0; ix < NBKCHANS; ++ix)
    {
        Float32 v;
        
#if !DBM_AUDIOLOGY
        // vtune is threshold elevation in Phon at 4kHz
        static Float32 slope = 3.276;
        static Float32 dbk = BARKBW;
        
        Float32 zbark = ix * dbk;

        // 17.5 zbark = 4 kHz
        v = vtune + slope * (zbark - 17.5f);
        if(v < 0.0f)
            v = 0.0f;
        else if(v > 78.0f)
            v = 0.0f;
#else // DBM_AUDIOLOGY
        static Float32 pdbs[NBKCHANS] = {
            // ISO226 phon -> dBSPL conversions
            // zb, fkhz, dbel (phons)
             /*  0,  0.00f, */  0.00f,
             /*  1,  0.10f, */  0.00f,
             /*  2,  0.20f, */  1.44f,
             /*  3,  0.30f, */  4.72f,
             /*  4,  0.40f, */  7.95f,
             /*  5,  0.51f, */ 12.03f,
             /*  6,  0.63f, */ 16.45f,
             /*  7,  0.77f, */ 18.99f,
             /*  8,  0.91f, */ 20.26f,
             /*  9,  1.08f, */ 26.02f,
             /* 10,  1.27f, */ 36.08f,
             /* 11,  1.48f, */ 45.12f,
             /* 12,  1.72f, */ 49.40f,
             /* 13,  2.00f, */ 49.92f,
             /* 14,  2.32f, */ 48.76f,
             /* 15,  2.70f, */ 46.86f,
             /* 16,  3.15f, */ 44.93f,
             /* 17,  3.70f, */ 44.76f,
             /* 18,  4.39f, */ 49.99f,
             /* 19,  5.26f, */ 59.69f,
             /* 20,  6.41f, */ 65.97f,
             /* 21,  7.69f, */ 64.62f,
             /* 22,  9.40f, */ 61.83f,
             /* 23, 11.85f, */ 67.35f,
             /* 24, 15.64f, */  0.00f,
        };
        v = pdbs[ix] * vtune / 60.0f;
#endif // DBM_AUDIOLOGY
        Float32 df = v / 5.0f;
        UInt32   jx = (UInt32)floor(df);
        df -= jx;
        
        m_bark[ix].interp_frac = df;
        m_bark[ix].pcoff1 = gfits[jx];
        m_bark[ix].pcoff2 = gfits[jx+1];
        m_bark[ix].dbel = v;
        m_bark[ix].sdbel = SONES(v) - SONES(0.0);
    }
}

void TCrescendo_bark_channel::make_same_vtuning(TCrescendo_bark_channel *pchan) {
    for(int ix = 0; ix < NBKCHANS; ++ix)
    {
        if(pchan->m_bark[ix].dbel == 0.0) {
            m_bark[ix].dbel = 0.0;
            m_bark[ix].sdbel = 0.0;
        }
    }
}

#endif // BARK_TRI

// -----------------------------------------------------------------------
#if BARK_SMOOTH

static const Float64 sones_thresh = 0.002066115702479776;

Float64 frac_live(Float64 phon_elev) {
    return (1.0 / (1.0 + SONES(phon_elev) / SONES(120.0)) );
}

void TCrescendo_bark_channel::fill_chan_aud(UInt32 dst_ix, Float32 aud, Float32 dh)
{
    // conversion dBHL -> dBSPL -> Phon
    Float64 phon = dbel_to_phon(dst_ix, aud);
    if(dh > 0.0)
        // for Decruitment we diminish the threshold elevation and apply post-gain
        phon -= dh;
    
    if(phon < 0.0)
        phon = 0.0;
    if(phon > 100.0)
        phon = 100.0;

    Float64 fl = frac_live(phon);
    
    m_bark[dst_ix].inv_frac_live = 1.0 / fl;
    m_bark[dst_ix].sdbel  = fl*SONES(phon) - sones_thresh;
    m_bark[dst_ix].dh     = dh;
}

void TCrescendo_bark_channel::set_audiology(Float32 *pAud, Float32 *pDH) {
    for(int ix = 0; ix < NBKCHANS-1; ++ix)
        fill_chan_aud(ix, pAud[ix], pDH[ix]);
    fill_last_chan(pAud, pDH);
}

void TCrescendo_bark_channel::fill_last_chan(Float32 *pAud, Float32 *pDH) {
    fill_chan_aud(NBKCHANS-1, pAud[NBKCHANS-2], pDH[NBKCHANS-2]); // extend using 8kHz audiology for 12kHz chan
    pAud[NBKCHANS-1] = pAud[NBKCHANS-2];
    pDH[NBKCHANS-1] = pDH[NBKCHANS-2];
}

extern Float64 ztst[];

void TCrescendo_bark_channel::set_bright_audiology(Float64 bright, Float32 *pAud, Float32 *pDH) {
    static const Float32 slope = 3.276;  // dB/zBark threshold elevation slope
    Float32 vt4ph = dbel_to_phon(7, bright); // ix=7 is 4 kHz
    for(int ix = 0; ix < NBKCHANS-1; ++ix)
    {
        Float32 zbark = ztst[ix];
        // 17.0 zbark = 4 kHz
        Float32 phon = vt4ph + slope * (zbark - 17.0f);
        Float32 dBEl = phon_to_dbel(ix, phon);
        pAud[ix] = dBEl;
        fill_chan_aud(ix, dBEl, pDH[ix]);
    }
    fill_last_chan(pAud, pDH);
}
#endif // BARK_SMOOTH

// -------------------------------------------------------
// Closed-form solutions from the EarSpring model

/*
 (:CA 3.1274886893054256E-16 :CB 1.7260691854979844E-36 :CE 118879.22940404999 :CF 1.4260192437899379E-7)
 
 (defun new-sones (phons)
   (let* ((b     (* 4 (sqr *beta*)))
          (g     (sqr *biggamma*))
          (psq   (ampl10 (- phons 40)))
          (ca    (* 3 (sqrt 3) (sqr g) (+ b g)))
          (capsq (* ca psq))
          (cb    (* 4 (cube (* b g))))
          (rad  (cubert
                 (+ capsq
                    (sqrt (+ cb
                             (sqr capsq))))
                 ))
          (norm (* g (sqr (cubert 6))))
          (cc   (/ norm))
          (cd   (cubert (sqrt 3)))
          (ce   (* cc cd (cubert 2)))
          (cf   (/ (* cc 2 (cubert 3) b g) cd))
          (s  (-
               (* ce rad)
               (/ cf rad))))
     (print
      (list :ca ca
            :cb cb
            :ce ce
            :cf cf))
     s))
*/

// Phons to Sones
Float64 es_sones(Float64 phons) {
    static const Float64 es_ca    = 3.1274886893054256E-16;
    static const Float64 es_cb    = 1.7260691854979844E-36;
    static const Float64 es_ce    = 118879.22940404999;
    static const Float64 es_cf    = 1.4260192437899379E-7;
    Float64 psq  = ampl10(phons - 40.0);
    Float64 apsq = es_ca * psq;
    Float64 rad  = pow(apsq + sqrt(es_cb + apsq*apsq), 1.0/3.0);
    return (es_ce * rad - es_cf / rad);
}

// Sones to Phons
Float64 inv_es_sones(Float64 sones) {
    static const Float64 es_a = 0.04839593776013318;
    static const Float64 es_b = 0.9516040622398668;
    Float64 psq = sones * (es_a + es_b * sones * sones);
    return (40.0 + db10(psq));
}

// -- end of audiology.cpp -- //
