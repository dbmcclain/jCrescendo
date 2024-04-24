/*
 *  tfilter.h
 *  CrescendoPlayThru
 *
 *  Created by David McClain on 10/18/07.
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

#ifndef __TFILTER_H__
#define __TFILTER_H__

#include <math.h>
#include "my_types.h"
#include "smart_ptr.h"
#include "useful_math.h"
#include <Accelerate/Accelerate.h>

#if 0 // Not available on ARM64
typedef union {
    __m128  v;
    float   f[4];
    double  d[2];
} f4vector;

typedef union {
    __m128  v;
    double  d[2];
} d2vector;
#endif

// --------------------------------------------------
class TFilter
{
	Float64 m_coffs[5];
	Float64 m_state[2];
	
public:
	TFilter(Float64 *pcoffs);
    virtual ~TFilter();
	
	Float64 filter(Float64 v);
	void reset();
#if _MATH_DITHER_
    void ftz();
#else
    void ftz()
    {}
#endif
    
	TFilter *clone();
};

// --------------------------------------------------
#if 0  // Not on ARM64
class TStereoFilter
{
    d2vector m_coffs[5];
    d2vector m_state[2];
    
public:
    TStereoFilter(Float64 *pcoffs);
    virtual ~TStereoFilter();
    
    d2vector filter(d2vector v);
    void reset();
#if _MATH_DITHER_
    void ftz();
#else
    void ftz()
    {}
#endif
};
#endif

// --------------------------------------------------
class TOrd4Filter
{
    TPtr<TFilter> m_filter1;
    TPtr<TFilter> m_filter2;
    
public:
    TOrd4Filter(Float64 *pcoffs1, Float64 *pcoffs2);
    
    Float64 filter(Float64 x);
    void reset();
#if _MATH_DITHER_
    void ftz();
#else
    void ftz()
    {}
#endif
};

// --------------------------------------------------
#if 0 // Not on ARM64
class TStereoOrd4Filter
{
    TPtr<TStereoFilter> m_filter1;
    TPtr<TStereoFilter> m_filter2;
    
public:
    TStereoOrd4Filter(Float64 *pcoffs1, Float64 *pcoffs2);
    
    d2vector filter(d2vector x);
    void reset();
#if _MATH_DITHER_
    void ftz();
#else
    void ftz()
    {}
#endif
};
#endif

// --------------------------------------------------
class T1770Filter
{
    TPtr<TOrd4Filter> m_filter;
    
public:
    T1770Filter(Float64 fsamp);
    Float64 filter(Float64 x);
    void reset();
#if _MATH_DITHER_
    void ftz();
#else
    void ftz()
    {}
#endif
};

// --------------------------------------------------
#if 0 // Not on ARM64
class TStereo1770Filter
{
    TPtr<TStereoOrd4Filter> m_filter;
    
public:
    TStereo1770Filter(Float64 fsamp);
    d2vector filter(d2vector x);
    void reset();
#if _MATH_DITHER_
    void ftz();
#else
    void ftz()
    {}
#endif
};
#endif

// --------------------------------------------------
class TBWeightedFilter
{
    TPtr<TFilter> m_filter1;
    TPtr<TFilter> m_filter2;
    TPtr<TFilter> m_filter3;
    
public:
    TBWeightedFilter(Float64 fsamp);
    
    Float64 filter(Float64 x);
    void reset();
#if _MATH_DITHER_
    void ftz();
#else
    void ftz()
    {}
#endif
};

// --------------------------------------------------

extern TFilter *make_bpf_filter(Float64 fsamp, Float64 fc, Float64 q, Float64 gaindB);
extern TFilter *make_hishelf_filter(Float64 fsamp, Float64 fc, Float64 q, Float64 leveldB);
extern TFilter *make_lowshelf_filter(Float64 fsamp, Float64 fc, Float64 q, Float64 leveldB);
extern TFilter *make_lpf_filter(Float64 fsamp, Float64 fc, Float64 q, Float64 gaindB);
extern TFilter *make_hpf_filter(Float64 fsamp, Float64 fc, Float64 q, Float64 gaindB);

#if 0  // Not on ARM64
extern TStereoFilter *make_stereo_bpf_filter(Float64 fsamp, Float64 fc, Float64 q, Float64 gaindB);
extern TStereoFilter *make_stereo_hishelf_filter(Float64 fsamp, Float64 fc, Float64 q, Float64 leveldB);
extern TStereoFilter *make_stereo_lowshelf_filter(Float64 fsamp, Float64 fc, Float64 q, Float64 leveldB);
extern TStereoFilter *make_stereo_lpf_filter(Float64 fsamp, Float64 fc, Float64 q, Float64 gaindB);
extern TStereoFilter *make_stereo_hpf_filter(Float64 fsamp, Float64 fc, Float64 q, Float64 gaindB);

extern TStereoOrd4Filter *make_hdph_iir_eq(Float64 fsamp, Float64 fc1, Float64 q1, Float64 gdb1,
                                           Float64 fc2, Float64 q2, Float64 gdb2);

extern TStereoOrd4Filter *make_symphony_prefilter(Float64 fsamp);
#endif

// ----------------------------------------------------------------------
// Inline helper routines for better readability...

#if 0 // Not on ARM64
inline d2vector stereo(Float64 left, Float64 right)
{
    d2vector ans;
    ans.d[0] = left;
    ans.d[1] = right;
    return ans;
}

inline d2vector mono(Float64 val)
{
    d2vector ans;
    ans.d[0] = ans.d[1] = val;
    return ans;
}

inline d2vector d2v(__m128 v)
{
    d2vector ans;
    ans.v = v;
    return ans;
}

inline Float64 left(d2vector v)
{
    return v.d[0];
}

inline Float64 right(d2vector v)
{
    return v.d[1];
}

inline d2vector d2v_mul(d2vector a, d2vector b)
{
    return d2v(_mm_mul_pd(a.v, b.v));
}

inline d2vector d2v_add(d2vector a, d2vector b)
{
    return d2v(_mm_add_pd(a.v, b.v));
}

inline Float64 d2v_max(d2vector v)
{
    return max(left(v), right(v));
}

inline d2vector d2v_pwr(d2vector v)
{
    return d2v_mul(v, v);
}

inline Float64 d2v_maxpwr(d2vector v)
{
    return d2v_max(d2v_pwr(v));
}
#endif


inline Float64 dbampl20(Float64 xdb)
{
  return pow(10.0, 0.05*xdb);
}

inline Float32 dbampl20f(Float32 xdb)
{
  return powf(10.0f, 0.05f*xdb);
}

#endif // __TFILTER_H__
