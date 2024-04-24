/* note #undef's at end of file */
#define IA 16807
#define IM 2147483647
#define AM (1.0f/IM)
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)
#define EPS 1.2e-7f
#define RNMX (1.0f-EPS)

#include "my_types.h"

inline SInt32 inner(SInt32 v)
{
	SInt32 k = v/IQ;
	v = IA*(v - k*IQ) - IR*k;
	if(v < 0)
		v += IM;
	return v;
}

Float32 ran1(SInt32 *idum)
{
    // returns pseudo-random number [0.0,1.0)
  int j;
  SInt32 v;
  static SInt32 iy=0;
  static SInt32 iv[NTAB];
  Float32 temp;
  
  v = *idum;
  if(v <= 0 || !iy) 
  {
    if (-v < 1) 
		v=1;
    else 
		v = -v;
	for(j = 8; --j >= 0;)
		v = inner(v);
    for (j=NTAB; --j >= 0;) 
	{
		v = inner(v);
        iv[j] = v;
    }
    iy=iv[0];
  }
  *idum = inner(v);
  j=(int)(iy/NDIV);
  iy=iv[j];
  iv[j] = v;
  if((temp = AM*iy) > RNMX) 
	  return RNMX;
  else 
	  return temp;
}
#undef IA
#undef IM
#undef AM
#undef IQ
#undef IR
#undef NTAB
#undef NDIV
#undef EPS
#undef RNMX
