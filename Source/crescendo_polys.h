// crescendo_polys.h -- Polynomial approximations to the HC curves
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
// ------------------------------------------------------------------------
// Crescendo Nonlinear Compression Curves
// ------------------------------------------------------------------------------
// DBM/RAL 12/06
// weighted order (2,2) rational minimax fits
// {dbmin, p0,p1,p2,q1,q2}
//
// dbmin <= dbpwr <= 0
//
// gdb = (p0 + p1*x + p2*x^2)/(1.0 + q1*x + q2*x^2)
//

// ------------------------------------------------------------------------------
#if USE_ORIGINAL_FITS // prefer most recent MiniMax approx from Mathematica below  DM/RAL 08/22/23
inline Float64 RANGE_REDUCE(Float64 dbpwr, Float64 *pcoffs)
{
  return (1.0 - 2.0*min(0.0, max(dbpwr-100.0, pcoffs[0]))/pcoffs[0]);
}

#if 0 // no need to limit gains on the low end
inline Float64 POST_SCALE(Float64 gdb)
{
  Float64 x = min(gdb, 24.0824);
  return ((x < 16.0) ? x : 16.0 + 0.75*(x-16.0));
}
#else
#define POST_SCALE(gdb)  (gdb)
#endif

inline Float64 APPROX(Float64 dbpwr, Float64 *pcoffs)
{
  Float64 x = RANGE_REDUCE(dbpwr, pcoffs);
  return ((pcoffs[3]*x+pcoffs[2])*x+pcoffs[1])/((pcoffs[5]*x+pcoffs[4])*x+1.0);
}

inline Float64 INTERPOLATE(Float64 dbpwr, Float64 frac, Float64 *pcoffs1, Float64 *pcoffs2)
{
  return ((0.0 == frac) 
	  ? APPROX(dbpwr, pcoffs1)
	  : (1.0-frac)*APPROX(dbpwr, pcoffs1) + frac*APPROX(dbpwr, pcoffs2));
}

// ------------------------------------------------------------------------------
#if GEN_POLY_TABLES
static Float64 gfit00[] = { -80.0,
			   0.0, 0.0, 0.0,
			   0.0, 0.0};
// (2,2) fits from NML
// unrestricted domain fits 20 dBSPL to 100 dBSPL
static Float64 gfit05[] = { -80.000000, 0.012153932845, -0.027972072658, 0.020234172436, 1.106218356485, 0.395151170592};
static Float64 gfit10[] = { -80.000000, 0.050195789141, -0.111617678367, 0.078211292363, 1.136573472741, 0.420641059795};
static Float64 gfit15[] = { -80.000000, 0.161335644299, -0.333041916426, 0.218357161984, 1.181995855308, 0.460606334581};
static Float64 gfit20[] = { -80.000000, 0.395090449522, -0.767315342693, 0.470643549457, 1.189576745240, 0.476954296066};
static Float64 gfit25[] = { -80.000000, 0.729531631616, -1.393947278250, 0.833516862035, 1.146557413417, 0.461619378482};
static Float64 gfit30[] = { -80.000000, 1.168306433997, -2.234778748792, 1.326413323836, 1.078196616528, 0.434322803072};
static Float64 gfit35[] = { -80.000000, 1.756814693160, -3.373932478990, 1.993065280096, 0.992069976176, 0.402011673342};
static Float64 gfit40[] = { -80.000000, 2.555796160098, -4.919994039434, 2.886270086186, 0.887762017845, 0.365618234421};
static Float64 gfit45[] = { -80.000000, 3.634032175130, -6.987899941065, 4.054920582166, 0.764540813057, 0.325257725544};
static Float64 gfit50[] = { -80.000000, 5.061765619606, -9.677313941142, 5.527783196897, 0.624093143413, 0.281860954703};
static Float64 gfit55[] = { -80.000000, 6.900453426629, -13.051866897679, 7.304307850380, 0.470569813788, 0.237132455000};
static Float64 gfit60[] = { -80.000000, 9.192293573511, -17.102019260069, 9.326713532491, 0.311587671121, 0.193715803621};
static Float64 gfit65[] = { -80.000000, 11.949868915007, -21.770617692341, 11.525134322195, 0.153794964781, 0.153476845029};
static Float64 gfit70[] = { -80.000000, 15.155610254241, -26.928113554838, 13.789208033938, 0.004699965880, 0.118201401328};
static Float64 gfit75[] = { -80.000000, 18.765714945524, -32.411640615103, 16.007719527784, -0.130068664218, 0.088779516867};
static Float64 gfit80[] = { -80.000000, 22.719979993526, -38.048453196509, 18.081404185623, -0.246995723140, 0.065333859211};
static Float64 gfit85[] = { -80.000000, 26.952568848659, -43.680698577395, 19.935687509170, -0.344735165018, 0.047387263449};
static Float64 gfit90[] = { -80.000000, 31.400741776793, -49.181772902765, 21.526279829902, -0.423735506702, 0.034112523180};
// ------------------------------------------------------------------------------

