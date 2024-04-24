/*
 *  useful_math.h
 *  CrescendoPlayThru
 *
 *  Created by David McClain on 10/25/07.
 *  Copyright 2007 Refined Audiometrics Laboratory. All rights reserved.
 *
 */
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

#ifndef __USEFUL_MATH_H__
#define __USEFUL_MATH_H__

#include <math.h>
#include <fenv.h>
#include "my_types.h"

// -------------------------------------------------------------
// DAZFZ - a class that automatically instantiates the DAZ and FZ flags
// in the x87 environment. It will restore the environment on destruction.
//
// But note that this only prevents exceptions and does nothing for X87 stalls.
// And it has no effect on the X86/64 instruction set.
// So, sadly, we still need to dither for denormals...

class DAZFZ
{
    // fenv_t savenv;
    
public:
    DAZFZ()
    {
        /*
        fegetenv(&savenv);
        fesetenv(FE_DFL_DISABLE_SSE_DENORMS_ENV);
         */
    }
    
    ~DAZFZ()
    { /* fesetenv(&savenv); */ }
};



// -------------------------------------------------------------
//

template<class __T>
inline __T max(__T a, __T b)
{ return (a >= b) ? a : b; }
						
template<class __T>
inline __T min(__T a, __T b)
{ return (a <= b) ? a : b; }

template<class __T>
inline __T abs(__T x)
{ return (x < 0) ? -x : x; }

// for FP equality testing: within(a, b, tol) and beyond(a, b, tol)
template <class __T>
inline bool within(__T a, __T b, __T tol) {
    return abs(a - b) < tol;
}

template <class __T>
inline bool beyond(__T a, __T b, __T tol) {
    return !within(a, b, tol);
}


inline Float64 db10(Float64 x)
{ return (x > 0.0 ? 10.0*log10(x) : -140.0); }

inline Float32 db10f(Float32 x)
{ return (x > 0.0f ? 10.0f*log10f(x) : -140.0f); }

inline Float64 db20(Float64 x)
{ return (x > 0.0 ? 20.0*log10(x) : -140.0); }

inline Float32 db20f(Float32 x)
{ return (x > 0.0f ? 20.0f*log10f(x) : -140.0f); }

inline Float64 ampl20(Float64 xdb)
{ return pow(10.0, 0.05*xdb); }

inline Float32 ampl20f(Float32 xdb)
{ return powf(10.0f, 0.05f*xdb); }

inline Float64 ampl10(Float64 xdb)
{ return pow(10.0, 0.1*xdb); }

inline Float32 ampl10f(Float32 xdb)
{ return powf(10.0f, 0.1f*xdb); }

inline Float64 ftz(Float64 v)
{
#if _MATH_DITHER_
    return (fabs(v) < 1.0e-7 ? 0.0 : v);
#else
    return v;
#endif
}

inline Float32 ftzf(Float32 v)
{
#if _MATH_DITHER_
    return (fabsf(v) < 1.0e-7f ? 0.0f : v);
#else
    return v;
#endif
}

template<class __T>
inline __T sq(__T x)
{ return (x*x); }

//-------------------------------------------------------------------
//
inline void incrmod(UInt32& ctr, UInt32 incr, UInt32 modsiz)
{
  ctr += incr;
  while(ctr >= modsiz)
    ctr -= modsiz;
}

inline void nincrmod(UInt32& ctr, UInt32 incr, UInt32 modsiz)
{
	// faster version for when modsiz is power of 2
	ctr += incr;
	ctr &= (modsiz-1);
}

//-------------------------------------------------------------------
//
extern Float64 e_folding(Float64 tcms, Float64 fsamp);

#ifdef WIN32
inline SInt32 roundf(Float32 x)
{
	if(x < 0.0)
		return((SInt32)(x - 0.5f));
	else
		return((SInt32)(x + 0.5f));
}
#endif

template <class __T>
inline bool within_vec(__T *a, __T *b, UInt32 nel, __T tol) {
    // return true if every element of a is within tol
    // of corresponding element from b
    while(nel-- > 0) {
        if(!within(*a++, *b++, tol))
            return false;
    }
    return true;
}


#endif // __USEFUL_MATH_H__
