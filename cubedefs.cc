#include "cubedefs.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <string>

#ifdef __x86_64__
#define USE_ASM
//#define ASMCHECK
#endif

#ifdef ASMCHECK
static std::string dumpedges(unsigned long edges) {
    std::ostringstream res;
    res << std::hex << std::setw(15) << edges << "  ";
    for(int i = 63; i >= 0; --i) {
        res << (edges & 1ul << i ? "1" : "0");
        if( i && i % 5 == 0 )
            res << "|";
    }
    return res.str();
}
#endif

const int cubeCornerColors[8][3] = {
  { CBLUE,   CORANGE, CYELLOW }, { CBLUE,  CYELLOW, CRED },
  { CBLUE,   CWHITE,  CORANGE }, { CBLUE,  CRED,    CWHITE },
  { CGREEN,  CYELLOW, CORANGE }, { CGREEN, CRED,    CYELLOW },
  { CGREEN,  CORANGE, CWHITE  }, { CGREEN, CWHITE,  CRED }
};

const int cubeEdgeColors[12][2] = {
  { CBLUE,   CYELLOW },
  { CORANGE, CBLUE   }, { CRED,    CBLUE },
  { CBLUE,   CWHITE  },
  { CYELLOW, CORANGE }, { CYELLOW, CRED },
  { CWHITE,  CORANGE }, { CWHITE,  CRED },
  { CGREEN,  CYELLOW },
  { CORANGE, CGREEN  }, { CRED,    CGREEN },
  { CGREEN,  CWHITE  }
};

const char *rotateDirName(int rd) {
	switch( rd ) {
	case ORANGECW:  return "orange-cw";
	case ORANGE180: return "orange-180";
	case ORANGECCW: return "orange-ccw";
	case REDCW:  return "red-cw";
	case RED180: return "red-180";
	case REDCCW: return "red-ccw";
	case YELLOWCW:  return "yellow-cw";
	case YELLOW180: return "yellow-180";
	case YELLOWCCW: return "yellow-ccw";
	case WHITECW:  return "white-cw";
	case WHITE180: return "white-180";
	case WHITECCW: return "white-ccw";
	case GREENCW:  return "green-cw";
	case GREEN180: return "green-180";
	case GREENCCW: return "green-ccw";
	case BLUECW:  return "blue-cw";
	case BLUE180: return "blue-180";
	case BLUECCW: return "blue-ccw";
	}
	static char buf[20];
	sprintf(buf, "%d", rd);
	return buf;
}

int rotateNameToDir(const char *rotateName) {
    int rd;
    for(rd = 0; rd < RCOUNT; ++rd) {
        const char *dirName = rotateDirName(rd);
        if( !strncmp(rotateName, dirName, strlen(dirName) ) )
            break;
    }
    return rd;
}

int rotateDirReverse(int rd) {
	switch( rd ) {
	case ORANGECW:  return ORANGECCW;
	case ORANGE180: return ORANGE180;
	case ORANGECCW: return ORANGECW;
	case REDCW:  return REDCCW;
	case RED180: return RED180;
	case REDCCW: return REDCW;
	case YELLOWCW:  return YELLOWCCW;
	case YELLOW180: return YELLOW180;
	case YELLOWCCW: return YELLOWCW;
	case WHITECW:  return WHITECCW;
	case WHITE180: return WHITE180;
	case WHITECCW: return WHITECW;
	case GREENCW:  return GREENCCW;
	case GREEN180: return GREEN180;
	case GREENCCW: return GREENCW;
	case BLUECW:  return BLUECCW;
	case BLUE180: return BLUE180;
	case BLUECCW: return BLUECW;
	}
	return RCOUNT;
}

int transformReverse(int idx) {
    switch( idx ) {
        case TD_C0_7_CW: return  TD_C0_7_CCW;
        case TD_C0_7_CCW: return  TD_C0_7_CW;
        case TD_C1_6_CW: return  TD_C1_6_CCW;
        case TD_C1_6_CCW: return  TD_C1_6_CW;
        case TD_C2_5_CW: return  TD_C2_5_CCW;
        case TD_C2_5_CCW: return  TD_C2_5_CW;
        case TD_C3_4_CW: return  TD_C3_4_CCW;
        case TD_C3_4_CCW: return  TD_C3_4_CW;
        case TD_BG_CW: return  TD_BG_CCW;
        case TD_BG_CCW: return TD_BG_CW;
        case TD_YW_CW: return TD_YW_CCW;
        case TD_YW_CCW: return TD_YW_CW;
        case TD_OR_CW: return TD_OR_CCW;
        case TD_OR_CCW: return TD_OR_CW;
    }
    return idx;
}

static const char *transformName(unsigned td) {
    switch( td ) {
    case TD_0: return "0";
    case TD_C0_7_CW: return "c0-7.cw";
    case TD_C0_7_CCW: return "c0-7.ccw";
    case TD_C1_6_CW: return "c1-6.cw";
    case TD_C1_6_CCW: return "c1-6.ccw";
    case TD_C2_5_CW: return "c2-5.cw";
    case TD_C2_5_CCW: return "c2-5.ccw";
    case TD_C3_4_CW: return "c3-4.cw";
    case TD_C3_4_CCW: return "c3-4.ccw";
    case TD_BG_CW: return "bg.cw";
    case TD_BG_180: return "bg.180";
    case TD_BG_CCW: return "bg.ccw";
    case TD_YW_CW: return "yw.cw";
    case TD_YW_180: return "yw.180";
    case TD_YW_CCW: return "yw.ccw";
    case TD_OR_CW: return "or.cw";
    case TD_OR_180: return "or.180";
    case TD_OR_CCW: return "or.ccw";
    case TD_E0_11: return "e0-11";
    case TD_E1_10: return "e1-10";
    case TD_E2_9: return "e2-9";
    case TD_E3_8: return "e3-8";
    case TD_E4_7: return "e4-7";
    case TD_E5_6: return "e5-6";
    }
    return "unknown";
}

static const unsigned char R120[3] = { 1, 2, 0 };
static const unsigned char R240[3] = { 2, 0, 1 };

cubecorners_perm::cubecorners_perm(unsigned corner0perm, unsigned corner1perm,
			unsigned corner2perm, unsigned corner3perm,
			unsigned corner4perm, unsigned corner5perm,
			unsigned corner6perm, unsigned corner7perm)
	: perm(corner0perm | corner1perm << 3 | corner2perm << 6 |
			corner3perm << 9 | corner4perm << 12 | corner5perm << 15 |
			corner6perm << 18 | corner7perm << 21)
{
}

