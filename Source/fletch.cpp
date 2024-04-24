// fletch.cpp
// DM/RAL 10/07
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
#include "fletch.h"

// ----------------------------------------------------------
// Fletcher-Munson Conversion dBSPL <-> dBHL
//

// #define FMC(bark, fmdb)  (120.0/(120.0-(fmdb)))
#define FMC(bark, fmdb)  (fmdb)

// Performing ATH correction to dBHL by directly multiplying signal
// powers in the linear frequency FFT with an interpolated ATH filter
// helps to minimize seam artifacts in a swept tone as we cross from one
// analysis band to another.

// ATH in FFT domain at 48 kHz Fsamp
// FMC(freq Hz, mag dB)
t_FrqStruct gATH = {
    48000, 256,
    {
// New (6/20) dBSPL <-> Phon Conversions ISO226
        FMC(    0.0,   20.0),
        FMC(  187.5,  10.93),
        FMC(  375.0,   3.57),
        FMC(  562.5,   0.81),
        FMC(  750.0,  -0.20),
        FMC(  937.5,  -0.25),
        FMC( 1125.0,   0.72),
        FMC( 1312.5,   0.88),
        FMC( 1500.0,  -0.37),
        FMC( 1687.5,  -1.88),
        FMC( 1875.0,  -3.25),
        FMC( 2062.5,  -4.53),
        FMC( 2250.0,  -5.71),
        FMC( 2437.5,  -6.74),
        FMC( 2625.0,  -7.60),
        FMC( 2812.5,  -8.28),
        FMC( 3000.0,  -8.77),
        FMC( 3187.5,  -9.07),
        FMC( 3375.0,  -9.20),
        FMC( 3562.5,  -9.16),
        FMC( 3750.0,  -8.97),
        FMC( 3937.5,  -8.64),
        FMC( 4125.0,  -8.17),
        FMC( 4312.5,  -7.58),
        FMC( 4500.0,  -6.88),
        FMC( 4687.5,  -6.06),
        FMC( 4875.0,  -5.15),
        FMC( 5062.5,  -4.14),
        FMC( 5250.0,  -3.04),
        FMC( 5437.5,  -1.90),
        FMC( 5625.0,  -0.75),
        FMC( 5812.5,   0.41),
        FMC( 6000.0,   1.55),
        FMC( 6187.5,   2.64),
        FMC( 6375.0,   3.70),
        FMC( 6562.5,   4.69),
        FMC( 6750.0,   5.62),
        FMC( 6937.5,   6.47),
        FMC( 7125.0,   7.26),
        FMC( 7312.5,   7.96),
        FMC( 7500.0,   8.58),
        FMC( 7687.5,   9.12),
        FMC( 7875.0,   9.57),
        FMC( 8062.5,   9.94),
        FMC( 8250.0,  10.23),
        FMC( 8437.5,  10.45),
        FMC( 8625.0,  10.60),
        FMC( 8812.5,  10.70),
        FMC( 9000.0,  10.75),
        FMC( 9187.5,  10.76),
        FMC( 9375.0,  10.74),
        FMC( 9562.5,  10.68),
        FMC( 9750.0,  10.61),
        FMC( 9937.5,  10.51),
        FMC(10125.0,  10.40),
        FMC(10312.5,  10.27),
        FMC(10500.0,  10.14),
        FMC(10687.5,   9.99),
        FMC(10875.0,   9.84),
        FMC(11062.5,   9.68),
        FMC(11250.0,   9.51),
        FMC(11437.5,   9.35),
        FMC(11625.0,   9.18),
        FMC(11812.5,   9.01),
        FMC(12000.0,   8.83),
        FMC(12187.5,   8.66),
        FMC(12375.0,   8.49),
        FMC(12562.5,   8.33),
        FMC(12750.0,   8.16),
        FMC(12937.5,   8.00),
        FMC(13125.0,   7.84),
        FMC(13312.5,   7.69),
        FMC(13500.0,   7.54),
        FMC(13687.5,   7.40),
        FMC(13875.0,   7.26),
        FMC(14062.5,   7.13),
        FMC(14250.0,   7.00),
        FMC(14437.5,   6.88),
        FMC(14625.0,   6.77),
        FMC(14812.5,   6.66),
        FMC(15000.0,   6.56),
        FMC(15187.5,   6.47),
        FMC(15375.0,   6.39),
        FMC(15562.5,   6.31),
        FMC(15750.0,   6.24),
        FMC(15937.5,   6.18),
        FMC(16125.0,   6.13),
        FMC(16312.5,   6.08),
        FMC(16500.0,   6.04),
        FMC(16687.5,   6.01),
        FMC(16875.0,   5.99),
        FMC(17062.5,   5.98),
        FMC(17250.0,   5.97),
        FMC(17437.5,   5.98),
        FMC(17625.0,   5.99),
        FMC(17812.5,   6.01),
        FMC(18000.0,   6.03),
        FMC(18187.5,   6.07),
        FMC(18375.0,   6.11),
        FMC(18562.5,   6.16),
        FMC(18750.0,   6.22),
        FMC(18937.5,   6.29),
        FMC(19125.0,   6.36),
        FMC(19312.5,   6.44),
        FMC(19500.0,   6.54),
        FMC(19687.5,   6.63),
        FMC(19875.0,   6.74),
        FMC(20062.5,   6.85),
        FMC(20250.0,   6.98),
        FMC(20437.5,   7.11),
        FMC(20625.0,   7.24),
        FMC(20812.5,   7.39),
        FMC(21000.0,   7.54),
        FMC(21187.5,   7.70),
        FMC(21375.0,   7.86),
        FMC(21562.5,   8.04),
        FMC(21750.0,   8.22),
        FMC(21937.5,   8.41),
        FMC(22125.0,   8.60),
        FMC(22312.5,   8.80),
        FMC(22500.0,   9.01),
        FMC(22687.5,   9.23),
        FMC(22875.0,   9.45),
        FMC(23062.5,   9.68),
        FMC(23250.0,   9.92),
        FMC(23437.5,  10.16),
        FMC(23625.0,  10.41),
        FMC(23812.5,  10.67),
        FMC(24000.0,  10.93),
    }
};
#undef FMC