Float64* gfits[] = {
  gfit00, gfit05, gfit10, gfit15,
  gfit20, gfit25, gfit30, gfit35,
  gfit40, gfit45, gfit50, gfit55,
  gfit60, gfit65, gfit70, gfit75,
  gfit80, gfit85, gfit90, gfit00 };

#else
extern Float64* gfits[];
#endif // GEN_POLY_TABLES
#endif // USE_ORIGINAL_FITS

// ======================================================================================================================
#if USE_MATHEMATICA_ASONES_FITS
/*
 ;; -------------------------------------------------------------------
 ;; MiniMax Rational Approximations from Mathematica. DM/RAL 08/22/23
 ;;
 ;; Approximation for:
 ;;
 ;;    Gain = 30*Log10(1 + 10^((P - Pthr)/30)) - (P - Pthr)
 ;;
 ;; For elevated threshold Pthr, and P ranging from 0 Phon to 100 Phon.
 ;;
 ;; E.g., For Pthr = 0
 ;;
 ;;              (0.396 - 0.00381*P)
 ;;    Gain = -------------------------
 ;;           (1 - 0.056*P + 0.001*P^2)
 ;;
 ;; for P from 0 phon to 100 phon
 ;;
 ;; These all approximate:   Gain = 30*Log10[1 + 10^((P - Pthr)/30)] - (P - Pthr)
 */
#define GAINPOLYS(pthr, a0, a1, b0, b1, b2, err)  {a0, a1, b1, b2}
//      Pthr          Numerator                                Denominator               Err
//                   A0          A1                 B0        B1            B2
#if GEN_POLY_TABLES
static Float64 gfit_0  [4] = GAINPOLYS(  0,      6.23914,   -0.0596408,            1,   -0.0857069,   0.00734216,     0.309134);
static Float64 gfit_5  [4] = GAINPOLYS(  5,      8.20133,   -0.0783325,            1,   -0.0822086,   0.00667493,     0.303159);
static Float64 gfit_10 [4] = GAINPOLYS( 10,      10.5421,    -0.100583,            1,   -0.0779337,   0.00595444,     0.295672);
static Float64 gfit_15 [4] = GAINPOLYS( 15,      13.2565,    -0.126313,            1,   -0.0729398,   0.00521498,     0.286514);
static Float64 gfit_20 [4] = GAINPOLYS( 20,      16.3302,    -0.155345,            1,   -0.0673569,   0.00449056,     0.275568);
static Float64 gfit_25 [4] = GAINPOLYS( 25,      19.7463,    -0.187469,            1,   -0.0613623,   0.00380899,     0.262769);
static Float64 gfit_30 [4] = GAINPOLYS( 30,        23.49,    -0.222482,            1,    -0.055148,   0.00318887,     0.248124);
static Float64 gfit_35 [4] = GAINPOLYS( 35,      27.5495,    -0.260199,            1,   -0.0488942,   0.00263954,     0.231722);
static Float64 gfit_40 [4] = GAINPOLYS( 40,      31.9151,    -0.300443,            1,   -0.0427541,   0.00216289,     0.213743);
static Float64 gfit_45 [4] = GAINPOLYS( 45,      36.5763,    -0.343017,            1,   -0.0368491,   0.0017558,      0.194455);
static Float64 gfit_50 [4] = GAINPOLYS( 50,      41.5189,    -0.387678,            1,   -0.0312711,   0.00141231,     0.174209);
static Float64 gfit_55 [4] = GAINPOLYS( 55,      46.7225,    -0.434108,            1,   -0.0260877,   0.00112526,     0.153423);
static Float64 gfit_60 [4] = GAINPOLYS( 60,      52.1585,    -0.481904,            1,   -0.0213486,   8.8735E-4,      0.132565);
static Float64 gfit_65 [4] = GAINPOLYS( 65,      57.7897,    -0.530561,            1,   -0.0170897,   6.91756E-4,     0.112136);
static Float64 gfit_70 [4] = GAINPOLYS( 70,      63.5699,    -0.579476,            1,   -0.0133363,   5.32348E-4,     0.0926405);
static Float64 gfit_75 [4] = GAINPOLYS( 75,       69.446,    -0.627954,            1,   -0.0101037,   4.03774E-4,     0.0745605);
static Float64 gfit_80 [4] = GAINPOLYS( 80,      75.3608,    -0.675227,            1,  -0.00739529,   3.01387E-4,     0.0583201);
static Float64 gfit_85 [4] = GAINPOLYS( 85,      81.2576,    -0.720493,            1,  -0.00520012,   2.21128E-4,     0.0442437);
static Float64 gfit_90 [4] = GAINPOLYS( 90,      87.0861,    -0.762966,            1,  -0.00348958,   1.59392E-4,     0.0325162);
static Float64 gfit_95 [4] = GAINPOLYS( 95,       92.809,    -0.801951,            1,  -0.00221609,   1.12918E-4,     0.0231543);
static Float64 gfit_100[4] = GAINPOLYS(100,      98.4055,    -0.836915,            1,   -0.0013155,   7.87346E-5,     0.0160045);

