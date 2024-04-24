//
//  vTuningParams.h
//  Crescendo-AU
//
//  Created by David McClain on 12/21/12.
//
//
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

#ifndef Crescendo_AU_vTuningParams_h
#define Crescendo_AU_vTuningParams_h

#include "my_types.h"

struct tVTuningParams
{
    UInt32  hdphx_onoff;
    UInt32  proc_onoff;
    // UInt32  corr_only_onoff;
    // UInt32  postEQ;
    // UInt32  headphone;
    UInt32  engine;
    // UInt32  use_rolloff;
    // UInt32  use_symphony_preEQ;
    
    Float32 vTune;
    // Float32 voldB;
    // Float32 attendB;
    // Float32 brighnessdB;
    Float32 Brightness;
    Float32 CaldBSPL;
    Float32 CalLUFS;
    Float32 FSamp;
    Float32 releaseFast;
    Float32 releaseSlow;
    
    Float32 aud[4][12]; // audL, audR, dhL, dhR
    
    Float32 specL[12]; /* out parameters from Crescendo engine */
    Float32 gainL[12]; /* only 11 are used, but we state 12 for alignment purposes */
    Float32 specR[12];
    Float32 gainR[12];
};

// Groups in the aud matrix
#define AUDL  0
#define AUDR  1
#define DHL   2
#define DHR   3

#endif