Float64 invAmpl10(Float64 v)
{
    return ampl10(-v);
}

void TCrescendo::compute_inverse_ATH_filter()
{
    interpolate_FrqStruct(&gATH, get_InvATH(), &invAmpl10);
    // invalidate_unified_filter();
}

// -------------------------------------------------------------
//
Float64 qinterp(Float64 *pTbl, UInt32 ix1, Float64 x)
{
    // Quadratic interpolation for uniformly sampled data
    // x in [0,1], is frac of interval between y[0] and y[1]
    // where x: -1 .. 0 .. 1
    //                   ^
    //                   x
    // and: y[-1] .. y[0] .. y[1]
    //                     ^
    //                    y(x)
    //
    if(ix1 < 2)
    {
        x -= Float64(2 - ix1);
        ix1 = 2;
    }
    Float64 ym1 = pTbl[ix1-2];
    Float64 y0  = pTbl[ix1-1];
    Float64 yp1 = pTbl[ix1];
    Float64 c = 0.5*(yp1 + ym1) - y0;
    Float64 b = 0.5*(yp1 - ym1);
    return y0 + x*(b + x*c);
}

void TCrescendo::interpolate_FrqStruct(t_FrqStruct *eqtbl, Float64 *dst, tAmplFn *pfn, bool norm1kHz)
{
    UInt32 dst_ix, tbl_ix;
    SInt32 ctr;
    
    Float64 ref = 0.0;
    Float64 frac;
    
    if(norm1kHz)
    {
        frac = 1000.0 * Float64(eqtbl->nfft) / Float64(eqtbl->fsamp);
        tbl_ix = UInt32(floor(frac));
        frac -= Float64(tbl_ix);
        ref = qinterp(eqtbl->db, tbl_ix, frac);
    }

    UInt32 tbl_sr = eqtbl->fsamp;
    UInt32 cur_sr = UInt32(get_sampleRate());
    UInt32 tbl_limit  = eqtbl->nfft / 2 + 1;
    tbl_ix = 0;
    dst_ix = 0;
    ctr = 0; // tables agree at freq = 0
    while(dst_ix < 129 && tbl_ix < tbl_limit)
    {
        if(ctr > 0)
        {
            ctr -= cur_sr;
            ++tbl_ix;
        }
        else if(ctr == 0)
        {
            dst[dst_ix] = (*pfn)(eqtbl->db[tbl_ix] - ref);
            ctr = tbl_sr;
            ++dst_ix;
        }
        else { // ctr < 0
            frac = Float64(ctr + cur_sr) / Float64(cur_sr);
            dst[dst_ix] = (*pfn)(qinterp(eqtbl->db, tbl_ix, frac) - ref);
            ctr += tbl_sr;
            ++dst_ix;
        }
    }
    Float64 last_val = dst[dst_ix-1]; // last known value
    while(dst_ix < 129)
        dst[dst_ix++] = last_val;
}