cubecorners_perm cubecorners_perm::compose(cubecorners_perm ccp1, cubecorners_perm ccp2)
{
    cubecorners_perm res;
#ifdef USE_ASM
    unsigned long tmp1;

    asm(// store ccp1 in xmm1
        "pdep %[depPerm], %q[ccp1], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        // store ccp2 in xmm2
        "pdep %[depPerm], %q[ccp2], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm2, %%xmm2\n"
        // permute; result in xmm1
        "vpshufb %%xmm2, %%xmm1, %%xmm1\n"
        // store xmm1 in res
        "vpextrq $0, %%xmm1, %[tmp1]\n"
        "pext %[depPerm], %[tmp1], %q[res]\n"
            : [res]      "=r"  (res.perm),
              [tmp1]     "=&r" (tmp1)
            : [ccp1]     "r"   (ccp1.perm),
              [ccp2]     "r"   (ccp2.perm),
              [depPerm]  "r"   (0x0707070707070707ul)
            : "xmm1", "xmm2"
       );
#ifdef ASMCHECK
    cubecorners_perm chk = res;
    res.perm = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
    for(unsigned i = 0; i < 8; ++i)
        res.perm |= ccp1.getAt(ccp2.getAt(i)) << 3*i;
#endif
#ifdef ASMCHECK
    if( res.perm != chk.perm ) {
        flockfile(stdout);
        printf("corner perm compose mismatch!\n");
        printf("ccp1 = 0x%x;\n", ccp1.perm);
        printf("ccp2 = 0x%x;\n", ccp2.perm);
        printf("exp  = 0x%x;\n", res.perm);
        printf("got  = 0x%x;\n", chk.perm);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
    return res;
}

cubecorners_perm cubecorners_perm::compose3(cubecorners_perm ccp1, cubecorners_perm ccp2,
        cubecorners_perm ccp3)
{
    cubecorners_perm res;
#ifdef USE_ASM
    unsigned long tmp1;

    asm(// store ccp2 in xmm1
        "pdep %[depPerm], %q[ccp2], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        // store ccp3 in xmm2
        "pdep %[depPerm], %q[ccp3], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm2, %%xmm2\n"
        // permute; result in xmm2
        "vpshufb %%xmm2, %%xmm1, %%xmm2\n"
        // store ccp1 in xmm1
        "pdep %[depPerm], %q[ccp1], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        // permute; result in xmm1
        "vpshufb %%xmm2, %%xmm1, %%xmm1\n"
        // store xmm1 in res
        "vpextrq $0, %%xmm1, %[tmp1]\n"
        "pext %[depPerm], %[tmp1], %q[res]\n"
            : [res]      "=r"  (res.perm),
              [tmp1]     "=&r" (tmp1)
            : [ccp1]     "r"   (ccp1.perm),
              [ccp2]     "r"   (ccp2.perm),
              [ccp3]     "r"   (ccp3.perm),
              [depPerm]  "r"   (0x707070707070707ul)
            : "xmm1", "xmm2"
       );
#ifdef ASMCHECK
    cubecorners_perm chk = res;
    res.perm = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
#if 0
    res = compose(compose(ccp1, ccp2), ccp3);
#else
    for(int i = 0; i < 8; ++i) {
        unsigned corner3perm = ccp3.perm >> 3 * i & 0x7;
        unsigned long corner2perm = ccp2.perm >> 3 * corner3perm & 0x7;
        unsigned long corner1perm = ccp1.perm >> 3 * corner2perm & 0x7;
        res.perm |= corner1perm << 3*i;
    }
#endif
#endif
#ifdef ASMCHECK
    if( res.perm != chk.perm ) {
        flockfile(stdout);
        printf("corner perm compose3 mismatch!\n");
        printf("ccp1 = 0x%x;\n", ccp1.perm);
        printf("ccp2 = 0x%x;\n", ccp2.perm);
        printf("ccp3 = 0x%x;\n", ccp3.perm);
        printf("exp  = 0x%x;\n", res.perm);
        printf("got  = 0x%x;\n", chk.perm);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
    return res;
}

cubecorners_perm cubecorners_perm::reverse() const
{
    cubecorners_perm res;
#ifdef USE_ASM
    unsigned long tmp1;
    asm (
        // xmm1 := perm
        "pdep %[depPerm], %q[perm], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        // xmm2 := 3 * perm
        "vpaddb %%xmm1, %%xmm1, %%xmm2\n"
        "vpaddb %%xmm1, %%xmm2, %%xmm2\n"
        // store values 7 6 5 4 3 2 1 0 in xmm1 -> i
        "mov $0x0706050403020100, %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        // ymm1 = i << 3*perm
        "vpmovzxbd %%xmm1, %%ymm1\n"
        "vpmovzxbd %%xmm2, %%ymm2\n"
        "vpsllvd %%ymm2, %%ymm1, %%ymm1\n"

        // perform horizontal OR on ymm1
        "vextracti128 $1, %%ymm1, %%xmm2\n" // move ymm1[255:128] to xmm2[128:0]
        "vpor %%xmm2, %%xmm1, %%xmm1\n"     // ymm1[128:0] | ymm1[255:128]
        "vpshufd $0x4e, %%xmm1, %%xmm2\n"   // move xmm1[128:63] to xmm2[63:0]; 0x4e = 0q1032 (quaternary)
        "vpor %%xmm2, %%xmm1, %%xmm1\n"     // xmm1[63:0] | xmm1[127:64]
        "vphaddd %%xmm1, %%xmm1, %%xmm1\n"  // xmm1[31:0] + xmm1[63:32]
        "vpextrd $0, %%xmm1, %[res]\n"
        "vzeroupper\n"
        : [res]         "=r"    (res),
          [tmp1]        "=&r"   (tmp1)
        : [perm]       "r"      (perm),
          [depPerm]     "rm"    (0x707070707070707ul)
        : "ymm1", "ymm2"
        );
#ifdef ASMCHECK
    cubecorners_perm chk = res;
    res.perm = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
    for(int i = 0; i < 8; ++i)
        res.perm |= i << 3*getAt(i);
#endif
#ifdef ASMCHECK
    if( res.perm != chk.perm ) {
        flockfile(stdout);
        printf("corner perm reverse mismatch!\n");
        printf("this.perm = 0x%o;\n", perm);
        printf("exp.perm  = 0x%o;\n", res.perm);
        printf("got.perm  = 0x%o;\n", chk.perm);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
	return res;
}

unsigned short cubecorners_perm::getPermIdx() const {
    unsigned short res = 0;
    unsigned indexes = 0;
    for(int i = 7; i >= 0; --i) {
        unsigned p = getAt(i) * 4;
        res = (8-i) * res + (indexes >> p & 0xf);
        indexes += 0x11111111 << p;
    }
    return res;
}

cubecorners_perm cubecorners_perm::fromPermIdx(unsigned short idx)
{
    unsigned unused = 0x76543210;
    cubecorners_perm ccp;

    for(unsigned cornerIdx = 8; cornerIdx > 0; --cornerIdx) {
        unsigned p = idx % cornerIdx * 4;
        ccp.setAt(8-cornerIdx, unused >> p & 0xf);
        unsigned m = -1 << p;
        unused = unused & ~m | unused >> 4 & m;
        idx /= cornerIdx;
    }
    return ccp;
}

bool cubecorners_perm::isPermParityOdd() const
{
    bool isSwapsOdd = false;
    unsigned permScanned = 0;
    for(unsigned i = 0; i < 8; ++i) {
        if( permScanned & 1 << i )
            continue;
        permScanned |= 1 << i;
        unsigned p = i;
        while( (p = getAt(p)) != i ) {
            permScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
    return isSwapsOdd;
}

cubecorner_orients::cubecorner_orients(unsigned corner0orient, unsigned corner1orient,
			unsigned corner2orient, unsigned corner3orient,
			unsigned corner4orient, unsigned corner5orient,
			unsigned corner6orient, unsigned corner7orient)
	: orients(corner0orient | corner1orient << 2 | corner2orient << 4 |
			corner3orient << 6 | corner4orient << 8 | corner5orient << 10 |
			corner6orient << 12 | corner7orient << 14)
{
}

cubecorner_orients cubecorner_orients::compose(cubecorner_orients cco1,
            cubecorners_perm ccp2, cubecorner_orients cco2)
{
    cubecorner_orients res;
#ifdef USE_ASM
    unsigned long tmp1;
    asm(
        // store ccp2 in xmm2
        "pdep %[depPerm], %[ccp2], %[tmp1]\n"
        "vmovq %[tmp1], %%xmm2\n"
        // store cco1 in xmm1
        "pdep %[depOrient], %[cco1], %[tmp1]\n"
        "vmovq %[tmp1], %%xmm1\n"
        // permute the cco1; result in xmm1
        "vpshufb %%xmm2, %%xmm1, %%xmm1\n"
        // store cco2 in xmm2
        "pdep %[depOrient], %[cco2], %[tmp1]\n"
        "vmovq %[tmp1], %%xmm2\n"
        // add the orients
        "vpaddb %%xmm2, %%xmm1, %%xmm1\n"
        // calculate modulo 3
        "vmovd %[mod3], %%xmm2\n"
        "vpshufb %%xmm1, %%xmm2, %%xmm2\n"
        // store result
        "vmovq %%xmm2, %[tmp1]\n"
        "pext %[depOrient], %[tmp1], %[tmp1]\n"
        "mov %w[tmp1], %[resOrients]\n"
        "vzeroupper\n"
        : [resOrients]  "=r"  (res.orients),
          [tmp1]        "=&r" (tmp1)
        : [cco1]        "r"   ((unsigned long)cco1.orients),
          [ccp2]        "r"   ((unsigned long)ccp2.get()),
          [cco2]        "r"   ((unsigned long)cco2.orients),
          [depPerm]     "rm"  (0x707070707070707ul),
          [depOrient]   "rm"  (0x303030303030303ul),
          [mod3]        "rm"  (0x100020100)
        : "xmm1", "xmm2"
       );
#ifdef ASMCHECK
    cubecorner_orients chk = res;
    res.orients = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
	static const unsigned char MOD3[] = { 0, 1, 2, 0, 1 };
	for(int i = 0; i < 8; ++i) {
        unsigned cc2Perm = ccp2.get() >> 3*i & 7;
        unsigned cc2Orient = cco2.orients >> 2*i & 3;
        unsigned cco1Orient = cco1.orients >> 2*cc2Perm & 3;
        unsigned resOrient = MOD3[cco1Orient + cc2Orient];
        res.orients |= resOrient << 2 * i;
    }
#endif
#ifdef ASMCHECK
    if( res.orients != chk.orients ) {
        flockfile(stdout);
        printf("corners compose mismatch!\n");
        printf("cco1        = 0x%x;\n", cco1.orients);
        printf("ccp2        = 0x%x;\n", ccp2.get());
        printf("cco2        = 0x%x;\n", cco2.orients);
        printf("exp.orients = 0x%x;\n", res.orients);
        printf("got.orients = 0x%x;\n", chk.orients);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
	return res;
}

cubecorner_orients cubecorner_orients::compose3(cubecorner_orients cco1,
            cubecorners_perm ccp2, cubecorner_orients cco2,
            cubecorners_perm ccp3, cubecorner_orients cco3)
{
    cubecorner_orients res;
#ifdef USE_ASM
    unsigned long tmp1;
    asm(
        // store ccp3 in xmm3
        "pdep %[depPerm], %[ccp3], %[tmp1]\n"
        "vmovq %[tmp1], %%xmm3\n"
        // store cco2 in xmm2
        "pdep %[depOrient], %[cco2], %[tmp1]\n"
        "vmovq %[tmp1], %%xmm2\n"
        // permute the cco2; result in xmm1 (cco2orient)
        "vpshufb %%xmm3, %%xmm2, %%xmm1\n"
        // store cco3 in xmm2
        "pdep %[depOrient], %[cco3], %[tmp1]\n"
        "vmovq %[tmp1], %%xmm2\n"
        // add the orients, result in xmm1 (cco2orient + cco3orient)
        "vpaddb %%xmm2, %%xmm1, %%xmm1\n"
        // store ccp2 in xmm2
        "pdep %[depPerm], %[ccp2], %[tmp1]\n"
        "vmovq %[tmp1], %%xmm2\n"
        // permute the ccp2; result in xmm3 (midperm)
        "vpshufb %%xmm3, %%xmm2, %%xmm3\n"
        // store cco1 in xmm2
        "pdep %[depOrient], %[cco1], %[tmp1]\n"
        "vmovq %[tmp1], %%xmm2\n"
        // permute the cco1; result in xmm2 (cco1orient)
        "vpshufb %%xmm3, %%xmm2, %%xmm2\n"
        // add the orients
        "vpaddb %%xmm2, %%xmm1, %%xmm1\n"
        // calculate modulo 3
        "vmovd %[mod3], %%xmm2\n"
        "vpshufb %%xmm1, %%xmm2, %%xmm2\n"
        // store result
        "vmovq %%xmm2, %[tmp1]\n"
        "pext %[depOrient], %[tmp1], %[tmp1]\n"
        "mov %w[tmp1], %[resOrients]\n"
        "vzeroupper\n"
        : [resOrients]  "=r"  (res.orients),
          [tmp1]        "=&r" (tmp1)
        : [cco1]        "r"   ((unsigned long)cco1.orients),
          [ccp2]        "r"   ((unsigned long)ccp2.get()),
          [cco2]        "r"   ((unsigned long)cco2.orients),
          [ccp3]        "r"   ((unsigned long)ccp3.get()),
          [cco3]        "r"   ((unsigned long)cco3.orients),
          [depPerm]     "rm"  (0x707070707070707ul),
          [depOrient]   "rm"  (0x303030303030303ul),
          [mod3]        "rm"  (0x20100020100ul)
        : "xmm1", "xmm2"
       );
#ifdef ASMCHECK
    cubecorner_orients chk = res;
    res.orients = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
	static const unsigned char MOD3[] = { 0, 1, 2, 0, 1, 2, 0 };
	for(int i = 0; i < 8; ++i) {
        unsigned ccp3Perm = ccp3.get() >> 3*i & 7;
        unsigned cco3orient = cco3.orients >> 2*i & 3;
        unsigned midperm = ccp2.get() >> 3*ccp3Perm & 7;
        unsigned cco2orient = cco2.orients >> 2*ccp3Perm & 3;
        unsigned cco1orient = cco1.orients >> 2*midperm & 3;
        unsigned resorient = MOD3[cco1orient + cco2orient + cco3orient];
        res.orients |= resorient << 2 * i;
    }
#endif
#ifdef ASMCHECK
    if( res.orients != chk.orients ) {
        flockfile(stdout);
        printf("corner orients compose3 mismatch!\n");
        printf("cco1        = 0x%x;\n", cco1.orients);
        printf("ccp2        = 0x%x;\n", ccp2.get());
        printf("cco2        = 0x%x;\n", cco2.orients);
        printf("ccp3        = 0x%x;\n", ccp3.get());
        printf("cco3        = 0x%x;\n", cco3.orients);
        printf("exp.orients = 0x%x;\n", res.orients);
        printf("got.orients = 0x%x;\n", chk.orients);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
	return res;
}

cubecorner_orients cubecorner_orients::reverse(cubecorners_perm ccp) const {
    unsigned short revOrients = (orients & 0xaaaa) >> 1 | (orients & 0x5555) << 1;
    cubecorner_orients res;
#ifdef USE_ASM
    unsigned long tmp1;
    asm (
        // xmm2[63:0] := 2*perm
        "pdep %[depPermx2], %q[perm], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm2, %%xmm2\n"
        // xmm1[63:0] := revOrients
        "pdep %[depOrient], %q[revOrients], %[tmp1]\n"
        "vmovq %[tmp1], %%xmm1\n"
        // ymm1 = revOrients << 2*perm
        "vpmovzxbd %%xmm1, %%ymm1\n"        // expand to 32 bytes
        "vpmovzxbd %%xmm2, %%ymm2\n"
        "vpsllvd %%ymm2, %%ymm1, %%ymm1\n"  // shift

        // perform horizontal OR on ymm1
        "vextracti128 $1, %%ymm1, %%xmm2\n" // move ymm1[255:128] to xmm2[128:0]
        "vpor %%xmm2, %%xmm1, %%xmm1\n"     // ymm1[128:0] | ymm1[255:128]
        "vpshufd $0x4e, %%xmm1, %%xmm2\n"   // move xmm1[128:63] to xmm2[63:0]; 0x4e = 0q1032 (quaternary)
        "vpor %%xmm2, %%xmm1, %%xmm1\n"     // xmm1[63:0] | xmm1[127:64]
        "vphaddd %%xmm1, %%xmm1, %%xmm1\n"  // xmm1[31:0] + xmm1[63:32]
        "vpextrw $0, %%xmm1, %q[res]\n"
        : [res]         "=r"    (res),
          [tmp1]        "=&r"   (tmp1)
        : [perm]        "r"     (ccp.get()),
          [revOrients]  "r"     (revOrients),
          [depPermx2]   "rm"    (0xe0e0e0e0e0e0e0eul),
          [depOrient]   "rm"    (0x303030303030303ul)
        : "ymm1", "ymm2"
        );
#ifdef ASMCHECK
    cubecorner_orients chk = res;
    res.orients = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
    for(int i = 0; i < 8; ++i)
        res.orients |= (revOrients >> 2*i & 3) << 2 * ccp.getAt(i);
#endif
#ifdef ASMCHECK
    if( res.orients != chk.orients ) {
        flockfile(stdout);
        printf("corner orients reverse mismatch!\n");
        printf("this.orients = 0x%o;\n", orients);
        printf("exp.orients  = 0x%o;\n", res.orients);
        printf("got.orients  = 0x%o;\n", chk.orients);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
	return res;
}

unsigned short cubecorner_orients::getOrientIdx() const
{
	unsigned short res = 0;
	for(unsigned i = 0; i < 7; ++i)
		res = res * 3 + getAt(i);
	return res;
}

cubecorner_orients cubecorner_orients::fromOrientIdx(unsigned short idx)
{
    cubecorner_orients res;
    int sum = 0;
    for(unsigned i = 0; i < 7; ++i) {
        unsigned short val = idx % 3;
        idx /= 3;
        res.setAt(6-i, val);
        sum += val;
    }
    res.setAt(7, (15-sum) % 3);
    return res;
}

bool cubecorner_orients::isBGspace() const {
    return orients == 0;
}

bool cubecorner_orients::isYWspace(cubecorners_perm ccp) const {
    const unsigned orients0356[8] = { 0, 1, 1, 0, 1, 0, 0, 1 };
    const unsigned orients1247[8] = { 2, 0, 0, 2, 0, 2, 2, 0 };
    for(unsigned i = 0; i < 8; ++i) {
        switch(ccp.getAt(i)) {
            case 0:
            case 3:
            case 5:
            case 6:
                if( getAt(i) != orients0356[i] )
                    return false;
                break;
            case 1:
            case 2:
            case 4:
            case 7:
                if( getAt(i) != orients1247[i] )
                    return false;
                break;
        }
    }
    return true;
}

bool cubecorner_orients::isORspace(cubecorners_perm ccp) const {
    const unsigned orients0356[8] = { 0, 2, 2, 0, 2, 0, 0, 2 };
    const unsigned orients1247[8] = { 1, 0, 0, 1, 0, 1, 1, 0 };
    for(unsigned i = 0; i < 8; ++i) {
        switch(ccp.getAt(i)) {
            case 0:
            case 3:
            case 5:
            case 6:
                if( getAt(i) != orients0356[i] )
                    return false;
                break;
            case 1:
            case 2:
            case 4:
            case 7:
                if( getAt(i) != orients1247[i] )
                    return false;
                break;
        }
    }
    return true;
}

cubecorner_orients cubecorner_orients::representativeBG(cubecorners_perm ccp) const
{
    cubecorner_orients orepr;
#ifdef USE_ASM
    unsigned long tmp1;
    asm (
        // xmm2[63:0] := 2*perm
        "pdep %[depPermx2], %q[perm], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm2, %%xmm2\n"
        // xmm1[63:0] := cco
        "pdep %[depOrient], %q[cco], %[tmp1]\n"
        "vmovq %[tmp1], %%xmm1\n"
        // ymm1 = cco << 2*perm
        "vpmovzxbd %%xmm1, %%ymm1\n"        // expand to 32 bytes
        "vpmovzxbd %%xmm2, %%ymm2\n"
        "vpsllvd %%ymm2, %%ymm1, %%ymm1\n"  // shift

        // perform horizontal OR on ymm1
        "vextracti128 $1, %%ymm1, %%xmm2\n" // move ymm1[255:128] to xmm2[128:0]
        "vpor %%xmm2, %%xmm1, %%xmm1\n"     // ymm1[128:0] | ymm1[255:128]
        "vpshufd $0x4e, %%xmm1, %%xmm2\n"   // move xmm1[128:63] to xmm2[63:0]; 0x4e = 0q1032 (quaternary)
        "vpor %%xmm2, %%xmm1, %%xmm1\n"     // xmm1[63:0] | xmm1[127:64]
        "vphaddd %%xmm1, %%xmm1, %%xmm1\n"  // xmm1[31:0] + xmm1[63:32]
        "vpextrw $0, %%xmm1, %q[orepr]\n"
        : [orepr]       "=r"    (orepr.orients),
          [tmp1]        "=&r"   (tmp1)
        : [perm]        "r"     (ccp.get()),
          [cco]         "r"     (this->orients),
          [depPermx2]   "rm"    (0xe0e0e0e0e0e0e0eul),
          [depOrient]   "rm"    (0x303030303030303ul)
        : "ymm1", "ymm2"
        );
#ifdef ASMCHECK
    cubecorner_orients chk = orepr;
    orepr.orients = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
    for(unsigned i = 0; i < 8; ++i)
        orepr.orients |= (orients >> 2*i & 3) << 2*ccp.getAt(i);
#endif
#ifdef ASMCHECK
    if( orepr.orients != chk.orients ) {
        flockfile(stdout);
        printf("corner orients representativeBG mismatch!\n");
        printf("this.orients = 0x%o;\n", orients);
        printf("exp.orients  = 0x%o;\n", orepr.orients);
        printf("got.orients  = 0x%o;\n", chk.orients);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
    return orepr;
}

cubecorner_orients cubecorner_orients::representativeYW(cubecorners_perm ccp) const
{
    unsigned oadd[8] = { 2, 1, 1, 2, 1, 2, 2, 1 };
    cubecorner_orients orepr;
    cubecorners_perm ccpRev = ccp.reverse();
    for(unsigned i = 0; i < 8; ++i) {
        unsigned toAdd = oadd[i] == oadd[ccpRev.getAt(i)] ? 0 : oadd[i];
        orepr.setAt(i, (getAt(ccpRev.getAt(i))+toAdd) % 3);
    }
    return orepr;
}

cubecorner_orients cubecorner_orients::representativeOR(cubecorners_perm ccp) const
{
    unsigned oadd[8] = { 1, 2, 2, 1, 2, 1, 1, 2 };
    cubecorner_orients orepr;
    cubecorners_perm ccpRev = ccp.reverse();
    for(unsigned i = 0; i < 8; ++i) {
        unsigned toAdd = oadd[i] == oadd[ccpRev.getAt(i)] ? 0 : oadd[i];
        orepr.setAt(i, (getAt(ccpRev.getAt(i))+toAdd) % 3);
    }
    return orepr;
}

struct cubecorners {
	cubecorners_perm perm;
	cubecorner_orients orients;

    cubecorners(unsigned corner0perm, unsigned corner0orient,
            unsigned corner1perm, unsigned corner1orient,
            unsigned corner2perm, unsigned corner2orient,
            unsigned corner3perm, unsigned corner3orient,
            unsigned corner4perm, unsigned corner4orient,
            unsigned corner5perm, unsigned corner5orient,
            unsigned corner6perm, unsigned corner6orient,
            unsigned corner7perm, unsigned corner7orient)
        : perm(corner0perm, corner1perm, corner2perm, corner3perm, corner4perm, corner5perm,
                corner6perm, corner7perm),
        orients(corner0orient, corner1orient, corner2orient,
                corner3orient, corner4orient, corner5orient, corner6orient, corner7orient)
        {
        }
};

cubeedges::cubeedges(
        unsigned edge0perm, unsigned edge1perm, unsigned edge2perm,
        unsigned edge3perm, unsigned edge4perm, unsigned edge5perm,
        unsigned edge6perm, unsigned edge7perm, unsigned edge8perm,
        unsigned edge9perm, unsigned edge10perm,unsigned edge11perm,
        unsigned edge0orient, unsigned edge1orient, unsigned edge2orient,
        unsigned edge3orient, unsigned edge4orient, unsigned edge5orient,
        unsigned edge6orient, unsigned edge7orient, unsigned edge8orient,
        unsigned edge9orient, unsigned edge10orient, unsigned edge11orient)
	: edges((unsigned long)edge0perm        | (unsigned long)edge0orient << 4 |
			(unsigned long)edge1perm << 5   | (unsigned long)edge1orient << 9 |
			(unsigned long)edge2perm << 10  | (unsigned long)edge2orient << 14 |
			(unsigned long)edge3perm << 15  | (unsigned long)edge3orient << 19 |
			(unsigned long)edge4perm << 20  | (unsigned long)edge4orient << 24 |
			(unsigned long)edge5perm << 25  | (unsigned long)edge5orient << 29 |
			(unsigned long)edge6perm << 30  | (unsigned long)edge6orient << 34 |
			(unsigned long)edge7perm << 35  | (unsigned long)edge7orient << 39 |
			(unsigned long)edge8perm << 40  | (unsigned long)edge8orient << 44 |
			(unsigned long)edge9perm << 45  | (unsigned long)edge9orient << 49 |
			(unsigned long)edge10perm << 50 | (unsigned long)edge10orient << 54 |
			(unsigned long)edge11perm << 55 | (unsigned long)edge11orient << 59)
{
}

cubeedges cubeedges::compose(cubeedges ce1, cubeedges ce2)
{
    cubeedges res;
#ifdef USE_ASM
    unsigned long tmp1;

    // needs: bmi2 (pext, pdep), sse3 (pshufb), sse4.1 (pinsrq, pextrq)
    asm(
        // store 0xf in xmm3
        "mov $0xf, %[tmp1]\n"
        "vmovq %[tmp1], %%xmm3\n"
        "vpbroadcastb %%xmm3, %%xmm3\n"

        // store ce2 in xmm0
        "pdep %[depItem], %[ce2], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm0, %%xmm0\n"
        "mov %[ce2], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm0, %%xmm0\n"

        // store ce1 in xmm1
        "pdep %[depItem], %[ce1], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        "mov %[ce1], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm1, %%xmm1\n"

        // store ce2 perm in xmm2
        "vpand %%xmm0, %%xmm3, %%xmm2\n"
        // permute; result in xmm1
        "vpshufb %%xmm2, %%xmm1, %%xmm1\n"
        // store ce2 orient in xmm2
        "vpandn %%xmm0, %%xmm3, %%xmm2\n"
        // xor the orient
        "vpxor %%xmm2, %%xmm1, %%xmm1\n"

        // store xmm1 in res
        "vpextrq $0, %%xmm1, %[res]\n"
        "pext %[depItem], %[res], %[res]\n"
        "vpextrd $2, %%xmm1, %k[tmp1]\n"
        "pext %[depItem], %[tmp1], %[tmp1]\n"
        "shl $40, %[tmp1]\n"
        "or %[tmp1], %[res]\n"
            : [res]      "=&r" (res.edges),
              [tmp1]     "=&r" (tmp1)
            : [ce1]      "r"   (ce1.edges),
              [ce2]      "r"   (ce2.edges),
              [depItem]  "rm"  (0x1f1f1f1f1f1f1f1ful)
            : "xmm0", "xmm1", "xmm2", "xmm3", "cc"
       );
#ifdef ASMCHECK
    cubeedges chk = res;
    res.edges = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
    for(int i = 0; i < 12; ++i) {
        unsigned char edge2perm = ce2.edges >> 5 * i & 0xf;
        unsigned long edge1item = ce1.edges >> 5 * edge2perm & 0x1f;
        res.edges |= edge1item << 5*i;
    }
    unsigned long edge2orients = ce2.edges & 0x842108421084210ul;
    res.edges ^= edge2orients;
#endif
#ifdef ASMCHECK
    if( res.edges != chk.edges ) {
        flockfile(stdout);
        printf("edges compose mismatch!\n");
        printf("ce1.edges = 0x%lx;\n", ce1.edges);
        printf("ce2.edges = 0x%lx;\n", ce2.edges);
        printf("exp.edges = 0x%lx;\n", res.edges);
        printf("got.edges = 0x%lx;\n", chk.edges);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
    return res;
}

cubeedges cubeedges::compose3(cubeedges ce1, cubeedges ce2, cubeedges ce3)
{
	cubeedges res;
#ifdef USE_ASM
    unsigned long tmp1;

    // needs: bmi2 (pext, pdep), sse3 (pshufb), sse4.1 (pinsrq, pextrq)
    asm(
        // store 0xf in xmm3
        "mov $0xf, %[tmp1]\n"
        "vmovq %[tmp1], %%xmm3\n"
        "vpbroadcastb %%xmm3, %%xmm3\n"
        // store ce2 in xmm0
        "pdep %[depItem], %[ce2], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm0, %%xmm0\n"
        "mov %[ce2], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm0, %%xmm0\n"
        // store ce2 perm in xmm2
        "vpand %%xmm0, %%xmm3, %%xmm2\n"
        // store ce1 in xmm1
        "pdep %[depItem], %[ce1], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        "mov %[ce1], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm1, %%xmm1\n"
        // permute; result in xmm1
        "vpshufb %%xmm2, %%xmm1, %%xmm1\n"
        // store ce2 orient im xmm2
        "vpandn %%xmm0, %%xmm3, %%xmm2\n"
        // xor the orient
        "vpxor %%xmm2, %%xmm1, %%xmm1\n"
        // store ce3 in xmm0
        "pdep %[depItem], %[ce3], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm0, %%xmm0\n"
        "mov %[ce3], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm0, %%xmm0\n"
        // store ce3 perm in xmm2
        "vpand %%xmm3, %%xmm0, %%xmm2\n"
        // permute; result in xmm1
        "vpshufb %%xmm2, %%xmm1, %%xmm1\n"
        // store ce3 orient in xmm2
        "vpandn %%xmm0, %%xmm3, %%xmm2\n"
        // xor the orient
        "vpxor %%xmm2, %%xmm1, %%xmm1\n"
        // store xmm1 in res
        "vpextrq $0, %%xmm1, %[tmp1]\n"
        "pext %[depItem], %[tmp1], %[tmp1]\n"
        "vpextrd $2, %%xmm1, %k[res]\n"
        "pext %[depItem], %[res], %[res]\n"
        "shl $40, %[res]\n"
        "or %[tmp1], %[res]\n"
            : [res]         "=&r"  (res.edges),
              [tmp1]        "=&r" (tmp1)
            : [ce1]         "r"   (ce1.edges),
              [ce2]         "r"   (ce2.edges),
              [ce3]         "r"   (ce3.edges),
              [depItem]     "rm"  (0x1f1f1f1f1f1f1f1ful)
            : "xmm0", "xmm1", "xmm2", "xmm3", "cc"
       );
#ifdef ASMCHECK
    cubeedges chk = res;
    res.edges = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
    //res = compose(compose(ce1, ce2), ce3);
    for(int i = 0; i < 12; ++i) {
        unsigned edge3perm = ce3.edges >> 5 * i & 0xf;
        unsigned long edge2item = ce2.edges >> 5 * edge3perm;
        unsigned edge2perm = edge2item & 0xf;
        unsigned edge2orient = edge2item & 0x10;
        unsigned long edge1item = ce1.edges >> 5 * edge2perm & 0x1f;
        unsigned long edgemitem = edge1item^edge2orient;
        res.edges |= edgemitem << 5*i;
    }
    unsigned long edge3orients = ce3.edges & 0x842108421084210ul;
    res.edges ^= edge3orients;
#endif
#ifdef ASMCHECK
    if( res.edges != chk.edges ) {
        flockfile(stdout);
        printf("edges compose3 mismatch!\n");
        printf("ce1.edges = 0x%lx;\n", ce1.edges);
        printf("ce2.edges = 0x%lx;\n", ce2.edges);
        printf("exp.edges = 0x%lx;\n", res.edges);
        printf("got.edges = 0x%lx;\n", chk.edges);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
	return res;
}

cubeedges cubeedges::compose3revmid(cubeedges ce1, cubeedges ce2, cubeedges ce3)
{
	cubeedges res;
#ifdef USE_ASM
    unsigned long tmp1;

    asm(
        // store values b a 9 8 7 6 5 4 3 2 1 0 in xmm1
        "mov $0x0706050403020100, %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        "mov $0xb0a0908, %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm1, %%xmm1\n"

        // xmm2 := ce2
        "pdep %[depItem], %[ce2], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm2, %%xmm2\n"
        "mov %[ce2], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm2, %%xmm2\n"

        // xmm3 := ce2 & 0xf
        "mov $0xf, %[tmp1]\n"
        "movq %[tmp1], %%xmm3\n"
        "vpbroadcastb %%xmm3, %%xmm3\n"
        "vpand %%xmm3, %%xmm2, %%xmm3\n"

        // blend together the reversed cycles of length 1 .. 12

        "vpshufb %%xmm3, %%xmm3, %%xmm4\n"              // xmm4 := 2x shuffled edges
        "vpshufb %%xmm4, %%xmm4, %%xmm5\n"              // 4x
        "vpshufb %%xmm4, %%xmm5, %%xmm0\n"              // 6x
        "vpshufb %%xmm3, %%xmm0, %%xmm5\n"              // 7x

        "vpshufb %%xmm3, %%xmm5, %%xmm4\n"              // 8x
        "vpcmpeqb %%xmm1, %%xmm4, %%xmm6\n"
        "vpblendvb %%xmm6, %%xmm5, %%xmm0, %%xmm8\n"

        "vpshufb %%xmm3, %%xmm4, %%xmm5\n"              // 9x
        "vpcmpeqb %%xmm1, %%xmm5, %%xmm6\n"
        "vpblendvb %%xmm6, %%xmm4, %%xmm8, %%xmm8\n"

        "vpshufb %%xmm3, %%xmm5, %%xmm4\n"              // 10x
        "vpcmpeqb %%xmm1, %%xmm4, %%xmm6\n"
        "vpblendvb %%xmm6, %%xmm5, %%xmm8, %%xmm8\n"

        "vpshufb %%xmm3, %%xmm4, %%xmm5\n"              // 11x
        "vpcmpeqb %%xmm1, %%xmm5, %%xmm6\n"
        "vpblendvb %%xmm6, %%xmm4, %%xmm8, %%xmm8\n"

        "vpshufb %%xmm3, %%xmm5, %%xmm4\n"              // 12x
        "vpcmpeqb %%xmm1, %%xmm4, %%xmm6\n"
        "vpblendvb %%xmm6, %%xmm5, %%xmm8, %%xmm8\n"

        // store the reversed ce2 in xmm0
        "vpshufb %%xmm8, %%xmm2, %%xmm0\n"
        "vpxor %%xmm1, %%xmm0, %%xmm0\n"                // retrieve orients
        "vpxor %%xmm8, %%xmm0, %%xmm0\n"                // add orients to perm

        // store 0xf in xmm3
        "mov $0xf, %[tmp1]\n"
        "vmovq %[tmp1], %%xmm3\n"
        "vpbroadcastb %%xmm3, %%xmm3\n"

        // store ce2 perm in xmm2
        "vpand %%xmm0, %%xmm3, %%xmm2\n"
        // store ce1 in xmm1
        "pdep %[depItem], %[ce1], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        "mov %[ce1], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm1, %%xmm1\n"
        // permute; result in xmm1
        "vpshufb %%xmm2, %%xmm1, %%xmm1\n"
        // store ce2 orient im xmm2
        "vpandn %%xmm0, %%xmm3, %%xmm2\n"
        // xor the orient
        "vpxor %%xmm2, %%xmm1, %%xmm1\n"
        // store ce3 in xmm0
        "pdep %[depItem], %[ce3], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm0, %%xmm0\n"
        "mov %[ce3], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm0, %%xmm0\n"
        // store ce3 perm in xmm2
        "vpand %%xmm3, %%xmm0, %%xmm2\n"
        // permute; result in xmm1
        "vpshufb %%xmm2, %%xmm1, %%xmm1\n"
        // store ce3 orient in xmm2
        "vpandn %%xmm0, %%xmm3, %%xmm2\n"
        // xor the orient
        "vpxor %%xmm2, %%xmm1, %%xmm1\n"
        // store xmm1 in res
        "vpextrq $0, %%xmm1, %[tmp1]\n"
        "pext %[depItem], %[tmp1], %[tmp1]\n"
        "vpextrd $2, %%xmm1, %k[res]\n"
        "pext %[depItem], %[res], %[res]\n"
        "shl $40, %[res]\n"
        "or %[tmp1], %[res]\n"
        "vzeroupper\n"
            : [res]         "=&r"  (res.edges),
              [tmp1]        "=&r" (tmp1)
            : [ce1]         "r"   (ce1.edges),
              [ce2]         "r"   (ce2.edges),
              [ce3]         "r"   (ce3.edges),
              [depItem]     "rm"  (0x1f1f1f1f1f1f1f1ful)
            : "xmm0", "xmm1", "xmm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "cc"
       );
#ifdef ASMCHECK
    cubeedges chk = res;
    res.edges = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
    unsigned char ce2rperm[12], ce2rorient[12];
    for(unsigned i = 0; i < 12; ++i) {
        unsigned long edge2item = ce2.edges >> 5*i;
        unsigned long edge2perm = edge2item & 0xf;
        ce2rperm[edge2perm] = i;
        ce2rorient[edge2perm] = edge2item & 0x10;
    }
    for(int i = 0; i < 12; ++i) {
        unsigned edge3perm = ce3.edges >> 5 * i & 0xf;
        unsigned long edge1item = ce1.edges >> 5 * ce2rperm[edge3perm] & 0x1f;
        unsigned long edgemitem = edge1item^ce2rorient[edge3perm];
        res.edges |= edgemitem << 5*i;
    }
    unsigned long edge3orients = ce3.edges & 0x842108421084210ul;
    res.edges ^= edge3orients;
#endif
#ifdef ASMCHECK
    if( res.edges != chk.edges ) {
        flockfile(stdout);
        std::cout << "edges compose3revmid mismatch!\n" << std::endl;
        std::cout << "ce1.edges = " << dumpedges(ce1.edges) << std::endl;
        std::cout << "ce2.edges = " << dumpedges(ce2.edges) << std::endl;
        std::cout << "ce3.edges = " << dumpedges(ce3.edges) << std::endl;
        std::cout << "exp.edges = " << dumpedges(res.edges) << std::endl;
        std::cout << "got.edges = " << dumpedges(chk.edges) << std::endl;
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
	return res;
}

cubeedges cubeedges::reverse() const {
    cubeedges res;
#ifdef USE_ASM
    unsigned long tmp1;
    asm (
        // xmm1 := 1
        "mov $1, %[tmp1]\n"
        "vmovq %[tmp1], %%xmm1\n"
        "vpbroadcastb %%xmm1, %%xmm1\n"

        // xmm4 := edgeItem
        "pdep %[depItem], %[edges], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm4, %%xmm4\n"
        "mov %[edges], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm4, %%xmm4\n"

        // xmm5 := 16
        "vpsllw $4, %%xmm1, %%xmm5\n"
        // xmm3 := edgeItem & 0x10
        "vpand %%xmm4, %%xmm5, %%xmm3\n"

        // xmm5 := 15
        "vpsubb %%xmm1, %%xmm5, %%xmm5\n"

        // store values b a 9 8 7 6 5 4 3 2 1 0 in xmm1 -> i
        "mov $0x0706050403020100, %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        "mov $0xb0a0908, %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm1, %%xmm1\n"

        // xmm1 := edgeItem & 0x10 | i
        "vpor %%xmm3, %%xmm1, %%xmm1\n"

        // xmm3 := edgeItem & 0xf
        "vpand %%xmm4, %%xmm5, %%xmm3\n"
        // xmm2 := 5 * (edgeItem & 0xf)
        "vpsllw $2, %%xmm3, %%xmm2\n"
        "vpaddb %%xmm3, %%xmm2, %%xmm2\n"

        // ymm5 = (edgeItem & 0x10 | i) <<  5*(edgeItem & 0xf) : i=0,1,2,3
        "vpmovzxbq %%xmm1, %%ymm3\n"
        "vpmovzxbq %%xmm2, %%ymm4\n"
        "vpsllvq %%ymm4, %%ymm3, %%ymm5\n"

        // ymm5 |= (edgeItem & 0x10 | i) <<  5*(edgeItem & 0xf) : i=4,5,6,7
        "vpsrldq $4, %%xmm1, %%xmm3\n"
        "vpmovzxbq %%xmm3, %%ymm3\n"
        "vpsrldq $4, %%xmm2, %%xmm4\n"
        "vpmovzxbq %%xmm4, %%ymm4\n"
        "vpsllvq %%ymm4, %%ymm3, %%ymm6\n"
        "vpor %%ymm6, %%ymm5, %%ymm5\n"

        // ymm5 |= (edgeItem & 0x10 | i) <<  5*(edgeItem & 0xf) : i=8,9,10,11
        "vpsrldq $8, %%xmm1, %%xmm3\n"
        "vpmovzxbq %%xmm3, %%ymm3\n"
        "vpsrldq $8, %%xmm2, %%xmm4\n"
        "vpmovzxbq %%xmm4, %%ymm4\n"
        "vpsllvq %%ymm4, %%ymm3, %%ymm6\n"
        "vpor %%ymm6, %%ymm5, %%ymm5\n"

        // perform horizontal OR on ymm5
        "vextracti128 $1, %%ymm5, %%xmm6\n"
        "vpor %%xmm6, %%xmm5, %%xmm5\n"
        "vpshufd $0x4e, %%xmm5, %%xmm6\n"
        "vpor %%xmm6, %%xmm5, %%xmm5\n"
        "vpextrq $0, %%xmm5, %[res]\n"
        "vzeroupper\n"
        : [res]         "=r"    (res),
          [tmp1]        "=&r"   (tmp1)
        : [edges]       "r"     (edges),
          [depItem]     "rm"    (0x1f1f1f1f1f1f1f1ful)
        : "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "cc"
        );
#ifdef ASMCHECK
    cubeedges chk = res;
    res.edges = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
    for(unsigned i = 0; i < 12; ++i) {
        unsigned long edgeItem = edges >> 5*i;
        res.edges |= (edgeItem & 0x10 | i) << 5*(edgeItem & 0xf);
    }
#endif
#ifdef ASMCHECK
    if( res.edges != chk.edges ) {
        flockfile(stdout);
        printf("edges reverse mismatch!\n");
        printf("    edges = 0x%lx;\n", edges);
        printf("exp.edges = 0x%lx;\n", res.edges);
        printf("got.edges = 0x%lx;\n", chk.edges);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
    return res;
}

unsigned cubeedges::getPermIdx() const {
    unsigned res = 0;
    unsigned long indexes = 0;
    unsigned long e = edges << 2;
    unsigned shift = 60;
    for(unsigned i = 1; i <= 12; ++i) {
        shift -= 5;
        unsigned p = e >> shift & 0x3c;
        res = i * res + (indexes >> p & 0xf);
        indexes += 0x111111111111ul << p;
    }
    return res;
}

unsigned short cubeedges::getOrientIdx() const {
    unsigned short res;
#ifdef USE_ASM
    asm("pext %[orientIdxMask], %[edges], %q[res]\n"
            : [res]             "=r"  (res)
            : [orientIdxMask]   "rm"  (0x42108421084210ul),
              [edges]           "r"   (edges)
       );
#ifdef ASMCHECK
    unsigned short chk = res;
    res = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
    unsigned long orients = (edges & 0x42108421084210ul) >> 4;
    // orients == k0000j0000i0000h0000g0000f0000e0000d0000c0000b0000a
    orients |= orients >> 4;
    // orients == k000kj000ji000ih000hg000gf000fe000ed000dc000cb000ba
    orients |= orients >> 8;
    // orients == k000kj00kji0kjih0jihg0ihgf0hgfe0gfed0fedc0edcb0dcba
    orients &= 0x70000f0000ful;
    // orients ==         kji0000000000000000hgfe0000000000000000dcba
    res = orients | orients >> 16 | orients >> 32;
    // res == kjihgfedcba   (bit over 16 are cut off due to res variable type)
#endif
#ifdef ASMCHECK
    if( res != chk ) {
        flockfile(stdout);
        std::cout << "edges get orient mismatch!" << std::endl;
        std::cout << "edges = " << dumpedges(edges) << std::endl;
        std::cout << "exp   = 0x" << std::hex << res << std::endl;
        std::cout << "got   = 0x" << chk << std::endl;
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
	return res;
}

cubeedges cubeedges::fromPermAndOrientIdx(unsigned permIdx, unsigned short orientIdx)
{
    unsigned long unused = 0xba9876543210ul;
    cubeedges ce;
    unsigned orient11 = 0;

    for(unsigned edgeIdx = 12; edgeIdx > 0; --edgeIdx) {
        unsigned p = permIdx % edgeIdx * 4;
        ce.setPermAt(12-edgeIdx, unused >> p & 0xf);
        ce.setOrientAt(12-edgeIdx, orientIdx & 1);
        unsigned long m = -1ul << p;
        unused = unused & ~m | unused >> 4 & m;
        permIdx /= edgeIdx;
        orient11 ^= orientIdx;
        orientIdx /= 2;
    }
    ce.setOrientAt(11, orient11 & 1);
    return ce;
}

bool cubeedges::isPermParityOdd() const
{
    bool isSwapsOdd = false;
    unsigned permScanned = 0;
    for(unsigned i = 0; i < 12; ++i) {
        if( permScanned & 1 << i )
            continue;
        permScanned |= 1 << i;
        unsigned p = i;
        while( (p = getPermAt(p)) != i ) {
            permScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
    return isSwapsOdd;
}

bool cubeedges::isBGspace() const {
    const unsigned orients03811[12] = { 0, 1, 1, 0, 2, 2, 2, 2, 0, 1, 1, 0 };
    const unsigned orients12910[12] = { 1, 0, 0, 1, 2, 2, 2, 2, 1, 0, 0, 1 };
    for(unsigned i = 0; i < 12; ++i) {
        if( i < 4 || i >= 8 ) {
            switch( getPermAt(i) ) {
                case 0:
                case 3:
                case 8:
                case 11:
                    if( getOrientAt(i) != orients03811[i] )
                        return false;
                    break;
                case 1:
                case 2:
                case 9:
                case 10:
                    if( getOrientAt(i) != orients12910[i] )
                        return false;
                    break;
                default:
                    return false;
            }
        }else{
            if( getOrientAt(i) )
                return false;
        }
    }
    return true;
}

bool cubeedges::isYWspace() const {
    const unsigned orients03811[12] = { 0, 2, 2, 0, 1, 1, 1, 1, 0, 2, 2, 0 };
    const unsigned orients4567[12]  = { 1, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 1 };
    for(unsigned i = 0; i < 12; ++i) {
        if( orients03811[i] == 2 ) {
            if( getOrientAt(i) )
                return false;
        }else{
            switch( getPermAt(i) ) {
                case 0:
                case 3:
                case 8:
                case 11:
                    if( getOrientAt(i) != orients03811[i] )
                        return false;
                    break;
                case 4:
                case 5:
                case 6:
                case 7:
                    if( getOrientAt(i) != orients4567[i] )
                        return false;
                    break;
                default:
                    return false;
            }
        }
    }
    return true;
}

bool cubeedges::isORspace() const {
    const unsigned orients12910[12] = { 2, 0, 0, 2, 1, 1, 1, 1, 2, 0, 0, 2 };
    const unsigned orients4567[12]  = { 2, 1, 1, 2, 0, 0, 0, 0, 2, 1, 1, 2 };
    for(unsigned i = 0; i < 12; ++i) {
        if( orients12910[i] == 2 ) {
            if( getOrientAt(i) )
                return false;
        }else{
            switch( getPermAt(i) ) {
                case 1:
                case 2:
                case 9:
                case 10:
                    if( getOrientAt(i) != orients12910[i] )
                        return false;
                    break;
                case 4:
                case 5:
                case 6:
                case 7:
                    if( getOrientAt(i) != orients4567[i] )
                        return false;
                    break;
                default:
                    return false;
            }
        }
    }
    return true;
}

static bool isCeReprSolvable(const cubeedges &ce)
{
    bool isSwapsOdd = false;
    unsigned permScanned = 0;
    for(int i = 0; i < 12; ++i) {
        if( permScanned & 1 << i )
            continue;
        permScanned |= 1 << i;
        int p = i;
        while( (p = ce.getPermAt(p)) != i ) {
            if( permScanned & 1 << p ) {
                printf("edge perm %d is twice\n", p);
                return false;
            }
            permScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
	if( isSwapsOdd ) {
		printf("cubeedges unsolvable due to permutation parity\n");
		return false;
	}
	unsigned sumOrient = 0;
	for(int i = 0; i < 12; ++i)
		sumOrient += ce.getOrientAt(i);
	if( sumOrient % 2 ) {
		printf("cubeedges unsolvable due to edge orientations\n");
		return false;
	}
    return true;
}

cubeedges cubeedges::representativeBG() const
{
    cubeedges cerepr;
    const unsigned orientsIn = 0x6060;

    cubeedges ceRev = reverse();
    unsigned destIn = 0, destOut = 4, permOutSum = 0;
    for(unsigned i = 0; i < 12; ++i) {
        unsigned ipos = ceRev.getPermAt(i);
        unsigned long item = edges >> 5*ipos & 0x1f;
        if( ipos < 4 | ipos >= 8 ) {
            item ^= (orientsIn>>ipos ^ orientsIn>>destIn) & 0x10;
            cerepr.edges |= item << 5*destIn;
            ++destIn;
            destIn += destIn & 0x4; // if destIn == 4 then destIn := 8
        }else{
            permOutSum += i;
            if( destOut == 7 && permOutSum & 1 ) {
                // fix permutation parity
                unsigned long item6 = cerepr.edges >> 6*5 & 0x1f;
                cerepr.edges &= ~(0x1ful << 30);
                cerepr.edges |= item6 << 7*5;
                --destOut;
            }
            cerepr.edges |= item << 5*destOut;
            ++destOut;
        }
    }
#if 0
    cube cchk = { .ccp = csolved.ccp, .cco = .cco, .ce = cerepr };
    cube c = { .ccp = csolved.ccp, .cco = csolved.cco, .ce = ce };
    // someSpace = (c rev) ⊙  ccchk
    if( !cube::compose(c.reverse(), cchk).isBGspace() ) {
        printf("fatal: BG cube representative is not congruent\n");
        exit(1);
    }
    if( !isCubeSolvable1(cchk) ) {
        printf("fatal: BG cube representative is unsolvable\n");
        exit(1);
    }
#endif
    return cerepr;
}

cubeedges cubeedges::representativeYW() const
{
    cubeedges cerepr;
    unsigned orientsIn[12] = { 1, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 1 };

    cubeedges ceRev = reverse();
    unsigned destIn = 0, destOut = 0;
    for(unsigned i = 0; i < 12; ++i) {
        unsigned ipos = ceRev.getPermAt(i);
        if( orientsIn[ipos] != 2 ) {
            while( orientsIn[destIn] == 2 )
                ++destIn;
            cerepr.setPermAt(destIn, i);
            cerepr.setOrientAt(destIn, orientsIn[ipos] == orientsIn[destIn] ?
                getOrientAt(ipos) : !getOrientAt(ipos));
            ++destIn;
        }else{
            while( orientsIn[destOut] != 2 )
                ++destOut;
            cerepr.setPermAt(destOut, i);
            cerepr.setOrientAt(destOut, getOrientAt(ipos));
            ++destOut;
        }
    }
    // make the cube solvable
    bool isSwapsOdd = false;
    unsigned permScanned = 0;
    for(int i = 0; i < 12; ++i) {
        if( permScanned & 1 << i )
            continue;
        permScanned |= 1 << i;
        int p = i;
        while( (p = cerepr.getPermAt(p)) != i ) {
            permScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
	if( isSwapsOdd ) {
		unsigned p = cerepr.getPermAt(10);
		unsigned o = cerepr.getOrientAt(10);
        cerepr.setPermAt(10, cerepr.getPermAt(9));
        cerepr.setOrientAt(10, cerepr.getOrientAt(9));
        cerepr.setPermAt(9, p);
        cerepr.setOrientAt(9, o);
	}
    if( !isCeReprSolvable(*this) ) {
        printf("fatal: YW cube repesentative is unsolvable\n");
        exit(1);
    }
    return cerepr;
}

cubeedges cubeedges::representativeOR() const
{
    cubeedges cerepr;
    unsigned orientsIn[12] = { 2, 0, 0, 2, 1, 1, 1, 1, 2, 0, 0, 2 };

    cubeedges ceRev = reverse();
    unsigned destIn = 0, destOut = 0;
    for(unsigned i = 0; i < 12; ++i) {
        unsigned ipos = ceRev.getPermAt(i);
        if( orientsIn[ipos] != 2 ) {
            while( orientsIn[destIn] == 2 )
                ++destIn;
            cerepr.setPermAt(destIn, i);
            cerepr.setOrientAt(destIn, orientsIn[ipos] == orientsIn[destIn] ?
                getOrientAt(ipos) : !getOrientAt(ipos));
            ++destIn;
        }else{
            while( orientsIn[destOut] != 2 )
                ++destOut;
            cerepr.setPermAt(destOut, i);
            cerepr.setOrientAt(destOut, getOrientAt(ipos));
            ++destOut;
        }
    }
    // make the cube solvable
    bool isSwapsOdd = false;
    unsigned permScanned = 0;
    for(int i = 0; i < 12; ++i) {
        if( permScanned & 1 << i )
            continue;
        permScanned |= 1 << i;
        int p = i;
        while( (p = cerepr.getPermAt(p)) != i ) {
            permScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
	if( isSwapsOdd ) {
		unsigned p = cerepr.getPermAt(8);
		unsigned o = cerepr.getOrientAt(8);
        cerepr.setPermAt(8, cerepr.getPermAt(11));
        cerepr.setOrientAt(8, cerepr.getOrientAt(11));
        cerepr.setPermAt(11, p);
        cerepr.setOrientAt(11, o);
	}
    if( !isCeReprSolvable(*this) ) {
        printf("fatal: OR cube repesentative is unsolvable\n");
        exit(1);
    }
    return cerepr;
}

bool cube::operator==(const cube &c) const
{
	return ccp == c.ccp && cco == c.cco && ce == c.ce;
}

bool cube::operator!=(const cube &c) const
{
	return ccp != c.ccp || cco != c.cco || ce != c.ce;
}

bool cube::operator<(const cube &c) const
{
	return ccp < c.ccp || ccp == c.ccp && cco < c.cco ||
        ccp == c.ccp && cco == c.cco && ce < c.ce;
}

const struct cube csolved = {
    .ccp =   cubecorners_perm(0, 1, 2, 3, 4, 5, 6, 7),
    .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
    .ce  = cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
};

cube cube::representativeBG() const {
    cubecorner_orients ccoRepr = cco.representativeBG(ccp);
    cubeedges ceRepr = ce.representativeBG();
    return { .ccp = csolved.ccp, .cco = ccoRepr, .ce = ceRepr };
}

cube cube::representativeYW() const {
    cubecorner_orients ccoRepr = cco.representativeYW(ccp);
    cubeedges ceRepr = ce.representativeYW();
    return { .ccp = csolved.ccp, .cco = ccoRepr, .ce = ceRepr };
}

cube cube::representativeOR() const {
    cubecorner_orients ccoRepr = cco.representativeOR(ccp);
    cubeedges ceRepr = ce.representativeOR();
    return { .ccp = csolved.ccp, .cco = ccoRepr, .ce = ceRepr };
}

std::string cube::toParamText() const
{
    const char *colorChars = "YOBRGW";
    std::string res;

    res += colorChars[cubeCornerColors[ccp.getAt(4)][R120[cco.getAt(4)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(8)][1-ce.getOrientAt(8)]];
    res += colorChars[cubeCornerColors[ccp.getAt(5)][R240[cco.getAt(5)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(4)][ce.getOrientAt(4)]];
    res += 'Y';
    res += colorChars[cubeEdgeColors[ce.getPermAt(5)][ce.getOrientAt(5)]];
    res += colorChars[cubeCornerColors[ccp.getAt(0)][R240[cco.getAt(0)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(0)][1-ce.getOrientAt(0)]];
    res += colorChars[cubeCornerColors[ccp.getAt(1)][R120[cco.getAt(1)]]];

    res += colorChars[cubeCornerColors[ccp.getAt(4)][R240[cco.getAt(4)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(4)][1-ce.getOrientAt(4)]];
    res += colorChars[cubeCornerColors[ccp.getAt(0)][R120[cco.getAt(0)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(9)][ce.getOrientAt(9)]];
    res += 'O';
    res += colorChars[cubeEdgeColors[ce.getPermAt(1)][ce.getOrientAt(1)]];
    res += colorChars[cubeCornerColors[ccp.getAt(6)][R120[cco.getAt(6)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(6)][1-ce.getOrientAt(6)]];
    res += colorChars[cubeCornerColors[ccp.getAt(2)][R240[cco.getAt(2)]]];

    res += colorChars[cubeCornerColors[ccp.getAt(0)][cco.getAt(0)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(0)][ce.getOrientAt(0)]];
    res += colorChars[cubeCornerColors[ccp.getAt(1)][cco.getAt(1)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(1)][1-ce.getOrientAt(1)]];
    res += 'B';
    res += colorChars[cubeEdgeColors[ce.getPermAt(2)][1-ce.getOrientAt(2)]];
    res += colorChars[cubeCornerColors[ccp.getAt(2)][cco.getAt(2)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(3)][ce.getOrientAt(3)]];
    res += colorChars[cubeCornerColors[ccp.getAt(3)][cco.getAt(3)]];

    res += colorChars[cubeCornerColors[ccp.getAt(1)][R240[cco.getAt(1)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(5)][1-ce.getOrientAt(5)]];
    res += colorChars[cubeCornerColors[ccp.getAt(5)][R120[cco.getAt(5)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(2)][ce.getOrientAt(2)]];
    res += 'R';
    res += colorChars[cubeEdgeColors[ce.getPermAt(10)][ce.getOrientAt(10)]];
    res += colorChars[cubeCornerColors[ccp.getAt(3)][R120[cco.getAt(3)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(7)][1-ce.getOrientAt(7)]];
    res += colorChars[cubeCornerColors[ccp.getAt(7)][R240[cco.getAt(7)]]];

    res += colorChars[cubeCornerColors[ccp.getAt(5)][cco.getAt(5)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(8)][ce.getOrientAt(8)]];
    res += colorChars[cubeCornerColors[ccp.getAt(4)][cco.getAt(4)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(10)][1-ce.getOrientAt(10)]];
    res += 'G';
    res += colorChars[cubeEdgeColors[ce.getPermAt(9)][1-ce.getOrientAt(9)]];
    res += colorChars[cubeCornerColors[ccp.getAt(7)][cco.getAt(7)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(11)][ce.getOrientAt(11)]];
    res += colorChars[cubeCornerColors[ccp.getAt(6)][cco.getAt(6)]];

    res += colorChars[cubeCornerColors[ccp.getAt(2)][R120[cco.getAt(2)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(3)][1-ce.getOrientAt(3)]];
    res += colorChars[cubeCornerColors[ccp.getAt(3)][R240[cco.getAt(3)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(6)][ce.getOrientAt(6)]];
    res += 'W';
    res += colorChars[cubeEdgeColors[ce.getPermAt(7)][ce.getOrientAt(7)]];
    res += colorChars[cubeCornerColors[ccp.getAt(6)][R240[cco.getAt(6)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(11)][1-ce.getOrientAt(11)]];
    res += colorChars[cubeCornerColors[ccp.getAt(7)][R120[cco.getAt(7)]]];
    return res;
}

const struct cube crotated[RCOUNT] = {
    {   // ORANGECW
        .ccp =   cubecorners_perm(4, 1, 0, 3, 6, 5, 2, 7),
        .cco = cubecorner_orients(1, 0, 2, 0, 2, 0, 1, 0),
		.ce  = cubeedges(0, 4, 2, 3, 9, 5, 1, 7, 8, 6, 10, 11,
                         0, 1, 0, 0, 1, 0, 1, 0, 0, 1,  0,  0)
    },{ // ORANGE180
		.ccp =   cubecorners_perm(6, 1, 4, 3, 2, 5, 0, 7),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 9, 2, 3, 6, 5, 4, 7, 8, 1, 10, 11,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
    },{ // ORANGECCW
		.ccp =   cubecorners_perm(2, 1, 6, 3, 0, 5, 4, 7),
        .cco = cubecorner_orients(1, 0, 2, 0, 2, 0, 1, 0),
		.ce  = cubeedges(0, 6, 2, 3, 1, 5, 9, 7, 8, 4, 10, 11,
                         0, 1, 0, 0, 1, 0, 1, 0, 0, 1,  0,  0)
    },{ // REDCW
		.ccp =   cubecorners_perm(0, 3, 2, 7, 4, 1, 6, 5),
        .cco = cubecorner_orients(0, 2, 0, 1, 0, 1, 0, 2),
		.ce  = cubeedges(0, 1, 7, 3, 4, 2, 6, 10, 8, 9, 5, 11,
                         0, 0, 1, 0, 0, 1, 0,  1, 0, 0, 1,  0)
    },{ // RED180
		.ccp =   cubecorners_perm(0, 7, 2, 5, 4, 3, 6, 1),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 1, 10, 3, 4, 7, 6, 5, 8, 9, 2, 11,
                         0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  0)
    },{ // REDCCW
		.ccp =   cubecorners_perm(0, 5, 2, 1, 4, 7, 6, 3),
        .cco = cubecorner_orients(0, 2, 0, 1, 0, 1, 0, 2),
		.ce  = cubeedges(0, 1, 5, 3, 4, 10, 6, 2, 8, 9, 7, 11,
                         0, 0, 1, 0, 0,  1, 0, 1, 0, 0, 1,  0)
    },{ // YELLOWCW
		.ccp =   cubecorners_perm(1, 5, 2, 3, 0, 4, 6, 7),
        .cco = cubecorner_orients(2, 1, 0, 0, 1, 2, 0, 0),
		.ce  = cubeedges(5, 1, 2, 3, 0, 8, 6, 7, 4, 9, 10, 11,
                         1, 0, 0, 0, 1, 1, 0, 0, 1, 0,  0,  0)
    },{ // YELLOW180
		.ccp =   cubecorners_perm(5, 4, 2, 3, 1, 0, 6, 7),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(8, 1, 2, 3, 5, 4, 6, 7, 0, 9, 10, 11,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
    },{ // YELLOWCCW
		.ccp =   cubecorners_perm(4, 0, 2, 3, 5, 1, 6, 7),
        .cco = cubecorner_orients(2, 1, 0, 0, 1, 2, 0, 0),
		.ce  = cubeedges(4, 1, 2, 3, 8, 0, 6, 7, 5, 9, 10, 11,
                         1, 0, 0, 0, 1, 1, 0, 0, 1, 0,  0,  0)
    },{ // WHITECW
		.ccp =   cubecorners_perm(0, 1, 6, 2, 4, 5, 7, 3),
        .cco = cubecorner_orients(0, 0, 1, 2, 0, 0, 2, 1),
		.ce  = cubeedges(0, 1, 2, 6, 4, 5, 11, 3, 8, 9, 10, 7,
                         0, 0, 0, 1, 0, 0,  1, 1, 0, 0,  0, 1)
    },{ // WHITE180
		.ccp =   cubecorners_perm(0, 1, 7, 6, 4, 5, 3, 2),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 1, 2, 11, 4, 5, 7, 6, 8, 9, 10, 3,
                         0, 0, 0,  0, 0, 0, 0, 0, 0, 0,  0, 0)
    },{ // WHITECCW
		.ccp =   cubecorners_perm(0, 1, 3, 7, 4, 5, 2, 6),
        .cco = cubecorner_orients(0, 0, 1, 2, 0, 0, 2, 1),
		.ce  = cubeedges(0, 1, 2, 7, 4, 5, 3, 11, 8, 9, 10, 6,
                         0, 0, 0, 1, 0, 0, 1,  1, 0, 0,  0, 1)
    },{ // GREENCW
		.ccp =   cubecorners_perm(0, 1, 2, 3, 5, 7, 4, 6),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 10, 8, 11, 9,
                         0, 0, 0, 0, 0, 0, 0, 0,  1, 1,  1, 1)
    },{ // GREEN180
		.ccp =   cubecorners_perm(0, 1, 2, 3, 7, 6, 5, 4),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 11, 10, 9, 8,
                         0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0)
    },{ // GREENCCW
		.ccp =   cubecorners_perm(0, 1, 2, 3, 6, 4, 7, 5),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 8, 10,
                         0, 0, 0, 0, 0, 0, 0, 0, 1,  1, 1,  1)
    },{ // BLUECW
		.ccp =   cubecorners_perm(2, 0, 3, 1, 4, 5, 6, 7),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(1, 3, 0, 2, 4, 5, 6, 7, 8, 9, 10, 11,
                         1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  0, 0)
    },{ // BLUE180
		.ccp =   cubecorners_perm(3, 2, 1, 0, 4, 5, 6, 7),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(3, 2, 1, 0, 4, 5, 6, 7, 8, 9, 10, 11,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
    },{ // BLUECCW
		.ccp =   cubecorners_perm(1, 3, 0, 2, 4, 5, 6, 7),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(2, 0, 3, 1, 4, 5, 6, 7, 8, 9, 10, 11,
                         1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  0,  0)
	}
};

//   Y
// O B R G      == Y O B R G W
//   W
const struct cube ctransformed[TCOUNT] = {
    csolved,
    {    // O B Y G W R, TD_C0_7_CW
		.ccp =   cubecorners_perm(0, 4, 1, 5, 2, 6, 3, 7),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(4, 0, 8, 5, 1, 9, 2, 10, 6, 3, 11, 7,
                         0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0)
    },{     // B Y O W R G, TD_C0_7_CCW
		.ccp =   cubecorners_perm(0, 2, 4, 6, 1, 3, 5, 7),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(1, 4, 6, 9, 0, 3, 8, 11, 2, 5, 7, 10,
                         0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0,  0)
    },{     // B W R Y O G, TD_C1_6_CW
		.ccp =   cubecorners_perm(3, 1, 7, 5, 2, 0, 6, 4),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(2, 7, 5, 10, 3, 0, 11, 8, 1, 6, 4, 9,
                         0, 0, 0,  0, 0, 0,  0, 0, 0, 0, 0, 0)
    },{     // R G Y B W O, TD_C1_6_CCW
        .ccp =   cubecorners_perm(5, 1, 4, 0, 7, 3, 6, 2),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
        .ce  = cubeedges(5, 8, 0, 4, 10, 2, 9, 1, 7, 11, 3, 6,
                         0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0)
    },{     // G W O Y R B, TD_C2_5_CW
		.ccp =   cubecorners_perm(6, 4, 2, 0, 7, 5, 3, 1),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(9, 6, 4, 1, 11, 8, 3, 0, 10, 7, 5, 2,
                         0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0)
    },{     // R B W G Y O, TD_C2_5_CCW
		.ccp =   cubecorners_perm(3, 7, 2, 6, 1, 5, 0, 4),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(7, 3, 11, 6, 2, 10, 1, 9, 5, 0, 8, 4,
                         0, 0,  0, 0, 0,  0, 0, 0, 0, 0, 0, 0)
    },{     // O G W B Y R, TD_C3_4_CW
		.ccp =   cubecorners_perm(6, 2, 7, 3, 4, 0, 5, 1),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(6, 11, 3, 7, 9, 1, 10, 2, 4, 8, 0, 5,
                         0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0)
    },{     // G Y R W O B, TD_C3_4_CCW
		.ccp =   cubecorners_perm(5, 7, 1, 3, 4, 6, 0, 2),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(10, 5, 7, 2, 8, 11, 0, 3, 9, 4, 6, 1,
                          0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0)
    },{     // O W B Y G R, TD_BG_CW
		.ccp =   cubecorners_perm(2, 0, 3, 1, 6, 4, 7, 5),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(1, 3, 0, 2, 6, 4, 7, 5, 9, 11, 8, 10,
                         1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1,  1)
    },{     // W R B O G Y, TD_BG_180
		.ccp =   cubecorners_perm(3, 2, 1, 0, 7, 6, 5, 4),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8,
                         0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0)
    },{     // R Y B W G O, TD_BG_CCW
		.ccp =   cubecorners_perm(1, 3, 0, 2, 5, 7, 4, 6),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(2, 0, 3, 1, 5, 7, 4, 6, 10, 8, 11, 9,
                         1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1)
    },{     // Y B R G O W, TD_YW_CW
		.ccp =   cubecorners_perm(1, 5, 3, 7, 0, 4, 2, 6),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(5, 2, 10, 7, 0, 8, 3, 11, 4, 1, 9, 6,
                         1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1, 1)
    },{     // Y R G O B W, TD_YW_180
		.ccp =   cubecorners_perm(5, 4, 7, 6, 1, 0, 3, 2),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(8, 10, 9, 11, 5, 4, 7, 6, 0, 2, 1, 3,
                         0,  0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0)
    },{     // Y G O B R W, TD_YW_CCW
		.ccp =   cubecorners_perm(4, 0, 6, 2, 5, 1, 7, 3),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(4, 9, 1, 6, 8, 0, 11, 3, 5, 10, 2, 7,
                         1, 1, 1, 1, 1, 1,  1, 1, 1,  1, 1, 1)
    },{     // B O W R Y G, TD_OR_CW
		.ccp =   cubecorners_perm(2, 3, 6, 7, 0, 1, 4, 5),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(3, 6, 7, 11, 1, 2, 9, 10, 0, 4, 5, 8,
                         1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1)
    },{     // W O G R B Y, TD_OR_180
		.ccp =   cubecorners_perm(6, 7, 4, 5, 2, 3, 0, 1),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(11, 9, 10, 8, 6, 7, 4, 5, 3, 1, 2, 0,
                          0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    },{     // G O Y R W B, TD_OR_CCW
		.ccp =   cubecorners_perm(4, 5, 0, 1, 6, 7, 2, 3),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(8, 4, 5, 0, 9, 10, 1, 2, 11, 6, 7, 3,
                         1, 1, 1, 1, 1,  1, 1, 1,  1, 1, 1, 1)
    },{     // B R Y O W G, TD_E0_11
		.ccp =   cubecorners_perm(1, 0, 5, 4, 3, 2, 7, 6),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(0, 5, 4, 8, 2, 1, 10, 9, 3, 7, 6, 11,
                         1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1,  1)
    },{     // W B O G R Y, TD_E1_10
		.ccp =   cubecorners_perm(2, 6, 0, 4, 3, 7, 1, 5),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(6, 1, 9, 4, 3, 11, 0, 8, 7, 2, 10, 5,
                         1, 1, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1)
    },{     // W G R B O Y, TD_E2_9
		.ccp =   cubecorners_perm(7, 3, 5, 1, 6, 2, 4, 0),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(7, 10, 2, 5, 11, 3, 8, 0, 6, 9, 1, 4,
                         1,  1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1)
    },{     // G R W O Y B, TD_E3_8
		.ccp =   cubecorners_perm(7, 6, 3, 2, 5, 4, 1, 0),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(11, 7, 6, 3, 10, 9, 2, 1, 8, 5, 4, 0,
                          1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1)
    },{     // O Y G W B R, TD_E4_7
		.ccp =   cubecorners_perm(4, 6, 5, 7, 0, 2, 1, 3),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(9, 8, 11, 10, 4, 6, 5, 7, 1, 0, 3, 2,
                         1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
    },{     // R W G Y B O, TD_E5_6
		.ccp =   cubecorners_perm(7, 5, 6, 4, 3, 1, 2, 0),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(10, 11, 8, 9, 7, 5, 6, 4, 2, 3, 0, 1,
                          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
	}
};

static cubecorners_perm cubecornerPermsTransform1(cubecorners_perm ccp, int idx)
{
    cubecorners_perm ccp1 = cubecorners_perm::compose(ctransformed[idx].ccp, ccp);
	return cubecorners_perm::compose(ccp1,
            ctransformed[transformReverse(idx)].ccp);
}

cubecorners_perm cubecorners_perm::transform(unsigned transformDir) const
{
    cubecorners_perm cctr = ctransformed[transformDir].ccp;
    cubecorners_perm ccrtr = ctransformed[transformReverse(transformDir)].ccp;
    return cubecorners_perm::compose3(cctr, *this, ccrtr);
}

cubecorner_orients cubecorner_orients::transform(cubecorners_perm ccp, unsigned transformDir) const
{
    const cube &ctrans = ctransformed[transformDir];
    const cube &ctransRev = ctransformed[transformReverse(transformDir)];
	return cubecorner_orients::compose3(ctrans.cco, ccp, *this, ctransRev.ccp, ctransRev.cco);
}

cubeedges cubeedges::transform(int idx) const
{
    cubeedges cetrans = ctransformed[idx].ce;
	cubeedges res;
#ifdef USE_ASM
    cubeedges cetransRev = ctransformed[transformReverse(idx)].ce;
    unsigned long tmp1;
    asm(
        // store 0xf in xmm3
        "mov $0xf, %[tmp1]\n"
        "vmovq %[tmp1], %%xmm3\n"
        "vpbroadcastb %%xmm3, %%xmm3\n"

        // store ce in xmm0
        "pdep %[depItem], %[ce], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm0, %%xmm0\n"
        "mov %[ce], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm0, %%xmm0\n"

        // store cetrans perm in xmm1
        "pdep %[depItem], %[cetrans], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm1, %%xmm1\n"
        "mov %[cetrans], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm1, %%xmm1\n"
        "vpand %%xmm3, %%xmm1, %%xmm1\n"

        // store ce perm in xmm2
        "vpand %%xmm0, %%xmm3, %%xmm2\n"

        // permute cetrans to ce locations; result in xmm1
        "vpshufb %%xmm2, %%xmm1, %%xmm1\n"

        // store ce orients in xmm2
        "vpandn %%xmm0, %%xmm3, %%xmm2\n"

        // or xmm1 with ce orients
        "vpor %%xmm2, %%xmm1, %%xmm1\n"

        // store cetransRev perm in xmm2
        "pdep %[depItem], %[cetransRev], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm2, %%xmm2\n"
        "mov %[cetransRev], %[tmp1]\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp1]\n"
        "vpinsrq $1, %[tmp1], %%xmm2, %%xmm2\n"
        "vpand %%xmm3, %%xmm2, %%xmm2\n"

        // permute; result in xmm1
        "vpshufb %%xmm2, %%xmm1, %%xmm1\n"

        // store xmm1 in res
        "vpextrq $0, %%xmm1, %[res]\n"
        "pext %[depItem], %[res], %[res]\n"
        "vpextrd $2, %%xmm1, %k[tmp1]\n"
        "pext %[depItem], %[tmp1], %[tmp1]\n"
        "shl $40, %[tmp1]\n"
        "or %[tmp1], %[res]\n"
        : [res]         "=&r"  (res.edges),
          [tmp1]        "=&r" (tmp1)
        : [ce]          "r"   (this->edges),
          [cetrans]     "rm"  (cetrans.edges),
          [cetransRev]  "r"   (cetransRev.edges),
          [depItem]     "rm"  (0x1f1f1f1f1f1f1f1ful)
        : "xmm0", "xmm1", "xmm2", "xmm3", "cc"
       );
#ifdef ASMCHECK
    cubeedges chk = res;
    res.edges = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
#if 0
    cubeedges cetransRev = ctransformed[transformReverse(idx)].ce;
    cubeedges ce1 = cubeedges::compose(cetrans, *this);
    res = cubeedges::compose(ce1, cetransRev);
#elif 0
    cubeedges cetransRev = ctransformed[transformReverse(idx)].ce;
	cubeedges mid;
	for(int i = 0; i < 12; ++i) {
		unsigned cePerm = this->edges >> 5 * i & 0xf;
		unsigned long ctransPerm = cetrans.edges >> 5 * cePerm & 0xf;
        mid.edges |= ctransPerm << 5 * i;
    }
    unsigned long edgeorients = this->edges & 0x842108421084210ul;
    mid.edges |= edgeorients;
	for(int i = 0; i < 12; ++i) {
		unsigned ctransRev = cetransRev.edges >> 5 * i & 0xf;
        unsigned long ctransPerm = mid.edges >> 5 * ctransRev & 0x1f;
		res.edges |= ctransPerm << 5*i;
	}
#else
	for(int i = 0; i < 12; ++i) {
		unsigned ceItem = this->edges >> 5 * i;
		unsigned cePerm = ceItem & 0xf;
		unsigned ceOrient = ceItem & 0x10;
		unsigned ctransPerm = cetrans.edges >> 5 * cePerm & 0xf;
		unsigned ctransPerm2 = cetrans.edges >> 5 * i & 0xf;
		res.edges |= (unsigned long)(ctransPerm | ceOrient) << 5*ctransPerm2;
	}
#endif
#endif // defined(ASMCHECK) || !defined(USE_ASM)
#ifdef ASMCHECK
    if( res != chk ) {
        flockfile(stdout);
        std::cout << "edges transform mismatch!" << std::endl;
        std::cout << "edges            = " << dumpedges(this->edges) << std::endl;
        std::cout << "cetrans.edges    = " << dumpedges(cetrans.edges) << std::endl;
        std::cout << "cetransRev.edges = " << dumpedges(cetransRev.edges) << std::endl;
        std::cout << "expected.edges   = " << dumpedges(res.edges) << std::endl;
        std::cout << "got.edges        = " << dumpedges(chk.edges) << std::endl;
        funlockfile(stdout);
        exit(1);
    }
#endif
	return res;
}

cube cube::transform(unsigned transformDir) const
{
    const cube &ctrans = ctransformed[transformDir];
    cubecorners_perm ccp1 = cubecorners_perm::compose(ctrans.ccp, this->ccp);
    cubecorner_orients cco1 = cubecorner_orients::compose(ctrans.cco, this->ccp, this->cco);
    cubeedges ce1 = cubeedges::compose(ctrans.ce, this->ce);

    const cube &ctransRev = ctransformed[transformReverse(transformDir)];
    cubecorners_perm ccp2 = cubecorners_perm::compose(ccp1, ctransRev.ccp);
    cubecorner_orients cco2 = cubecorner_orients::compose(cco1, ctransRev.ccp, ctransRev.cco);
    cubeedges ce2 = cubeedges::compose(ce1, ctransRev.ce);
	return { .ccp = ccp2, .cco = cco2, .ce = ce2 };
}

void cubePrint(const cube &c)
{
	const char *colorPrint[CCOUNT] = {
		"\033[48;2;230;230;0m  \033[m",		// CYELLOW
		"\033[48;2;230;148;0m  \033[m",		// CORANGE
		"\033[48;2;0;0;230m  \033[m",		// CBLUE
		"\033[48;2;230;0;0m  \033[m",		// CRED
		"\033[48;2;0;230;0m  \033[m",		// CGREEN
		"\033[48;2;230;230;230m  \033[m",	// CWHITE
	};

	printf("\n");
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(4)][R120[c.cco.getAt(4)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(8)][!c.ce.getOrientAt(8)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(5)][R240[c.cco.getAt(5)]]]);
	printf("        %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.getPermAt(4)][c.ce.getOrientAt(4)]],
			colorPrint[CYELLOW],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(5)][c.ce.getOrientAt(5)]]);
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(0)][R240[c.cco.getAt(0)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(0)][!c.ce.getOrientAt(0)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(1)][R120[c.cco.getAt(1)]]]);
	printf("\n");
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(4)][R240[c.cco.getAt(4)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(4)][!c.ce.getOrientAt(4)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(0)][R120[c.cco.getAt(0)]]],
			colorPrint[cubeCornerColors[c.ccp.getAt(0)][c.cco.getAt(0)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(0)][c.ce.getOrientAt(0)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(1)][c.cco.getAt(1)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(1)][R240[c.cco.getAt(1)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(5)][!c.ce.getOrientAt(5)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(5)][R120[c.cco.getAt(5)]]],
			colorPrint[cubeCornerColors[c.ccp.getAt(5)][c.cco.getAt(5)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(8)][c.ce.getOrientAt(8)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(4)][c.cco.getAt(4)]]);
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.getPermAt(9)][c.ce.getOrientAt(9)]],
			colorPrint[CORANGE],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(1)][c.ce.getOrientAt(1)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(1)][!c.ce.getOrientAt(1)]],
			colorPrint[CBLUE],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(2)][!c.ce.getOrientAt(2)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(2)][c.ce.getOrientAt(2)]],
			colorPrint[CRED],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(10)][c.ce.getOrientAt(10)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(10)][!c.ce.getOrientAt(10)]],
			colorPrint[CGREEN],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(9)][!c.ce.getOrientAt(9)]]);
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(6)][R120[c.cco.getAt(6)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(6)][!c.ce.getOrientAt(6)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(2)][R240[c.cco.getAt(2)]]],
			colorPrint[cubeCornerColors[c.ccp.getAt(2)][c.cco.getAt(2)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(3)][c.ce.getOrientAt(3)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(3)][c.cco.getAt(3)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(3)][R120[c.cco.getAt(3)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(7)][!c.ce.getOrientAt(7)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(7)][R240[c.cco.getAt(7)]]],
			colorPrint[cubeCornerColors[c.ccp.getAt(7)][c.cco.getAt(7)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(11)][c.ce.getOrientAt(11)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(6)][c.cco.getAt(6)]]);
	printf("\n");
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(2)][R120[c.cco.getAt(2)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(3)][!c.ce.getOrientAt(3)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(3)][R240[c.cco.getAt(3)]]]);
	printf("        %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.getPermAt(6)][c.ce.getOrientAt(6)]],
			colorPrint[CWHITE],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(7)][c.ce.getOrientAt(7)]]);
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(6)][R240[c.cco.getAt(6)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(11)][!c.ce.getOrientAt(11)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(7)][R120[c.cco.getAt(7)]]]);
	printf("\n");
}

