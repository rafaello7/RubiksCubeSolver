#include <cstdio>
#include <cstdlib>
#include "cube.h"

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