Float64* gfits[] = {
  gfit_0,   gfit_5,   gfit_10,  gfit_15,
  gfit_20,  gfit_25,  gfit_30,  gfit_35,
  gfit_40,  gfit_45,  gfit_50,  gfit_55,
  gfit_60,  gfit_65,  gfit_70,  gfit_75,
  gfit_80,  gfit_85,  gfit_90,  gfit_95,
  gfit_100, gfit_100 }; // intentional duplicate last entry
#else
extern Float64* gfits[];
#endif // GEN_POLY_TABLES

inline Float64 APPROX(Float64 phon, Float64 *pcoffs)
{
    return ((pcoffs[1]*phon + pcoffs[0]) / ((pcoffs[3]*phon + pcoffs[2])*phon + 1.0));
}

inline Float64 INTERPOLATE(Float64 phon, Float64 frac, Float64 *pcoffs1, Float64 *pcoffs2)
{
    if(phon < 0.0)
        phon = 0.0;
    if(phon > 100.0)
        phon = 100.0;
    return ((0.0 == frac)
            ? APPROX(phon, pcoffs1)
            : (1.0-frac)*APPROX(phon, pcoffs1) + frac*APPROX(phon, pcoffs2));
}
#endif // USE_MATHEMATICA_ASONES_FITS

// ========================================================================================================
#if USE_EARSPRING_SONES_FITS
// Based on EarSpring Sones model. DM/RAL 08/22/23
// Fits are (2, 2) rational minimax fits over domain (30, 100) phon, reduced to domain (-1,1).
// Fit domain limits are shown in first two entries.
// Fits have max error of 0.25 dB, sigmoid shape from zero at low threshold
// elevations, rising to max at 100 dB elevation.
//
// Coffs are a0, a1, a2, b1, b2 in (a0 + a1*x + a2*x^2)/(1 + b1*x + b2*x^2)
// for x reduced domain to (-1, 1)
#if GEN_POLY_TABLES
static Float64 gfit05[7] = {30, 100,
 0.008626928604320194, -0.012361093220590867, 0.005932858721804497, 1.2052950393671134, 0.44098406917024136};
static Float64 gfit10[7] = {30, 100,
 0.03541069479177473, -0.051343904011781905, 0.024531720105164176, 1.1996732165939128, 0.43848480426207104};
static Float64 gfit15[7] = {30, 100,
 0.11261442173786807, -0.16475985206072139, 0.07919022688239116, 1.1840306178557647, 0.4317548958876204};
static Float64 gfit20[7] = {30, 100,
 0.2733227034377206, -0.4049476375515006, 0.19700826019254797, 1.1537777691810094, 0.4195210773476687};
