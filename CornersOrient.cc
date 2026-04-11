#include "cube.h"

CornersOrient::CornersOrient(unsigned corner0orient, unsigned corner1orient,
			unsigned corner2orient, unsigned corner3orient,
			unsigned corner4orient, unsigned corner5orient,
			unsigned corner6orient, unsigned corner7orient)
	: orients(corner0orient | corner1orient << 2 | corner2orient << 4 |
			corner3orient << 6 | corner4orient << 8 | corner5orient << 10 |
			corner6orient << 12 | corner7orient << 14)
{
}

CornersOrient CornersOrient::compose(CornersOrient cco1,
            CornersPerm ccp2, CornersOrient cco2)
{
    CornersOrient res;
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
    CornersOrient chk = res;
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

CornersOrient CornersOrient::compose3(CornersOrient cco1,
            CornersPerm ccp2, CornersOrient cco2,
            CornersPerm ccp3, CornersOrient cco3)
{
    CornersOrient res;
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
    CornersOrient chk = res;
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

CornersOrient CornersOrient::reverse(CornersPerm ccp) const {
    unsigned short revOrients = (orients & 0xaaaa) >> 1 | (orients & 0x5555) << 1;
    CornersOrient res;
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
    CornersOrient chk = res;
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

unsigned short CornersOrient::getOrientIdx() const
{
	unsigned short res = 0;
	for(unsigned i = 0; i < 7; ++i)
		res = res * 3 + getAt(i);
	return res;
}

CornersOrient CornersOrient::fromOrientIdx(unsigned short idx)
{
    CornersOrient res;
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

bool CornersOrient::isBGspace() const {
    return orients == 0;
}

bool CornersOrient::isYWspace(CornersPerm ccp) const {
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

bool CornersOrient::isORspace(CornersPerm ccp) const {
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

CornersOrient CornersOrient::representativeBG(CornersPerm ccp) const
{
    CornersOrient orepr;
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
    CornersOrient chk = orepr;
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

CornersOrient CornersOrient::representativeYW(CornersPerm ccp) const
{
    unsigned oadd[8] = { 2, 1, 1, 2, 1, 2, 2, 1 };
    CornersOrient orepr;
    CornersPerm ccpRev = ccp.reverse();
    for(unsigned i = 0; i < 8; ++i) {
        unsigned toAdd = oadd[i] == oadd[ccpRev.getAt(i)] ? 0 : oadd[i];
        orepr.setAt(i, (getAt(ccpRev.getAt(i))+toAdd) % 3);
    }
    return orepr;
}

CornersOrient CornersOrient::representativeOR(CornersPerm ccp) const
{
    unsigned oadd[8] = { 1, 2, 2, 1, 2, 1, 1, 2 };
    CornersOrient orepr;
    CornersPerm ccpRev = ccp.reverse();
    for(unsigned i = 0; i < 8; ++i) {
        unsigned toAdd = oadd[i] == oadd[ccpRev.getAt(i)] ? 0 : oadd[i];
        orepr.setAt(i, (getAt(ccpRev.getAt(i))+toAdd) % 3);
    }
    return orepr;
}

CornersOrient CornersOrient::transform(CornersPerm ccp, unsigned transformDir) const
{
    const cube &ctrans = ctransformed[transformDir];
    const cube &ctransRev = ctransformed[transformReverse(transformDir)];
	return CornersOrient::compose3(ctrans.cco, ccp, *this, ctransRev.ccp, ctransRev.cco);
}

