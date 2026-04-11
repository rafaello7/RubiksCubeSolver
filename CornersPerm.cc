#include "cube.h"

CornersPerm::CornersPerm(unsigned corner0perm, unsigned corner1perm,
			unsigned corner2perm, unsigned corner3perm,
			unsigned corner4perm, unsigned corner5perm,
			unsigned corner6perm, unsigned corner7perm)
	: perm(corner0perm | corner1perm << 3 | corner2perm << 6 |
			corner3perm << 9 | corner4perm << 12 | corner5perm << 15 |
			corner6perm << 18 | corner7perm << 21)
{
}

CornersPerm CornersPerm::compose(CornersPerm ccp1, CornersPerm ccp2)
{
    CornersPerm res;
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
    CornersPerm chk = res;
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

CornersPerm CornersPerm::compose3(CornersPerm ccp1, CornersPerm ccp2,
        CornersPerm ccp3)
{
    CornersPerm res;
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
    CornersPerm chk = res;
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

CornersPerm CornersPerm::reverse() const
{
    CornersPerm res;
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
    CornersPerm chk = res;
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

unsigned short CornersPerm::getPermIdx() const {
    unsigned short res = 0;
    unsigned indexes = 0;
    for(int i = 7; i >= 0; --i) {
        unsigned p = getAt(i) * 4;
        res = (8-i) * res + (indexes >> p & 0xf);
        indexes += 0x11111111 << p;
    }
    return res;
}

CornersPerm CornersPerm::fromPermIdx(unsigned short idx)
{
    unsigned unused = 0x76543210;
    CornersPerm ccp;

    for(unsigned cornerIdx = 8; cornerIdx > 0; --cornerIdx) {
        unsigned p = idx % cornerIdx * 4;
        ccp.setAt(8-cornerIdx, unused >> p & 0xf);
        unsigned m = -1 << p;
        unused = unused & ~m | unused >> 4 & m;
        idx /= cornerIdx;
    }
    return ccp;
}

bool CornersPerm::isPermParityOdd() const
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

static CornersPerm cubecornerPermsTransform1(CornersPerm ccp, int idx)
{
    CornersPerm ccp1 = CornersPerm::compose(ctransformed[idx].ccp, ccp);
	return CornersPerm::compose(ccp1,
            ctransformed[transformReverse(idx)].ccp);
}

CornersPerm CornersPerm::transform(unsigned transformDir) const
{
    CornersPerm cctr = ctransformed[transformDir].ccp;
    CornersPerm ccrtr = ctransformed[transformReverse(transformDir)].ccp;
    return CornersPerm::compose3(cctr, *this, ccrtr);
}