static Float64 gfit25[7] = {30, 100,
 0.5040210957405286, -0.7567818018892314, 0.3728468335309268, 1.1143149876321142, 0.40479197679269324};
static Float64 gfit30[7] = {30, 100,
 0.8092139358506897, -1.2308506446347816, 0.6131502360466855, 1.0667727319999556, 0.38826361714624724};
static Float64 gfit35[7] = {30, 100,
 1.2229773584470552, -1.8838919052810313, 0.9471298401484961, 1.0077804871165575, 0.36879354446698887};
static Float64 gfit40[7] = {30, 100,
 1.79308047879481, -2.7958181070847537, 1.415038814559232, 0.9335320861604448, 0.3451577592118398};
static Float64 gfit45[7] = {30, 100,
 2.57814275501546, -4.06349884836368, 2.0630248053133578, 0.8411237025272808, 0.3165962137918315};
static Float64 gfit50[7] = {30, 100,
 3.645300395705553, -5.792215903690983, 2.9354967319892938, 0.7295320129209693, 0.283194170108263};
static Float64 gfit55[7] = {30, 100,
 5.064179554570641, -8.078998954520486, 4.063383171035204, 0.6003058459274823, 0.24603228454433795};
static Float64 gfit60[7] = {30, 100,
 6.896820620509897, -10.989544459599295, 5.451019889816478, 0.45774767804501415, 0.2070401204109164};
static Float64 gfit65[7] = {30, 100,
 9.185830856013208, -14.535845045532227, 7.066940118425121, 0.30840528370320986, 0.16858235193377835};
static Float64 gfit70[7] = {30, 100,
 11.944997472682765, -18.664335655120944, 8.843898392723466, 0.15991708012437322, 0.13291766747779565};
static Float64 gfit75[7] = {30, 100,
 15.156360275770853, -23.26078657265399, 10.689409463510986, 0.019585741628560305, 0.10173763161388333};
static Float64 gfit80[7] = {30, 100,
 18.77496223266342, -28.170087244713834, 12.502630180367865, -0.10681563453737719, 0.07593327093851693};
static Float64 gfit85[7] = {30, 100,
 22.73896160882531, -33.22420367541622, 14.192514526704955, -0.21573137829670286, 0.05560332710443905};
static Float64 gfit90[7] = {30, 100,
 26.98100467535946, -38.2684162880875, 15.691228474507194, -0.30587914655351867, 0.04025245823904644};
static Float64 gfit95[7] = {30, 100,
 31.43736143289638, -43.17982344067738, 16.96056204469926, -0.3778548247235505, 0.029058720843240238};
static Float64 gfit100[7] = {30, 100,
 36.05330123758205, -47.875678698699375, 17.99115542239629, -0.43351769422830455, 0.021113950119056382};

Float64* gfits[] = {
  gfit05,  gfit05,  gfit10,  gfit15, // 0dB borrewd from 5dB fit
  gfit20,  gfit25,  gfit30,  gfit35,
  gfit40,  gfit45,  gfit50,  gfit55,
  gfit60,  gfit65,  gfit70,  gfit75,
  gfit80,  gfit85,  gfit90,  gfit95,
  gfit100, gfit100 }; // intentional duplicate last entry
#else
extern Float64* gfits[];
#endif // GEN_POLY_TABLES

inline Float64 APPROX(Float64 x, Float64 *pcoffs)
{
    return (((pcoffs[4]*x + pcoffs[3])*x + pcoffs[2]) / ((pcoffs[6]*x + pcoffs[5])*x + 1.0));
}

inline Float64 INTERPOLATE(Float64 phon, Float64 frac, Float64 *pcoffs1, Float64 *pcoffs2)
{
    Float64 x;
    if(phon <= pcoffs1[0])
        x = -1.0;
    else if(phon >= pcoffs1[1])
        x = 1.0;
    else
        x = 2.0*(phon - pcoffs1[0])/(pcoffs1[1]-pcoffs1[0]) - 1.0;
    return ((frac < 1.0e-2)
            ? APPROX(x, pcoffs1)
            : (1.0-frac)*APPROX(x, pcoffs1) + frac*APPROX(x, pcoffs2));
}
#endif // USE_EARSPRING_SONES_FITS

// -- end of crescendo_polys.h -- //

