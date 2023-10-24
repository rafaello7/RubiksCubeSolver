#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <map>
#include <set>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdarg.h>

#ifdef __x86_64__
#define USE_ASM
//#define ASMCHECK
#endif

enum { CSARR_SIZE = 11 };

const struct {
    unsigned memMinMB;
    struct {
        unsigned depthMax;
        bool useReverse;
    } par;
} depthMaxByMem[] = {
    { .memMinMB =      0, .par =  { .depthMax = 8,  .useReverse = true  }}, // ~320MB
//  { .memMinMB =    768, .par =  { .depthMax = 8,  .useReverse = false }}, slower than 8 rev; ~600MB
    { .memMinMB =   2700, .par =  { .depthMax = 9,  .useReverse = true  }}, // ~2.4GB
    { .memMinMB =   5120, .par =  { .depthMax = 9,  .useReverse = false }}, // ~4.8GB
    { .memMinMB = 1u<<31, .par =  { .depthMax = 10, .useReverse = true  }}, // ~27GB
    { .memMinMB = 1u<<31, .par =  { .depthMax = 10, .useReverse = false }}, // ~53GB
};

static unsigned depthMaxSelFn() {
    long pageSize = sysconf(_SC_PAGESIZE);
    long pageCount = sysconf(_SC_PHYS_PAGES);
    long memSizeMB = pageSize * pageCount / 1048576;
    unsigned model = 0;
    while( depthMaxByMem[model+1].memMinMB <= memSizeMB )
        ++model;
    std::cout << "mem MB: " << memSizeMB << ", model: " << model << std::endl;
    return model;
}

auto [DEPTH_MAX, USEREVERSE] = depthMaxByMem[depthMaxSelFn()].par;

static void sendRespMessage(int fd, const char *fmt, ...)
{
    va_list args;
    char message[4096], respBuf[5000];

    va_start(args, fmt);
    vsprintf(message, fmt, args);
    va_end(args);
    sprintf(respBuf, "%lx\r\n%s\r\n", strlen(message), message);
    write(fd, respBuf, strlen(respBuf));
}

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

enum rotate_dir {
	ORANGECW, ORANGE180, ORANGECCW, REDCW, RED180, REDCCW,
	YELLOWCW, YELLOW180, YELLOWCCW, WHITECW, WHITE180, WHITECCW,
	GREENCW, GREEN180, GREENCCW, BLUECW, BLUE180, BLUECCW,
	RCOUNT
};

enum cubecolor {
	CYELLOW, CORANGE, CBLUE, CRED, CGREEN, CWHITE, CCOUNT
};

static const int cubeCornerColors[8][3] = {
  { CBLUE,   CORANGE, CYELLOW }, { CBLUE,  CYELLOW, CRED },
  { CBLUE,   CWHITE,  CORANGE }, { CBLUE,  CRED,    CWHITE },
  { CGREEN,  CYELLOW, CORANGE }, { CGREEN, CRED,    CYELLOW },
  { CGREEN,  CORANGE, CWHITE  }, { CGREEN, CWHITE,  CRED }
};

static const int cubeEdgeColors[12][2] = {
  { CBLUE,   CYELLOW },
  { CORANGE, CBLUE   }, { CRED,    CBLUE },
  { CBLUE,   CWHITE  },
  { CYELLOW, CORANGE }, { CYELLOW, CRED },
  { CWHITE,  CORANGE }, { CWHITE,  CRED },
  { CGREEN,  CYELLOW },
  { CORANGE, CGREEN  }, { CRED,    CGREEN },
  { CGREEN,  CWHITE  }
};

static const char *rotateDirName(int rd) {
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

static int rotateDirReverse(int rd) {
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

static const unsigned char R120[3] = { 1, 2, 0 };
static const unsigned char R240[3] = { 2, 0, 1 };

class cubecorner_perms {
	unsigned perms;
public:
	cubecorner_perms() : perms(0) {}
	cubecorner_perms(unsigned corner0perm, unsigned corner1perm, unsigned corner2perm,
			unsigned corner3perm, unsigned corner4perm, unsigned corner5perm,
			unsigned corner6perm, unsigned corner7perm);

	void setAt(unsigned idx, unsigned char perm) {
		perms &= ~(7 << 3*idx);
		perms |= perm << 3*idx;
	}
	unsigned getAt(unsigned idx) const { return perms >> 3*idx & 7; }
	unsigned get() const { return perms; }
	void set(unsigned pms) { perms = pms; }
    static cubecorner_perms compose(cubecorner_perms, cubecorner_perms);
    cubecorner_perms symmetric() const {
        unsigned permsRes = perms ^ 0x924924;
        cubecorner_perms ccpres;
        ccpres.perms = (permsRes >> 12 | permsRes << 12) & 0xffffff;
        return ccpres;
    }
    cubecorner_perms reverse() const;
	bool operator==(const cubecorner_perms &ccp) const { return perms == ccp.perms; }
	bool operator!=(const cubecorner_perms &ccp) const { return perms != ccp.perms; }
	bool operator<(const cubecorner_perms &ccp) const { return perms < ccp.perms; }
};

cubecorner_perms::cubecorner_perms(unsigned corner0perm, unsigned corner1perm,
			unsigned corner2perm, unsigned corner3perm,
			unsigned corner4perm, unsigned corner5perm,
			unsigned corner6perm, unsigned corner7perm)
	: perms(corner0perm | corner1perm << 3 | corner2perm << 6 |
			corner3perm << 9 | corner4perm << 12 | corner5perm << 15 |
			corner6perm << 18 | corner7perm << 21)
{
}

cubecorner_perms cubecorner_perms::compose(cubecorner_perms ccp1, cubecorner_perms ccp2)
{
    cubecorner_perms res;
    for(unsigned i = 0; i < 8; ++i)
        res.perms |= ccp1.getAt(ccp2.getAt(i)) << 3*i;
    return res;
}

cubecorner_perms cubecorner_perms::reverse() const
{
    cubecorner_perms res;
#ifdef USE_ASM
    unsigned long tmp1;
    asm (
        // xmm1 := perm
        "pdep %[depPerm], %q[perms], %[tmp1]\n"
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
        : [perms]       "r"     (perms),
          [depPerm]     "rm"    (0x707070707070707ul)
        : "ymm1", "ymm2"
        );
#ifdef ASMCHECK
    cubecorner_perms chk = res;
    res.perms = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
    for(int i = 0; i < 8; ++i)
        res.perms |= i << 3*getAt(i);
#endif
#ifdef ASMCHECK
    if( res.perms != chk.perms ) {
        flockfile(stdout);
        printf("corner perms reverse mismatch!\n");
        printf("this.perms = 0x%o;\n", perms);
        printf("exp.perms  = 0x%o;\n", res.perms);
        printf("got.perms  = 0x%o;\n", chk.perms);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
	return res;
}

class cubecorner_orients {
	unsigned short orients;
public:
	cubecorner_orients() : orients(0) {}
	cubecorner_orients(unsigned corner0orient, unsigned corner1orient,
			unsigned corner2orient, unsigned corner3orient,
			unsigned corner4orient, unsigned corner5orient,
			unsigned corner6orient, unsigned corner7orient);

	void setAt(unsigned idx, unsigned char orient) {
		orients &= ~(3 << 2*idx);
		orients |= orient << 2*idx;
	}
	unsigned getAt(unsigned idx) const { return orients >> 2*idx & 3; }
	unsigned get() const { return orients; }
	void set(unsigned orts) { orients = orts; }
    static cubecorner_orients compose(cubecorner_orients cco1,
            cubecorner_perms ccp2, cubecorner_orients cco2);
    cubecorner_orients symmetric() const {
        cubecorner_orients ccores;
        // set orient 2 -> 1, 1 -> 2, 0 unchanged
        unsigned short orie = (orients & 0xaaaa) >> 1 | (orients & 0x5555) << 1;
        ccores.orients = (orie >> 8 | orie << 8) & 0xffff;
        return ccores;
    }
    cubecorner_orients reverse(cubecorner_perms) const;
	bool operator==(const cubecorner_orients &cco) const { return orients == cco.orients; }
	bool operator!=(const cubecorner_orients &cco) const { return orients != cco.orients; }
	bool operator<(const cubecorner_orients &cco) const { return orients < cco.orients; }
};

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
            cubecorner_perms ccp2, cubecorner_orients cco2)
{
	unsigned short resOrients = 0;
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
        : [resOrients]  "=r"  (resOrients),
          [tmp1]        "=&r" (tmp1)
        : [cco1]        "r"   ((unsigned long)cco1.get()),
          [ccp2]        "r"   ((unsigned long)ccp2.get()),
          [cco2]        "r"   ((unsigned long)cco2.get()),
          [depPerm]     "rm"  (0x707070707070707ul),
          [depOrient]   "rm"  (0x303030303030303ul),
          [mod3]        "rm"  (0x100020100)
        : "xmm1", "xmm2"
       );
#ifdef ASMCHECK
    unsigned short chkOrients = resOrients;
    resOrients = 0;
#endif
#endif // USE_ASM
#if defined(ASMCHECK) || !defined(USE_ASM)
	static const unsigned char MOD3[] = { 0, 1, 2, 0, 1 };
	for(int i = 0; i < 8; ++i) {
        unsigned cc2Perm = ccp2.get() >> 3*i & 7;
        unsigned cc2Orient = cco2.get() >> 2*i & 3;
        unsigned cco1Orient = cco1.get() >> 2*cc2Perm & 3;
        unsigned resOrient = MOD3[cco1Orient + cc2Orient];
        resOrients |= resOrient << 2 * i;
    }
#endif
#ifdef ASMCHECK
    if( resOrients != chkOrients ) {
        flockfile(stdout);
        printf("corners compose mismatch!\n");
        printf("cco1        = 0x%x;\n", cco1.get());
        printf("ccp2        = 0x%x;\n", ccp2.get());
        printf("cco2        = 0x%x;\n", cco2.get());
        printf("exp.orients = 0x%lx;\n", resOrients);
        printf("got.orients = 0x%lx;\n", chkOrients);
        funlockfile(stdout);
        exit(1);
    }
#endif  // ASMCHECK
    cubecorner_orients res;
    res.set(resOrients);
	return res;
}

cubecorner_orients cubecorner_orients::reverse(cubecorner_perms ccp) const {
    unsigned short revOrients = (orients & 0xaaaa) >> 1 | (orients & 0x5555) << 1;
    cubecorner_orients res;
#ifdef USE_ASM
    unsigned long tmp1;
    asm (
        // xmm2[63:0] := 2*perm
        "pdep %[depPermx2], %q[perms], %[tmp1]\n"
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
        : [perms]       "r"     (ccp.get()),
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

class cubecorners {
	cubecorner_perms perms;
	cubecorner_orients orients;
public:
	cubecorners() {}
	cubecorners(const cubecorner_perms &perms, const cubecorner_orients &orients)
		: perms(perms), orients(orients) {}
	cubecorners(unsigned corner0perm, unsigned corner0orient,
			unsigned corner1perm, unsigned corner1orient,
			unsigned corner2perm, unsigned corner2orient,
			unsigned corner3perm, unsigned corner3orient,
			unsigned corner4perm, unsigned corner4orient,
			unsigned corner5perm, unsigned corner5orient,
			unsigned corner6perm, unsigned corner6orient,
			unsigned corner7perm, unsigned corner7orient);

	unsigned getPermAt(unsigned idx) const { return perms.getAt(idx); }
	void setPermAt(unsigned idx, unsigned char perm) { perms.setAt(idx, perm); }
	unsigned getOrientAt(unsigned idx) const { return orients.getAt(idx); }
	void setOrientAt(unsigned idx, unsigned char orient) { orients.setAt(idx, orient); }
	cubecorner_perms getPerms() const { return perms; }
	cubecorner_orients getOrients() const { return orients; }
	void setOrients(unsigned orts) { orients.set(orts); }
    static cubecorner_orients orientsCompose(cubecorner_orients cco1,
            const cubecorners &cc2) {
        return cubecorner_orients::compose(cco1, cc2.perms, cc2.orients);
    }
    static cubecorners compose(const cubecorners &cc1, const cubecorners &cc2) {
        return cubecorners(cubecorner_perms::compose(cc1.perms, cc2.perms),
                cubecorner_orients::compose(cc1.orients, cc2.perms, cc2.orients));
    }
    cubecorners symmetric() const {
        return cubecorners(perms.symmetric(), orients.symmetric());
    }
    cubecorners reverse() const {
        return cubecorners(perms.reverse(), orients.reverse(perms));
    }

	bool operator==(const cubecorners &cc) const {
		return perms == cc.perms && orients == cc.orients;
	}
	bool operator<(const cubecorners &cc) const {
		return perms < cc.perms || perms == cc.perms && orients < cc.orients;
	}
};

cubecorners::cubecorners(unsigned corner0perm, unsigned corner0orient,
			unsigned corner1perm, unsigned corner1orient,
			unsigned corner2perm, unsigned corner2orient,
			unsigned corner3perm, unsigned corner3orient,
			unsigned corner4perm, unsigned corner4orient,
			unsigned corner5perm, unsigned corner5orient,
			unsigned corner6perm, unsigned corner6orient,
			unsigned corner7perm, unsigned corner7orient)

	: perms(corner0perm, corner1perm, corner2perm, corner3perm, corner4perm, corner5perm,
			corner6perm, corner7perm),
	orients(corner0orient, corner1orient, corner2orient,
			corner3orient, corner4orient, corner5orient, corner6orient, corner7orient)
{
}

class cubeedges {
	unsigned long edges;
public:
	cubeedges() : edges(0) {}
	cubeedges(unsigned edge0perm, unsigned edge0orient,
			unsigned edge1perm, unsigned edge1orient,
			unsigned edge2perm, unsigned edge2orient,
			unsigned edge3perm, unsigned edge3orient,
			unsigned edge4perm, unsigned edge4orient,
			unsigned edge5perm, unsigned edge5orient,
			unsigned edge6perm, unsigned edge6orient,
			unsigned edge7perm, unsigned edge7orient,
			unsigned edge8perm, unsigned edge8orient,
			unsigned edge9perm, unsigned edge9orient,
			unsigned edge10perm, unsigned edge10orient,
			unsigned edge11perm, unsigned edge11orient);

	void setAt(unsigned idx, unsigned char perm, unsigned char orient) {
		edges &= ~(0x1FUL << 5*idx);
		edges |= (unsigned long)(orient<<4 | perm) << 5*idx;
	}
	void setPermAt(unsigned idx, unsigned char perm) {
		edges &= ~(0xFUL << 5*idx);
		edges |= (unsigned long)perm << 5*idx;
	}
	void setOrientAt(unsigned idx, unsigned char orient) {
		edges &= ~(0x1UL << 5*idx+4);
		edges |= (unsigned long)orient << 5*idx+4;
	}
    static cubeedges compose(cubeedges, cubeedges);
    static cubeedges compose3(cubeedges, cubeedges, cubeedges);

    // the middle cubeedges should be reversed before compose
    static cubeedges compose3revmid(cubeedges, cubeedges, cubeedges);
    static cubeedges transform(cubeedges, int idx);
    cubeedges symmetric() const {
        cubeedges ceres;
        // map perm: 0 1 2 3 4 5 6 7 8 9 10 11 -> 8 9 10 11 4 5 6 7 0 1 2 3
        unsigned long edg = edges ^ (~edges & 0x210842108421084ul) << 1;
        ceres.edges = edg >> 40 | edg & 0xfffff00000ul | (edg & 0xfffff) << 40;
        return ceres;
    }
    cubeedges reverse() const;
	unsigned edgeN(unsigned idx) const { return edges >> 5 * idx & 0xf; }
	unsigned edgeR(unsigned idx) const { return edges >> (5*idx+4) & 1; }
	bool operator==(const cubeedges &ce) const { return edges == ce.edges; }
	bool operator!=(const cubeedges &ce) const { return edges != ce.edges; }
	bool operator<(const cubeedges &ce) const { return edges < ce.edges; }
    unsigned getPermIdx() const;
    unsigned short getOrientIdx() const;
    unsigned long get() const { return edges; }
    bool isNil() const { return edges == 0; }
};

cubeedges::cubeedges(unsigned edge0perm, unsigned edge0orient,
			unsigned edge1perm, unsigned edge1orient,
			unsigned edge2perm, unsigned edge2orient,
			unsigned edge3perm, unsigned edge3orient,
			unsigned edge4perm, unsigned edge4orient,
			unsigned edge5perm, unsigned edge5orient,
			unsigned edge6perm, unsigned edge6orient,
			unsigned edge7perm, unsigned edge7orient,
			unsigned edge8perm, unsigned edge8orient,
			unsigned edge9perm, unsigned edge9orient,
			unsigned edge10perm, unsigned edge10orient,
			unsigned edge11perm, unsigned edge11orient)
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

    // needs: bmi2 (pext, pdep), sse3 (pshufb), sse4.1 (pinsrq, pextrq)
    asm(
        // xmm1 := 1
        "mov $1, %[tmp1]\n"
        "vmovq %[tmp1], %%xmm1\n"
        "vpbroadcastb %%xmm1, %%xmm1\n"

        // xmm4 := edgeItem
        "pdep %[depItem], %[ce2], %[tmp1]\n"
        "vpinsrq $0, %[tmp1], %%xmm4, %%xmm4\n"
        "mov %[ce2], %[tmp1]\n"
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
        // xmm2 := 8 * (edgeItem & 0xf)
        "vpsllw $3, %%xmm3, %%xmm2\n"

        // ymm8 := 64
        "mov $64, %[tmp1]\n"
        "vmovq %[tmp1], %%xmm8\n"
        "vpbroadcastq %%xmm8, %%ymm8\n"

        // ymm5,ymm6 = (edgeItem & 0x10 | i) <<  8*(edgeItem & 0xf) : i=0,1,2,3
        // ymm5 - values shifthed less than 64, ymm6 - >=64
        "vpmovzxbq %%xmm1, %%ymm3\n"
        "vpmovzxbq %%xmm2, %%ymm4\n"
        "vpsllvq %%ymm4, %%ymm3, %%ymm5\n"
        "vpsubq %%ymm8, %%ymm4, %%ymm4\n"
        "vpsllvq %%ymm4, %%ymm3, %%ymm6\n"

        // ymm5,ymm6 |= (edgeItem & 0x10 | i) <<  8*(edgeItem & 0xf) : i=4,5,6,7
        "vpsrldq $4, %%xmm1, %%xmm3\n"
        "vpmovzxbq %%xmm3, %%ymm3\n"
        "vpsrldq $4, %%xmm2, %%xmm4\n"
        "vpmovzxbq %%xmm4, %%ymm4\n"
        "vpsllvq %%ymm4, %%ymm3, %%ymm7\n"
        "vpor %%ymm7, %%ymm5, %%ymm5\n"
        "vpsubq %%ymm8, %%ymm4, %%ymm4\n"
        "vpsllvq %%ymm4, %%ymm3, %%ymm7\n"
        "vpor %%ymm7, %%ymm6, %%ymm6\n"

        // ymm5,ymm6 |= (edgeItem & 0x10 | i) <<  8*(edgeItem & 0xf) : i=8,9,10,11
        "vpsrldq $8, %%xmm1, %%xmm3\n"
        "vpmovzxbq %%xmm3, %%ymm3\n"
        "vpsrldq $8, %%xmm2, %%xmm4\n"
        "vpmovzxbq %%xmm4, %%ymm4\n"
        "vpsllvq %%ymm4, %%ymm3, %%ymm7\n"
        "vpor %%ymm7, %%ymm5, %%ymm5\n"
        "vpsubq %%ymm8, %%ymm4, %%ymm4\n"
        "vpsllvq %%ymm4, %%ymm3, %%ymm7\n"
        "vpor %%ymm7, %%ymm6, %%ymm6\n"

        // perform horizontal OR on ymm5
        "vextracti128 $1, %%ymm5, %%xmm7\n"
        "vpor %%xmm7, %%xmm5, %%xmm5\n"
        "vpshufd $0x4e, %%xmm5, %%xmm7\n"
        "vpor %%xmm7, %%xmm5, %%xmm5\n"

        // perform horizontal OR on ymm6
        "vextracti128 $1, %%ymm6, %%xmm7\n"
        "vpor %%xmm7, %%xmm6, %%xmm6\n"
        "vpshufd $0x4e, %%xmm6, %%xmm7\n"
        "vpor %%xmm7, %%xmm6, %%xmm6\n"
        // store the reversed ce2 in xmm0
        "vpblendd $3, %%xmm5, %%xmm6, %%xmm0\n"

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

struct cube {
	cubecorners cc;
	cubeedges ce;

    static cube compose(const cube &c1, const cube &c2) {
        return { .cc = cubecorners::compose(c1.cc, c2.cc),
                 .ce = cubeedges::compose(c1.ce, c2.ce) };
    }
    cube symmetric() const {
        return { .cc = cc.symmetric(), .ce = ce.symmetric() };
    }
    cube reverse() const {
        return { .cc = cc.reverse(), .ce = ce.reverse() };
    }

	bool operator==(const cube &c) const;
	bool operator<(const cube &c) const;
};

bool cube::operator==(const cube &c) const
{
	return cc == c.cc && ce == c.ce;
}

bool cube::operator<(const cube &c) const
{
	return cc < c.cc || cc == c.cc && ce < c.ce;
}

//           ___________
//           | YELLOW  |
//           | 4  8  5 |
//           | 4     5 |
//           | 2  3  3 |
//  _________|_________|_________ _________
// |  ORANGE |   BLUE  |   RED   |  GREEN  |
// | 4  4  0 | 0  0  1 | 1  5  5 | 5  8  4 |
// | 9     1 | 1     2 | 1    10 | 10    9 |
// | 6  6  2 | 2  3  3 | 3  7  7 | 7 11  6 |
// |_________|_________|_________|_________|
//           |  WHITE  |
//           | 2  3  3 |
//           | 6     7 |
//           | 6 11  7 |
//           |_________|
//
// orientation 0:
//           ___________
//           | YELLOW  |
//           |         |
//           | e     e |
//           |         |
//  _________|_________|_________ _________
// |  ORANGE |   BLUE  |   RED   |  GREEN  |
// |         | c  e  c |         | c  e  c |
// | e     e |         | e     e |         |
// |         | c  e  c |         | c  e  c |
// |_________|_________|_________|_________|
//           |  WHITE  |
//           |         |
//           | e     e |
//           |         |
//           |_________|

const struct cube csolved = {
	.cc = cubecorners(
		0, 0,  1, 0,
		2, 0,  3, 0,
		4, 0,  5, 0,
		6, 0,  7, 0
	),
	.ce = cubeedges(
		0, 0,
		1, 0,
		2, 0,
		3, 0,
		4, 0,
		5, 0,
		6, 0,
		7, 0,
		8, 0,
		9, 0,
		10, 0,
		11, 0
	)
};

const struct cube crotated[RCOUNT] = {
	{ // ORANGECW
		.cc = cubecorners(
			4, 1,
			1, 0,
			0, 2,
			3, 0,
			6, 2,
			5, 0,
			2, 1,
			7, 0
		),
		.ce = cubeedges(
			 0, 0,
			 4, 1,
			 2, 0,
			 3, 0,
			 9, 1,
			 5, 0,
			 1, 1,
			 7, 0,
			 8, 0,
			 6, 1,
			10, 0,
			11, 0
		)
	},{ // ORANGE180
		.cc = cubecorners(
			6, 0,
			1, 0,
			4, 0,
			3, 0,
			2, 0,
			5, 0,
			0, 0,
			7, 0
		),
		.ce = cubeedges(
			 0, 0,
			 9, 0,
			 2, 0,
			 3, 0,
			 6, 0,
			 5, 0,
			 4, 0,
			 7, 0,
			 8, 0,
			 1, 0,
			10, 0,
			11, 0
		)
	},{ // ORANGECCW
		.cc = cubecorners(
			2, 1,
			1, 0,
			6, 2,
			3, 0,
			0, 2,
			5, 0,
			4, 1,
			7, 0
		),
		.ce = cubeedges(
			 0, 0,
			 6, 1,
			 2, 0,
			 3, 0,
			 1, 1,
			 5, 0,
			 9, 1,
			 7, 0,
			 8, 0,
			 4, 1,
			10, 0,
			11, 0
		)
	},{ // REDCW
		.cc = cubecorners(
			0, 0,
			3, 2,
			2, 0,
			7, 1,
			4, 0,
			1, 1,
			6, 0,
			5, 2
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 7, 1,
			 3, 0,
			 4, 0,
			 2, 1,
			 6, 0,
			10, 1,
			 8, 0,
			 9, 0,
			 5, 1,
			11, 0
		)
	},{ // RED180
		.cc = cubecorners(
			0, 0,
			7, 0,
			2, 0,
			5, 0,
			4, 0,
			3, 0,
			6, 0,
			1, 0
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			10, 0,
			 3, 0,
			 4, 0,
			 7, 0,
			 6, 0,
			 5, 0,
			 8, 0,
			 9, 0,
			 2, 0,
			11, 0
		)
	},{ // REDCCW
		.cc = cubecorners(
			0, 0,
			5, 2,
			2, 0,
			1, 1,
			4, 0,
			7, 1,
			6, 0,
			3, 2
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 5, 1,
			 3, 0,
			 4, 0,
			10, 1,
			 6, 0,
			 2, 1,
			 8, 0,
			 9, 0,
			 7, 1,
			11, 0
		)
	},{ // YELLOWCW
		.cc = cubecorners(
			1, 2,
			5, 1,
			2, 0,
			3, 0,
			0, 1,
			4, 2,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 5, 1,
			 1, 0,
			 2, 0,
			 3, 0,
			 0, 1,
			 8, 1,
			 6, 0,
			 7, 0,
			 4, 1,
			 9, 0,
			10, 0,
			11, 0
		)
	},{ // YELLOW180
		.cc = cubecorners(
			5, 0,
			4, 0,
			2, 0,
			3, 0,
			1, 0,
			0, 0,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 8, 0,
			 1, 0,
			 2, 0,
			 3, 0,
			 5, 0,
			 4, 0,
			 6, 0,
			 7, 0,
			 0, 0,
			 9, 0,
			10, 0,
			11, 0
		)
	},{ // YELLOWCCW
		.cc = cubecorners(
			4, 2,
			0, 1,
			2, 0,
			3, 0,
			5, 1,
			1, 2,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 4, 1,
			 1, 0,
			 2, 0,
			 3, 0,
			 8, 1,
			 0, 1,
			 6, 0,
			 7, 0,
			 5, 1,
			 9, 0,
			10, 0,
			11, 0
		)
	},{ // WHITECW
		.cc = cubecorners(
			0, 0,
			1, 0,
			6, 1,
			2, 2,
			4, 0,
			5, 0,
			7, 2,
			3, 1
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			 6, 1,
			 4, 0,
			 5, 0,
			11, 1,
			 3, 1,
			 8, 0,
			 9, 0,
			10, 0,
			 7, 1
		)
	},{ // WHITE180
		.cc = cubecorners(
			0, 0,
			1, 0,
			7, 0,
			6, 0,
			4, 0,
			5, 0,
			3, 0,
			2, 0
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			11, 0,
			 4, 0,
			 5, 0,
			 7, 0,
			 6, 0,
			 8, 0,
			 9, 0,
			10, 0,
			 3, 0
		)
	},{ // WHITECCW
		.cc = cubecorners(
			0, 0,
			1, 0,
			3, 1,
			7, 2,
			4, 0,
			5, 0,
			2, 2,
			6, 1
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			 7, 1,
			 4, 0,
			 5, 0,
			 3, 1,
			11, 1,
			 8, 0,
			 9, 0,
			10, 0,
			 6, 1
		)
	},{ // GREENCW
		.cc = cubecorners(
			0, 0,
			1, 0,
			2, 0,
			3, 0,
			5, 0,
			7, 0,
			4, 0,
			6, 0
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			 3, 0,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			10, 1,
			 8, 1,
			11, 1,
			 9, 1
		)
	},{ // GREEN180
		.cc = cubecorners(
			0, 0,
			1, 0,
			2, 0,
			3, 0,
			7, 0,
			6, 0,
			5, 0,
			4, 0
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			 3, 0,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			11, 0,
			10, 0,
			 9, 0,
			 8, 0
		)
	},{ // GREENCCW
		.cc = cubecorners(
			0, 0,
			1, 0,
			2, 0,
			3, 0,
			6, 0,
			4, 0,
			7, 0,
			5, 0
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			 3, 0,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			 9, 1,
			11, 1,
			 8, 1,
			10, 1
		)
	},{ // BLUECW
		.cc = cubecorners(
			2, 0,
			0, 0,
			3, 0,
			1, 0,
			4, 0,
			5, 0,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 1, 1,
			 3, 1,
			 0, 1,
			 2, 1,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			 8, 0,
			 9, 0,
			10, 0,
			11, 0
		)
	},{ // BLUE180
		.cc = cubecorners(
			3, 0,
			2, 0,
			1, 0,
			0, 0,
			4, 0,
			5, 0,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 3, 0,
			 2, 0,
			 1, 0,
			 0, 0,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			 8, 0,
			 9, 0,
			10, 0,
			11, 0
		)
	},{ // BLUECCW
		.cc = cubecorners(
			1, 0,
			3, 0,
			0, 0,
			2, 0,
			4, 0,
			5, 0,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 2, 1,
			 0, 1,
			 3, 1,
			 1, 1,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			 8, 0,
			 9, 0,
			10, 0,
			11, 0
		)
	}
};

//   Y
// O B R G      == Y O B R G W
//   W
const struct cube ctransformed[] = {
    csolved,
	{ // B W R Y O G
		.cc = cubecorners(
			3, 1,
			1, 2,
			7, 2,
			5, 1,
			2, 2,
			0, 1,
			6, 1,
			4, 2
		),
		.ce = cubeedges(
			 2, 0,
			 7, 0,
			 5, 0,
			10, 0,
			 3, 0,
			 0, 0,
			11, 0,
			 8, 0,
			 1, 0,
			 6, 0,
			 4, 0,
			 9, 0
		)
	},{ // R G Y B W O
        .cc = cubecorners(
             5, 2,
             1, 1,
             4, 1,
             0, 2,
             7, 1,
             3, 2,
             6, 2,
             2, 1
        ),
        .ce = cubeedges(
             5, 0,
             8, 0,
             0, 0,
             4, 0,
            10, 0,
             2, 0,
             9, 0,
             1, 0,
             7, 0,
            11, 0,
             3, 0,
             6, 0
        )
	},{ // O B Y G W R
		.cc = cubecorners(
			 0,  2,
			 4,  1,
			 1,  1,
			 5,  2,
			 2,  1,
			 6,  2,
			 3,  2,
			 7,  1
		),
		.ce = cubeedges(
			 4,  0,
			 0,  0,
			 8,  0,
			 5,  0,
			 1,  0,
			 9,  0,
			 2,  0,
			10,  0,
			 6,  0,
			 3,  0,
			11,  0,
			 7,  0
		)
	},{ // B Y O W R G
		.cc = cubecorners(
			 0,  1,
			 2,  2,
			 4,  2,
			 6,  1,
			 1,  2,
			 3,  1,
			 5,  1,
			 7,  2
		),
		.ce = cubeedges(
			 1,  0,
			 4,  0,
			 6,  0,
			 9,  0,
			 0,  0,
			 3,  0,
			 8,  0,
			11,  0,
			 2,  0,
			 5,  0,
			 7,  0,
			10,  0
		)
	},{ // G Y R W O B
		.cc = cubecorners(
			 5,  1,
			 7,  2,
			 1,  2,
			 3,  1,
			 4,  2,
			 6,  1,
			 0,  1,
			 2,  2
		),
		.ce = cubeedges(
			10,  0,
			 5,  0,
			 7,  0,
			 2,  0,
			 8,  0,
			11,  0,
			 0,  0,
			 3,  0,
			 9,  0,
			 4,  0,
			 6,  0,
			 1,  0
		)
	},{ // O G W B Y R
		.cc = cubecorners(
			 6,  2,
			 2,  1,
			 7,  1,
			 3,  2,
			 4,  1,
			 0,  2,
			 5,  2,
			 1,  1
		),
		.ce = cubeedges(
			 6,  0,
			11,  0,
			 3,  0,
			 7,  0,
			 9,  0,
			 1,  0,
			10,  0,
			 2,  0,
			 4,  0,
			 8,  0,
			 0,  0,
			 5,  0
		)
	},{ // G W O Y R B
		.cc = cubecorners(
			 6,  1,
			 4,  2,
			 2,  2,
			 0,  1,
			 7,  2,
			 5,  1,
			 3,  1,
			 1,  2
		),
		.ce = cubeedges(
			 9,  0,
			 6,  0,
			 4,  0,
			 1,  0,
			11,  0,
			 8,  0,
			 3,  0,
			 0,  0,
			10,  0,
			 7,  0,
			 5,  0,
			 2,  0
		)
	},{ // R B W G Y O
		.cc = cubecorners(
			 3,  2,
			 7,  1,
			 2,  1,
			 6,  2,
			 1,  1,
			 5,  2,
			 0,  2,
			 4,  1
		),
		.ce = cubeedges(
			 7,  0,
			 3,  0,
			11,  0,
			 6,  0,
			 2,  0,
			10,  0,
			 1,  0,
			 9,  0,
			 5,  0,
			 0,  0,
			 8,  0,
			 4,  0
		)
	},{ // B O W R Y G
		.cc = cubecorners(
			 2,  1,
			 3,  2,
			 6,  2,
			 7,  1,
			 0,  2,
			 1,  1,
			 4,  1,
			 5,  2
		),
		.ce = cubeedges(
			 3,  1,
			 6,  1,
			 7,  1,
			11,  1,
			 1,  1,
			 2,  1,
			 9,  1,
			10,  1,
			 0,  1,
			 4,  1,
			 5,  1,
			 8,  1
		)
	},{ // W O G R B Y
		.cc = cubecorners(
			 6,  0,
			 7,  0,
			 4,  0,
			 5,  0,
			 2,  0,
			 3,  0,
			 0,  0,
			 1,  0
		),
		.ce = cubeedges(
			11,  0,
			 9,  0,
			10,  0,
			 8,  0,
			 6,  0,
			 7,  0,
			 4,  0,
			 5,  0,
			 3,  0,
			 1,  0,
			 2,  0,
			 0,  0
		)
	},{ // G O Y R W B
		.cc = cubecorners(
			 4,  1,
			 5,  2,
			 0,  2,
			 1,  1,
			 6,  2,
			 7,  1,
			 2,  1,
			 3,  2
		),
		.ce = cubeedges(
			 8,  1,
			 4,  1,
			 5,  1,
			 0,  1,
			 9,  1,
			10,  1,
			 1,  1,
			 2,  1,
			11,  1,
			 6,  1,
			 7,  1,
			 3,  1
		)
	},{ // O W B Y G R
		.cc = cubecorners(
			 2,  0,
			 0,  0,
			 3,  0,
			 1,  0,
			 6,  0,
			 4,  0,
			 7,  0,
			 5,  0
		),
		.ce = cubeedges(
			 1,  1,
			 3,  1,
			 0,  1,
			 2,  1,
			 6,  1,
			 4,  1,
			 7,  1,
			 5,  1,
			 9,  1,
			11,  1,
			 8,  1,
			10,  1
		)
	},{ // W R B O G Y
		.cc = cubecorners(
			 3,  0,
			 2,  0,
			 1,  0,
			 0,  0,
			 7,  0,
			 6,  0,
			 5,  0,
			 4,  0
		),
		.ce = cubeedges(
			 3,  0,
			 2,  0,
			 1,  0,
			 0,  0,
			 7,  0,
			 6,  0,
			 5,  0,
			 4,  0,
			11,  0,
			10,  0,
			 9,  0,
			 8,  0
		)
	},{ // R Y B W G O
		.cc = cubecorners(
			 1,  0,
			 3,  0,
			 0,  0,
			 2,  0,
			 5,  0,
			 7,  0,
			 4,  0,
			 6,  0
		),
		.ce = cubeedges(
			 2,  1,
			 0,  1,
			 3,  1,
			 1,  1,
			 5,  1,
			 7,  1,
			 4,  1,
			 6,  1,
			10,  1,
			 8,  1,
			11,  1,
			 9,  1
		)
	},{ // Y G O B R W
		.cc = cubecorners(
			 4,  2,
			 0,  1,
			 6,  1,
			 2,  2,
			 5,  1,
			 1,  2,
			 7,  2,
			 3,  1
		),
		.ce = cubeedges(
			 4,  1,
			 9,  1,
			 1,  1,
			 6,  1,
			 8,  1,
			 0,  1,
			11,  1,
			 3,  1,
			 5,  1,
			10,  1,
			 2,  1,
			 7,  1
		)
	},{ // Y R G O B W
		.cc = cubecorners(
			 5,  0,
			 4,  0,
			 7,  0,
			 6,  0,
			 1,  0,
			 0,  0,
			 3,  0,
			 2,  0
		),
		.ce = cubeedges(
			 8,  0,
			10,  0,
			 9,  0,
			11,  0,
			 5,  0,
			 4,  0,
			 7,  0,
			 6,  0,
			 0,  0,
			 2,  0,
			 1,  0,
			 3,  0
		)
	},{ // Y B R G O W
		.cc = cubecorners(
			 1,  2,
			 5,  1,
			 3,  1,
			 7,  2,
			 0,  1,
			 4,  2,
			 2,  2,
			 6,  1
		),
		.ce = cubeedges(
			 5,  1,
			 2,  1,
			10,  1,
			 7,  1,
			 0,  1,
			 8,  1,
			 3,  1,
			11,  1,
			 4,  1,
			 1,  1,
			 9,  1,
			 6,  1
		)
	},{ // O Y G W B R
		.cc = cubecorners(
			 4,  0,
			 6,  0,
			 5,  0,
			 7,  0,
			 0,  0,
			 2,  0,
			 1,  0,
			 3,  0
		),
		.ce = cubeedges(
			 9,  1,
			 8,  1,
			11,  1,
			10,  1,
			 4,  1,
			 6,  1,
			 5,  1,
			 7,  1,
			 1,  1,
			 0,  1,
			 3,  1,
			 2,  1
		)
	},{ // W B O G R Y
		.cc = cubecorners(
			 2,  2,
			 6,  1,
			 0,  1,
			 4,  2,
			 3,  1,
			 7,  2,
			 1,  2,
			 5,  1
		),
		.ce = cubeedges(
			 6,  1,
			 1,  1,
			 9,  1,
			 4,  1,
			 3,  1,
			11,  1,
			 0,  1,
			 8,  1,
			 7,  1,
			 2,  1,
			10,  1,
			 5,  1
		)
	},{ // B R Y O W G
		.cc = cubecorners(
			 1,  1,
			 0,  2,
			 5,  2,
			 4,  1,
			 3,  2,
			 2,  1,
			 7,  1,
			 6,  2
		),
		.ce = cubeedges(
			 0,  1,
			 5,  1,
			 4,  1,
			 8,  1,
			 2,  1,
			 1,  1,
			10,  1,
			 9,  1,
			 3,  1,
			 7,  1,
			 6,  1,
			11,  1
		)
	},{ // R W G Y B O
		.cc = cubecorners(
			 7,  0,
			 5,  0,
			 6,  0,
			 4,  0,
			 3,  0,
			 1,  0,
			 2,  0,
			 0,  0
		),
		.ce = cubeedges(
			10,  1,
			11,  1,
			 8,  1,
			 9,  1,
			 7,  1,
			 5,  1,
			 6,  1,
			 4,  1,
			 2,  1,
			 3,  1,
			 0,  1,
			 1,  1
		)
	},{ // W G R B O Y
		.cc = cubecorners(
			 7,  2,
			 3,  1,
			 5,  1,
			 1,  2,
			 6,  1,
			 2,  2,
			 4,  2,
			 0,  1
		),
		.ce = cubeedges(
			 7,  1,
			10,  1,
			 2,  1,
			 5,  1,
			11,  1,
			 3,  1,
			 8,  1,
			 0,  1,
			 6,  1,
			 9,  1,
			 1,  1,
			 4,  1
		)
	},{ // G R W O Y B
		.cc = cubecorners(
			 7,  1,
			 6,  2,
			 3,  2,
			 2,  1,
			 5,  2,
			 4,  1,
			 1,  1,
			 0,  2
		),
		.ce = cubeedges(
			11,  1,
			 7,  1,
			 6,  1,
			 3,  1,
			10,  1,
			 9,  1,
			 2,  1,
			 1,  1,
			 8,  1,
			 5,  1,
			 4,  1,
			 0,  1
		)
	}
};

const int TCOUNT = sizeof(ctransformed) / sizeof(ctransformed[0]);

static int transformReverse(int idx) {
    switch( idx ) {
        case 1: return  2;
        case 2: return  1;
        case 3: return  4;
        case 4: return  3;
        case 5: return  6;
        case 6: return  5;
        case 7: return  8;
        case 8: return  7;
        case 9: return  11;
        case 11: return 9;
        case 12: return 14;
        case 14: return 12;
        case 15: return 17;
        case 17: return 15;
    }
    return idx;
}

static cube cubeTransform(const cube &c, unsigned idx)
{
    const cube &ctrans = ctransformed[idx];
    cubecorners cc1 = cubecorners::compose(ctrans.cc, c.cc);
    cubeedges ce1 = cubeedges::compose(ctrans.ce, c.ce);
    const cube &ctransRev = ctransformed[transformReverse(idx)];
    cubecorners cc2 = cubecorners::compose(cc1, ctransRev.cc);
    cubeedges ce2 = cubeedges::compose(ce1, ctransRev.ce);
	return { .cc = cc2, .ce = ce2 };
}

static cubecorner_perms cubecornerPermsTransform1(cubecorner_perms ccp, int idx)
{
    cubecorner_perms ccp1 = cubecorner_perms::compose(ctransformed[idx].cc.getPerms(), ccp);
	return cubecorner_perms::compose(ccp1,
            ctransformed[transformReverse(idx)].cc.getPerms());
}

static cubecorner_perms cubecornerPermsTransform(cubecorner_perms ccp, int idx)
{
    cubecorner_perms cctr = ctransformed[idx].cc.getPerms();
    cubecorner_perms ccrtr = ctransformed[transformReverse(idx)].cc.getPerms();
	cubecorner_perms res;
	for(int cno = 0; cno < 8; ++cno) {
		res.setAt(cno, cctr.getAt(ccp.getAt(ccrtr.getAt(cno))));
    }
    return res;
}

static cubecorner_orients cubecornerOrientsTransform(cubecorner_perms ccp, cubecorner_orients cco, int idx)
{
    cubecorner_orients cco1 = cubecorner_orients::compose(ctransformed[idx].cc.getOrients(), ccp, cco);
	return cubecorners::orientsCompose(cco1, ctransformed[transformReverse(idx)].cc);
}

cubeedges cubeedges::transform(cubeedges ce, int idx)
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

        // store cetrans perms in xmm1
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

        // store cetransRev perms in xmm2
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
        : [ce]          "r"   (ce.edges),
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
    cubeedges ce1 = cubeedges::compose(cetrans, ce);
    res = cubeedges::compose(ce1, cetransRev);
#elif 0
    cubeedges cetransRev = ctransformed[transformReverse(idx)].ce;
	cubeedges mid;
	for(int i = 0; i < 12; ++i) {
		unsigned cePerm = ce.edges >> 5 * i & 0xf;
		unsigned long ctransPerm = cetrans.edges >> 5 * cePerm & 0xf;
        mid.edges |= ctransPerm << 5 * i;
    }
    unsigned long edgeorients = ce.edges & 0x842108421084210ul;
    mid.edges |= edgeorients;
	for(int i = 0; i < 12; ++i) {
		unsigned ctransRev = cetransRev.edges >> 5 * i & 0xf;
        unsigned long ctransPerm = mid.edges >> 5 * ctransRev & 0x1f;
		res.edges |= ctransPerm << 5*i;
	}
#else
	for(int i = 0; i < 12; ++i) {
		unsigned ceItem = ce.edges >> 5 * i;
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
        std::cout << "ce.edges         = " << dumpedges(ce.edges) << std::endl;
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

static cubecorner_perms cornersIdxToPerm(unsigned short idx);

struct ReprCandidateTransform {
    bool reversed;
    bool symmetric;
    unsigned short transformIdx;
};

struct CubecornerPermToRepr {
    unsigned reprIdx;       // index in gReprPerms
    std::vector<ReprCandidateTransform> transform;
};

static std::vector<cubecorner_perms> gReprPerms;
static std::map<cubecorner_perms, CubecornerPermToRepr> gPermToRepr;

static void permReprInit()
{
    for(unsigned pidx = 0; pidx < 40320; ++pidx) {
        cubecorner_perms perm = cornersIdxToPerm(pidx);
        cubecorner_perms permRepr;
        std::vector<ReprCandidateTransform> transform;
        for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
            cubecorner_perms permr = reversed ? perm.reverse() : perm;
            for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
                cubecorner_perms permchk = symmetric ? permr.symmetric() : permr;
                for(unsigned short td = 0; td < TCOUNT; ++td) {
                    cubecorner_perms cctr = ctransformed[td].cc.getPerms();
                    cubecorner_perms ccrtr = ctransformed[transformReverse(td)].cc.getPerms();
                    cubecorner_perms cand;
                    for(int cno = 0; cno < 8; ++cno)
                        cand.setAt(cno, cctr.getAt(permchk.getAt(ccrtr.getAt(cno))));
                    if( td+reversed+symmetric == 0 || cand < permRepr ) {
                        permRepr = cand;
                        transform.clear();
                        transform.push_back({ .reversed = (bool)reversed,
                                .symmetric = (bool)symmetric, .transformIdx = td });
                    }else if( cand == permRepr ) {
                        transform.push_back({ .reversed = (bool)reversed,
                                .symmetric = (bool)symmetric, .transformIdx = td });
                    }
                }
            }
        }
        std::map<cubecorner_perms, CubecornerPermToRepr>::const_iterator it = gPermToRepr.find(permRepr);
        unsigned permReprIdx;
        if( it == gPermToRepr.end() ) {
            permReprIdx = gReprPerms.size();
            gReprPerms.push_back(permRepr);
            CubecornerPermToRepr permToRepr = { .reprIdx = permReprIdx };
            gPermToRepr[permRepr] = permToRepr;
        }else
            permReprIdx = it->second.reprIdx;
        CubecornerPermToRepr permToRepr = { .reprIdx = permReprIdx, .transform = transform };
        gPermToRepr[perm] = permToRepr;
    }
    std::cout << "repr size=" << gReprPerms.size() << std::endl;
}

static unsigned cubecornerPermRepresentativeIdx(cubecorner_perms ccp)
{
    return gPermToRepr.at(ccp).reprIdx;
}

static cubecorner_perms cubecornerPermsRepresentative(cubecorner_perms ccp)
{
    unsigned permReprIdx = gPermToRepr.at(ccp).reprIdx;
    return gReprPerms[permReprIdx];
}

struct EdgeReprCandidateTransform {
    unsigned transformedIdx;
    bool reversed;
    bool symmetric;
    cubeedges ceTrans;
};

static cubecorner_orients cubecornerOrientsRepresentative(cubecorner_perms ccp, cubecorner_orients cco,
         std::vector<EdgeReprCandidateTransform> &transform)
{
    CubecornerPermToRepr permToRepr = gPermToRepr.at(ccp);
    cubecorner_perms ccpsymm, ccprev, ccprevsymm;
    cubecorner_orients orepr, ccosymm, ccorev, ccorevsymm;
    bool isInit = false, isSymmInit = false, isRevInit = false, isRevSymmInit = false;
	transform.clear();
    for(const ReprCandidateTransform &rct : permToRepr.transform) {
        cubecorner_orients ocand;
        if( rct.reversed ) {
            if( !isRevInit ) {
                ccprev = ccp.reverse();
                ccorev = cco.reverse(ccp);
                isRevInit = true;
            }
            if( rct.symmetric ) {
                if( !isRevSymmInit ) {
                    ccprevsymm = ccprev.symmetric();
                    ccorevsymm = ccorev.symmetric();
                    isRevSymmInit = true;
                }
                ocand = cubecornerOrientsTransform(ccprevsymm, ccorevsymm, rct.transformIdx);
            }else{
                ocand = cubecornerOrientsTransform(ccprev, ccorev, rct.transformIdx);
            }
        }else{
            if( rct.symmetric ) {
                if( !isSymmInit ) {
                    ccpsymm = ccp.symmetric();
                    ccosymm = cco.symmetric();
                    isSymmInit = true;
                }
                ocand = cubecornerOrientsTransform(ccpsymm, ccosymm, rct.transformIdx);
            }else{
                ocand = cubecornerOrientsTransform(ccp, cco, rct.transformIdx);
            }
        }
        if( !isInit || ocand < orepr ) {
            orepr = ocand;
            transform.clear();
            transform.push_back({ .transformedIdx = rct.transformIdx,
                    .reversed = rct.reversed, .symmetric = rct.symmetric, .ceTrans = csolved.ce });
            isInit = true;
        }else if( isInit && ocand == orepr )
            transform.push_back({ .transformedIdx = rct.transformIdx,
                    .reversed = rct.reversed, .symmetric = rct.symmetric, .ceTrans = csolved.ce });
    }
    return orepr;
}

static cubeedges cubeedgesRepresentative(cubeedges ce, const std::vector<EdgeReprCandidateTransform> &transform)
{
    cubeedges cerepr;

    if( transform.size() == 1 ) {
        const EdgeReprCandidateTransform &erct = transform.front();
        cubeedges cechk = erct.reversed ? ce.reverse() : ce;
        if( erct.symmetric )
            cechk = cechk.symmetric();
        cerepr = cubeedges::transform(cechk, erct.transformedIdx);
    }else{
        cubeedges cesymm, cerev, cerevsymm;
        bool isInit = false, isSymmInit = false, isRevInit = false, isRevSymmInit = false;
        for(const EdgeReprCandidateTransform &erct : transform) {
            cubeedges cand;
            if( erct.reversed ) {
                if( ! isRevInit ) {
                    cerev = ce.reverse();
                    isRevInit = true;
                }
                if( erct.symmetric ) {
                    if( ! isRevSymmInit ) {
                        cerevsymm = cerev.symmetric();
                        isRevSymmInit = true;
                    }
                    cand = cubeedges::transform(cerevsymm, erct.transformedIdx);
                }else{
                    cand = cubeedges::transform(cerev, erct.transformedIdx);
                }
            }else{
                if( erct.symmetric ) {
                    if( ! isSymmInit ) {
                        cesymm = ce.symmetric();
                        isSymmInit = true;
                    }
                    cand = cubeedges::transform(cesymm, erct.transformedIdx);
                }else{
                    cand = cubeedges::transform(ce, erct.transformedIdx);
                }
            }
            if( !isInit || cand < cerepr )
                cerepr = cand;
            isInit = true;
        }
    }
    return cerepr;
}

/* Prepares to get representative cubeedges for a list of cubes having common
 * cubecorners (cc1) with a cube c2.
 * Parameters:
 *    ccpComposed, ccoComposed - cubecorners of the common cc1 composed with c2.cc
 *    ce2                      - c2.ce
 *    transform                - output list to pass later to cubeedgesComposedRepresentative()
 */
static cubecorner_orients cubecornerOrientsComposedRepresentative(
        cubecorner_perms ccpComposed, cubecorner_orients ccoComposed,
        cubeedges ce2, std::vector<EdgeReprCandidateTransform> &transform)
{
    CubecornerPermToRepr permToRepr = gPermToRepr.at(ccpComposed);
    cubecorner_perms ccpsymm, ccprev, ccprevsymm;
    cubecorner_orients orepr, ccosymm, ccorev, ccorevsymm;
    bool isInit = false, isSymmInit = false, isRevInit = false, isRevSymmInit = false;
	transform.clear();
    for(const ReprCandidateTransform &rct : permToRepr.transform) {
        cubecorner_orients ocand;
        if( rct.reversed ) {
            if( !isRevInit ) {
                ccprev = ccpComposed.reverse();
                ccorev = ccoComposed.reverse(ccpComposed);
                isRevInit = true;
            }
            if( rct.symmetric ) {
                if( !isRevSymmInit ) {
                    ccprevsymm = ccprev.symmetric();
                    ccorevsymm = ccorev.symmetric();
                    isRevSymmInit = true;
                }
                ocand = cubecornerOrientsTransform(ccprevsymm, ccorevsymm, rct.transformIdx);
            }else{
                ocand = cubecornerOrientsTransform(ccprev, ccorev, rct.transformIdx);
            }
        }else{
            if( rct.symmetric ) {
                if( !isSymmInit ) {
                    ccpsymm = ccpComposed.symmetric();
                    ccosymm = ccoComposed.symmetric();
                    isSymmInit = true;
                }
                ocand = cubecornerOrientsTransform(ccpsymm, ccosymm, rct.transformIdx);
            }else{
                ocand = cubecornerOrientsTransform(ccpComposed, ccoComposed, rct.transformIdx);
            }
        }
        if( !isInit || ocand < orepr ) {
            orepr = ocand;
            transform.clear();
            transform.push_back({ .transformedIdx = rct.transformIdx,
                    .reversed = rct.reversed, .symmetric = rct.symmetric });
            isInit = true;
        }else if( isInit && ocand == orepr )
            transform.push_back({ .transformedIdx = rct.transformIdx,
                    .reversed = rct.reversed, .symmetric = rct.symmetric });
    }
    for(EdgeReprCandidateTransform &erct : transform) {
        cubeedges ce2chk = erct.symmetric ? ce2.symmetric() : ce2;
        if( erct.reversed ) {
            // transform((ce2 rev) ⊙  (ce1 rev)) = cetrans ⊙  (ce2 rev) ⊙  (ce1 rev) ⊙  cetransRev
            // preparing cetrans ⊙  (ce2 rev)
            erct.ceTrans = cubeedges::compose(
                    ctransformed[erct.transformedIdx].ce, ce2chk.reverse());
        }else{
            // transform(ce1 ⊙  ce2) = cetrans ⊙  ce1 ⊙  ce2 ⊙  cetransRev
            // preparing ce2 ⊙  cetransRev
            erct.ceTrans = cubeedges::compose(ce2chk,
                    ctransformed[transformReverse(erct.transformedIdx)].ce);
        }
    }
    return orepr;
}

static cubeedges cubeedgesComposedRepresentative(cubeedges ce, const std::vector<EdgeReprCandidateTransform> &transform)
{
    cubeedges cerepr, cesymm, cerev, cerevsymm;
    bool isInit = false, isSymmInit = false, isRevInit = false, isRevSymmInit = false;
    for(const EdgeReprCandidateTransform &erct : transform) {
        cubeedges cand;
        if( erct.reversed ) {
            if( ! isRevInit ) {
                cerev = ce.reverse();
                isRevInit = true;
            }
            if( erct.symmetric ) {
                if( ! isRevSymmInit ) {
                    cerevsymm = cerev.symmetric();
                    isRevSymmInit = true;
                }
                cand = cubeedges::compose3(erct.ceTrans, cerevsymm,
                    ctransformed[transformReverse(erct.transformedIdx)].ce);
            }else{
                cand = cubeedges::compose3(erct.ceTrans, cerev,
                    ctransformed[transformReverse(erct.transformedIdx)].ce);
            }
        }else{
            if( erct.symmetric ) {
                if( ! isSymmInit ) {
                    cesymm = ce.symmetric();
                    isSymmInit = true;
                }
                cand = cubeedges::compose3(ctransformed[erct.transformedIdx].ce,
                        cesymm, erct.ceTrans);
            }else{
                cand = cubeedges::compose3(ctransformed[erct.transformedIdx].ce,
                        ce, erct.ceTrans);
            }
        }
        if( !isInit || cand < cerepr )
            cerepr = cand;
        isInit = true;
    }
    return cerepr;
}

static cube cubeRepresentative(const cube &c) {
    std::vector<EdgeReprCandidateTransform> transform;

    cubecorner_perms ccpRepr = cubecornerPermsRepresentative(c.cc.getPerms());
    cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(c.cc.getPerms(),
            c.cc.getOrients(), transform);
    cubeedges ceRepr = cubeedgesRepresentative(c.ce, transform);
    return { .cc = cubecorners(ccpRepr, ccoRepr), .ce = ceRepr };
}

static void cubePrint(const cube &c)
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
			colorPrint[cubeCornerColors[c.cc.getPermAt(4)][R120[c.cc.getOrientAt(4)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(8)][!c.ce.edgeR(8)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(5)][R240[c.cc.getOrientAt(5)]]]);
	printf("        %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.edgeN(4)][c.ce.edgeR(4)]],
			colorPrint[CYELLOW],
			colorPrint[cubeEdgeColors[c.ce.edgeN(5)][c.ce.edgeR(5)]]);
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.cc.getPermAt(0)][R240[c.cc.getOrientAt(0)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(0)][!c.ce.edgeR(0)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(1)][R120[c.cc.getOrientAt(1)]]]);
	printf("\n");
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeCornerColors[c.cc.getPermAt(4)][R240[c.cc.getOrientAt(4)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(4)][!c.ce.edgeR(4)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(0)][R120[c.cc.getOrientAt(0)]]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(0)][c.cc.getOrientAt(0)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(0)][c.ce.edgeR(0)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(1)][c.cc.getOrientAt(1)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(1)][R240[c.cc.getOrientAt(1)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(5)][!c.ce.edgeR(5)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(5)][R120[c.cc.getOrientAt(5)]]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(5)][c.cc.getOrientAt(5)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(8)][c.ce.edgeR(8)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(4)][c.cc.getOrientAt(4)]]);
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.edgeN(9)][c.ce.edgeR(9)]],
			colorPrint[CORANGE],
			colorPrint[cubeEdgeColors[c.ce.edgeN(1)][c.ce.edgeR(1)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(1)][!c.ce.edgeR(1)]],
			colorPrint[CBLUE],
			colorPrint[cubeEdgeColors[c.ce.edgeN(2)][!c.ce.edgeR(2)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(2)][c.ce.edgeR(2)]],
			colorPrint[CRED],
			colorPrint[cubeEdgeColors[c.ce.edgeN(10)][c.ce.edgeR(10)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(10)][!c.ce.edgeR(10)]],
			colorPrint[CGREEN],
			colorPrint[cubeEdgeColors[c.ce.edgeN(9)][!c.ce.edgeR(9)]]);
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeCornerColors[c.cc.getPermAt(6)][R120[c.cc.getOrientAt(6)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(6)][!c.ce.edgeR(6)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(2)][R240[c.cc.getOrientAt(2)]]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(2)][c.cc.getOrientAt(2)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(3)][c.ce.edgeR(3)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(3)][c.cc.getOrientAt(3)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(3)][R120[c.cc.getOrientAt(3)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(7)][!c.ce.edgeR(7)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(7)][R240[c.cc.getOrientAt(7)]]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(7)][c.cc.getOrientAt(7)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(11)][c.ce.edgeR(11)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(6)][c.cc.getOrientAt(6)]]);
	printf("\n");
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.cc.getPermAt(2)][R120[c.cc.getOrientAt(2)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(3)][!c.ce.edgeR(3)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(3)][R240[c.cc.getOrientAt(3)]]]);
	printf("        %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.edgeN(6)][c.ce.edgeR(6)]],
			colorPrint[CWHITE],
			colorPrint[cubeEdgeColors[c.ce.edgeN(7)][c.ce.edgeR(7)]]);
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.cc.getPermAt(6)][R240[c.cc.getOrientAt(6)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(11)][!c.ce.edgeR(11)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(7)][R120[c.cc.getOrientAt(7)]]]);
	printf("\n");
}

static bool isCubeSolvable(int reqFd, const cube &c)
{
    bool isSwapsOdd = false;
    unsigned permsScanned = 0;
    for(int i = 0; i < 8; ++i) {
        if( permsScanned & 1 << i )
            continue;
        permsScanned |= 1 << i;
        int p = i;
        while( (p = c.cc.getPermAt(p)) != i ) {
            if( permsScanned & 1 << p ) {
                sendRespMessage(reqFd, "corner perm %d is twice\n", p);
                return false;
            }
            permsScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
    permsScanned = 0;
    for(int i = 0; i < 12; ++i) {
        if( permsScanned & 1 << i )
            continue;
        permsScanned |= 1 << i;
        int p = i;
        while( (p = c.ce.edgeN(p)) != i ) {
            if( permsScanned & 1 << p ) {
                sendRespMessage(reqFd, "edge perm %d is twice\n", p);
                return false;
            }
            permsScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
	if( isSwapsOdd ) {
		sendRespMessage(reqFd, "cube unsolvable due to permutation parity\n");
		return false;
	}
	unsigned sumOrient = 0;
	for(int i = 0; i < 8; ++i)
		sumOrient += c.cc.getOrientAt(i);
	if( sumOrient % 3 ) {
		sendRespMessage(reqFd, "cube unsolvable due to corner orientations\n");
		return false;
	}
	sumOrient = 0;
	for(int i = 0; i < 12; ++i)
		sumOrient += c.ce.edgeR(i);
	if( sumOrient % 2 ) {
		sendRespMessage(reqFd, "cube unsolvable due to edge orientations\n");
		return false;
	}
    return true;
}

struct elemLoc {
	int wall;
    int row;
	int col;
};

static bool cubeRead(int reqFd, const char *squareColors, cube &c)
{
	int walls[6][3][3];
	const char colorLetters[] = "YOBRGW";
	static const elemLoc cornerLocMap[8][3] = {
		{ { 2, 0, 0 }, { 1, 0, 2 }, { 0, 2, 0 } },
		{ { 2, 0, 2 }, { 0, 2, 2 }, { 3, 0, 0 } },

		{ { 2, 2, 0 }, { 5, 0, 0 }, { 1, 2, 2 } },
		{ { 2, 2, 2 }, { 3, 2, 0 }, { 5, 0, 2 } },

		{ { 4, 0, 2 }, { 0, 0, 0 }, { 1, 0, 0 } },
		{ { 4, 0, 0 }, { 3, 0, 2 }, { 0, 0, 2 } },

		{ { 4, 2, 2 }, { 1, 2, 0 }, { 5, 2, 0 } },
		{ { 4, 2, 0 }, { 5, 2, 2 }, { 3, 2, 2 } },
	};
	static const elemLoc edgeLocMap[12][2] = {
		{ { 2, 0, 1 }, { 0, 2, 1 } },
		{ { 1, 1, 2 }, { 2, 1, 0 } },
		{ { 3, 1, 0 }, { 2, 1, 2 } },
		{ { 2, 2, 1 }, { 5, 0, 1 } },

		{ { 0, 1, 0 }, { 1, 0, 1 } },
		{ { 0, 1, 2 }, { 3, 0, 1 } },
		{ { 5, 1, 0 }, { 1, 2, 1 } },
		{ { 5, 1, 2 }, { 3, 2, 1 } },

		{ { 4, 0, 1 }, { 0, 0, 1 } },
		{ { 1, 1, 0 }, { 4, 1, 2 } },
		{ { 3, 1, 2 }, { 4, 1, 0 } },
		{ { 4, 2, 1 }, { 5, 2, 1 } },
	};
    for(int cno = 0; cno < 54; ++cno) {
        const char *lpos = strchr(colorLetters, toupper(squareColors[cno]));
        if(lpos == NULL) {
            sendRespMessage(reqFd, "bad letter at column %d\n", cno);
            return false;
        }
        walls[cno / 9][cno % 9 / 3][cno % 3] = lpos - colorLetters;
    }
	for(int i = 0; i < 6; ++i) {
		if( walls[i][1][1] != i ) {
			sendRespMessage(reqFd, "bad orientation: wall=%d exp=%c is=%c\n",
					i, colorLetters[i], colorLetters[walls[i][1][1]]);
			return false;
		}
	}
	for(int i = 0; i < 8; ++i) {
		bool match = false;
		for(int n = 0; n < 8; ++n) {
			const int *elemColors = cubeCornerColors[n];
			match = true;
			for(int r = 0; r < 3 && match; ++r) {
				const elemLoc &el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[r] )
					match = false;
			}
			if( match ) {
				c.cc.setPermAt(i, n);
				c.cc.setOrientAt(i, 0);
				break;
			}
			match = true;
			for(int r = 0; r < 3 && match; ++r) {
				const elemLoc &el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[R120[r]] )
					match = false;
			}
			if( match ) {
				c.cc.setPermAt(i, n);
				c.cc.setOrientAt(i, 1);
				break;
			}
			match = true;
			for(int r = 0; r < 3 && match; ++r) {
				const elemLoc &el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[R240[r]] )
					match = false;
			}
			if( match ) {
				c.cc.setPermAt(i, n);
				c.cc.setOrientAt(i, 2);
				break;
			}
		}
		if( ! match ) {
			sendRespMessage(reqFd, "corner %d not found\n", i);
			return false;
		}
		for(int j = 0; j < i; ++j) {
			if( c.cc.getPermAt(i) == c.cc.getPermAt(j) ) {
				sendRespMessage(reqFd, "corner %d is twice: at %d and %d\n",
						c.cc.getPermAt(i), j, i);
				return false;
			}
		}
	}
	for(int i = 0; i < 12; ++i) {
		bool match = false;
		for(int n = 0; n < 12; ++n) {
			const int *elemColors = cubeEdgeColors[n];
			match = true;
			for(int r = 0; r < 2 && match; ++r) {
				const elemLoc &el = edgeLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[r] )
					match = false;
			}
			if( match ) {
				c.ce.setAt(i, n, 0);
				break;
			}
			match = true;
			for(int r = 0; r < 2 && match; ++r) {
				const elemLoc &el = edgeLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[!r] )
					match = false;
			}
			if( match ) {
				c.ce.setAt(i, n, 1);
				break;
			}
		}
		if( ! match ) {
			sendRespMessage(reqFd, "edge %d not found\n", i);
			return false;
		}
		for(int j = 0; j < i; ++j) {
			if( c.ce.edgeN(i) == c.ce.edgeN(j) ) {
				sendRespMessage(reqFd, "edge %d is twice: at %d and %d\n",
						c.ce.edgeN(i), j, i);
				return false;
			}
		}
	}
    if( ! isCubeSolvable(reqFd, c) )
        return false;
    return true;
}

static cubecorner_perms cornersIdxToPerm(unsigned short idx)
{
	unsigned unused = 0x76543210;
	cubecorner_perms ccp;

	for(unsigned cornerIdx = 8; cornerIdx > 0; --cornerIdx) {
		unsigned p = idx % cornerIdx * 4;
		ccp.setAt(8-cornerIdx, unused >> p & 0xf);
		unsigned m = -1 << p;
		unused = unused & ~m | unused >> 4 & m;
		idx /= cornerIdx;
	}
	return ccp;
}

static unsigned short cornersPermToIdx(cubecorner_perms ccp)
{
	unsigned short res = 0;
	unsigned indexes = 0;
	for(int i = 7; i >= 0; --i) {
		unsigned p = ccp.getAt(i) * 4;
		res = (8-i) * res + (indexes >> p & 0xf);
		indexes += 0x11111111 << p;
	}
	return res;
}

static cubecorner_orients cornersIdxToOrient(unsigned short idx)
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

static unsigned short cornersOrientToIdx(cubecorner_orients cco)
{
	unsigned short res = 0;
	for(unsigned i = 0; i < 7; ++i)
		res = res * 3 + cco.getAt(i);
	return res;
}

static cubeedges edgesIdxToPerm(unsigned idx)
{
    unsigned long unused = 0xba9876543210ul;
    cubeedges ce;

    for(unsigned edgeIdx = 12; edgeIdx > 0; --edgeIdx) {
        unsigned p = idx % edgeIdx * 4;
        ce.setPermAt(12-edgeIdx, unused >> p & 0xf);
        unsigned long m = -1ul << p;
        unused = unused & ~m | unused >> 4 & m;
        idx /= edgeIdx;
    }
    return ce;
}

static std::string getSetup() {
    std::ostringstream res;
    res << "setup: depth " << DEPTH_MAX;
    if( USEREVERSE )
        res << " rev";
#ifdef USE_ASM
    res << " asm";
#endif
#ifdef ASMCHECK
    res << " with check";
#endif
    return res.str();
}

class CornerOrientReprCubes {
	std::vector<cubeedges> m_items;
    std::vector<unsigned> m_orientOccur;
public:
    typedef std::vector<cubeedges>::const_iterator edges_iter;
    void initOccur() { m_orientOccur.resize(64); }
	bool addCube(cubeedges);
	bool containsCubeEdges(cubeedges) const;
    bool empty() const { return m_items.empty(); }
    size_t size() const { return m_items.size(); }
    edges_iter edgeBegin() const { return m_items.begin(); }
    edges_iter edgeEnd() const { return m_items.end(); }
    static cubeedges findSolutionEdge(
            const CornerOrientReprCubes &ccoReprCubes,
            const CornerOrientReprCubes &ccoReprSearchCubes,
            const std::vector<EdgeReprCandidateTransform> &otransform,
            bool edgeReverse);
    static cubeedges findSolutionEdge(
            const CornerOrientReprCubes &ccoReprCubes,
            const CornerOrientReprCubes &ccoReprSearchCubes,
            const EdgeReprCandidateTransform&,
            bool edgeReverse);
};

bool CornerOrientReprCubes::addCube(cubeedges ce)
{
	unsigned lo = 0, hi = m_items.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_items[mi] < ce )
			lo = mi + 1;
		else
			hi = mi;
	}
	if( lo < m_items.size() && m_items[lo] == ce )
		return false;
	if( lo == m_items.size() ) {
		m_items.push_back(ce);
	}else{
		m_items.resize(m_items.size()+1);
        std::copy_backward(m_items.begin()+lo, m_items.end()-1, m_items.end());
		m_items[lo] = ce;
	}
    if( ! m_orientOccur.empty() ) {
        unsigned short orientIdx = ce.getOrientIdx();
        m_orientOccur[orientIdx >> 5] |= 1ul << (orientIdx & 0x1f);
    }
	return true;
}

bool CornerOrientReprCubes::containsCubeEdges(cubeedges ce) const
{
    if( ! m_orientOccur.empty() ) {
        unsigned short orientIdx = ce.getOrientIdx();
        if( (m_orientOccur[orientIdx >> 5] & 1ul << (orientIdx & 0x1f)) == 0 )
            return false;
    }
	unsigned lo = 0, hi = m_items.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_items[mi] < ce )
			lo = mi + 1;
		else
			hi = mi;
	}
	return lo < m_items.size() && m_items[lo] == ce;
}

cubeedges CornerOrientReprCubes::findSolutionEdge(
        const CornerOrientReprCubes &ccoReprCubes,
        const CornerOrientReprCubes &ccoReprSearchCubes,
        const std::vector<EdgeReprCandidateTransform> &otransform,
        bool edgeReverse)
{
    for(CornerOrientReprCubes::edges_iter edgeIt = ccoReprCubes.edgeBegin();
            edgeIt != ccoReprCubes.edgeEnd(); ++edgeIt)
    {
        const cubeedges ce = edgeReverse ? edgeIt->reverse() : *edgeIt;
        cubeedges ceSearchRepr = cubeedgesComposedRepresentative(ce, otransform);
        if( ccoReprSearchCubes.containsCubeEdges(ceSearchRepr) )
            return ce;
    }
    return cubeedges();
}

cubeedges CornerOrientReprCubes::findSolutionEdge(
        const CornerOrientReprCubes &ccoReprCubes,
        const CornerOrientReprCubes &ccoReprSearchCubes,
        const EdgeReprCandidateTransform &erct,
        bool edgeReverse)
{
    cubeedges cetrans = ctransformed[erct.transformedIdx].ce;
    cubeedges cetransRev = ctransformed[transformReverse(erct.transformedIdx)].ce;
    for(CornerOrientReprCubes::edges_iter edgeIt = ccoReprCubes.edgeBegin();
            edgeIt != ccoReprCubes.edgeEnd(); ++edgeIt)
    {
        cubeedges ce = *edgeIt;
        cubeedges ces = erct.symmetric ? ce.symmetric() : ce;
        cubeedges ceSearchRepr;
        if( erct.reversed ) {
            if( edgeReverse )
                ceSearchRepr = cubeedges::compose3(erct.ceTrans, ces, cetransRev);
            else
                ceSearchRepr = cubeedges::compose3revmid(erct.ceTrans, ces, cetransRev);
        }else{
            if( edgeReverse )
                ceSearchRepr = cubeedges::compose3revmid(cetrans, ces, erct.ceTrans);
            else
                ceSearchRepr = cubeedges::compose3(cetrans, ces, erct.ceTrans);
        }
        if( ccoReprSearchCubes.containsCubeEdges(ceSearchRepr) )
            return edgeReverse ? ce.reverse() : ce;
    }
    return cubeedges();
}

class CornerPermReprCubes {
    std::vector<std::pair<cubecorner_orients, CornerOrientReprCubes>> m_coreprCubes;
    unsigned short m_idxmap[2187];
    bool m_initOccur;

public:
    typedef std::vector<std::pair<cubecorner_orients, CornerOrientReprCubes>>::const_iterator ccocubes_iter;
	CornerPermReprCubes();
	~CornerPermReprCubes();
    bool empty() const { return m_coreprCubes.size() == 1; }
    size_t size() const;
    void initOccur();
	const CornerOrientReprCubes &cornerOrientCubesAt(unsigned corientIdx) const {
        return m_coreprCubes[m_idxmap[corientIdx]].second;
    }
	CornerOrientReprCubes &cornerOrientCubesAdd(unsigned corientIdx);
    ccocubes_iter ccoCubesBegin() const { return m_coreprCubes.begin()+1; }
    ccocubes_iter ccoCubesEnd() const { return m_coreprCubes.end(); }
};

CornerPermReprCubes::CornerPermReprCubes()
    : m_initOccur(false)
{
    std::fill_n(m_idxmap, 2187, 0);
    m_coreprCubes.push_back(std::make_pair(cubecorner_orients(), CornerOrientReprCubes()));
}

CornerPermReprCubes::~CornerPermReprCubes()
{
}

size_t CornerPermReprCubes::size() const {
    size_t res = 0;
    for(ccocubes_iter it = ccoCubesBegin(); it != ccoCubesEnd(); ++it)
        res += it->second.size();
    return res;
}

void CornerPermReprCubes::initOccur() {
    for(unsigned i = 1; i < m_coreprCubes.size(); ++i)
        m_coreprCubes[i].second.initOccur();
    m_initOccur = true;
}

CornerOrientReprCubes &CornerPermReprCubes::cornerOrientCubesAdd(unsigned corientIdx) {
    if( m_idxmap[corientIdx] == 0 ) {
        m_idxmap[corientIdx] = m_coreprCubes.size();
        cubecorner_orients cco = cornersIdxToOrient(corientIdx);
        m_coreprCubes.push_back(std::make_pair(cco, CornerOrientReprCubes()));
        if( m_initOccur )
            m_coreprCubes.back().second.initOccur();
    }
    return m_coreprCubes[m_idxmap[corientIdx]].second;
}

class CubesReprAtDepth {
    std::vector<std::pair<cubecorner_perms, CornerPermReprCubes>> m_cornerPermReprCubes;
public:
    typedef std::vector<std::pair<cubecorner_perms, CornerPermReprCubes>>::const_iterator ccpcubes_iter;
    CubesReprAtDepth();
    CubesReprAtDepth(const CubesReprAtDepth&) = delete;
    ~CubesReprAtDepth();
    bool empty() const;
    size_t size() const;
    void initOccur();
    CornerPermReprCubes &add(unsigned idx);
    const CornerPermReprCubes &getAt(unsigned idx) const {
        return m_cornerPermReprCubes[idx].second;
    }
    ccpcubes_iter ccpCubesBegin() const { return m_cornerPermReprCubes.begin(); }
    ccpcubes_iter ccpCubesEnd() const { return m_cornerPermReprCubes.end(); }
};

CubesReprAtDepth::CubesReprAtDepth()
    : m_cornerPermReprCubes(USEREVERSE ? 654 : 984)
{
    for(unsigned i = 0; i < m_cornerPermReprCubes.size(); ++i)
        m_cornerPermReprCubes[i].first = gReprPerms[i];
}

CubesReprAtDepth::~CubesReprAtDepth() {
}

bool CubesReprAtDepth::empty() const {
    for(const std::pair<cubecorner_perms, CornerPermReprCubes> &reprCube : m_cornerPermReprCubes)
        if( !reprCube.second.empty() )
            return false;
    return true;
}

size_t CubesReprAtDepth::size() const {
    size_t res = 0;
    for(const std::pair<cubecorner_perms, CornerPermReprCubes> &reprCube : m_cornerPermReprCubes)
        res += reprCube.second.size();
    return res;
}

void CubesReprAtDepth::initOccur()
{
    for(std::pair<cubecorner_perms, CornerPermReprCubes> &reprCube : m_cornerPermReprCubes)
        reprCube.second.initOccur();
}

CornerPermReprCubes &CubesReprAtDepth::add(unsigned idx) {
    return m_cornerPermReprCubes[idx].second;
}

static std::string printMoves(const CubesReprAtDepth *cubesByDepth, const cube &c, bool movesRev = false)
{
	std::vector<int> rotateDirs;
    std::vector<int>::iterator insertPos = rotateDirs.end();
    cube crepr = cubeRepresentative(c);
    unsigned ccpReprIdx = cubecornerPermRepresentativeIdx(crepr.cc.getPerms());
    unsigned corientIdx = cornersOrientToIdx(crepr.cc.getOrients());
	unsigned depth = 0;
	while( true ) {
        const CornerPermReprCubes &ccpReprCubes = cubesByDepth[depth].getAt(ccpReprIdx);
        const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(corientIdx);
		if( ccoReprCubes.containsCubeEdges(crepr.ce) )
			break;
		++depth;
        if( depth > DEPTH_MAX ) {
            std::cout << "printMoves: depth reached maximum, cube NOT FOUND" << std::endl;
            exit(1);
        }
	}
	cube cc = c;
	while( depth-- > 0 ) {
		int cm = 0;
        cube ccRev = cc.reverse();
		cube cc1;
		while( cm < RCOUNT ) {
			cc1 = cube::compose(cc, crotated[cm]);
            cube cc1repr = cubeRepresentative(cc1);
            ccpReprIdx = cubecornerPermRepresentativeIdx(cc1repr.cc.getPerms());
            corientIdx = cornersOrientToIdx(cc1repr.cc.getOrients());
            const CornerPermReprCubes &ccpReprCubes = cubesByDepth[depth].getAt(ccpReprIdx);
            const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(corientIdx);
            if( ccoReprCubes.containsCubeEdges(cc1repr.ce) )
                break;
			cc1 = cube::compose(ccRev, crotated[cm]);
            cc1repr = cubeRepresentative(cc1);
            ccpReprIdx = cubecornerPermRepresentativeIdx(cc1repr.cc.getPerms());
            corientIdx = cornersOrientToIdx(cc1repr.cc.getOrients());
            const CornerPermReprCubes &ccpReprCubesRev = cubesByDepth[depth].getAt(ccpReprIdx);
            const CornerOrientReprCubes &ccoReprCubesRev = ccpReprCubesRev.cornerOrientCubesAt(corientIdx);
            if( ccoReprCubesRev.containsCubeEdges(cc1repr.ce) ) {
                movesRev = !movesRev;
                break;
            }
			++cm;
		}
        if( cm == RCOUNT ) {
            std::cout << "printMoves: cube at depth " << depth << " NOT FOUND" << std::endl;
            exit(1);
        }
		insertPos = rotateDirs.insert(insertPos, movesRev ? cm : rotateDirReverse(cm));
        if( movesRev )
            ++insertPos;
		cc = cc1;
	}
    std::string res;
	for(auto rotateDir : rotateDirs) {
		res += " ";
        res += rotateDirName(rotateDir);
    }
    return res;
}

class ProgressBase {
    static std::mutex m_progressMutex;
    static bool m_isCancelRequested;
protected:
    static void mutexLock();
    static bool isCancelRequestedNoLock() { return m_isCancelRequested; }
    static void mutexUnlock();
public:
    static void requestCancel();
    static void requestUncancel();
    static bool isCancelRequested();
};

std::mutex ProgressBase::m_progressMutex;
bool ProgressBase::m_isCancelRequested;

void ProgressBase::mutexLock() {
    m_progressMutex.lock();
}

void ProgressBase::mutexUnlock() {
    m_progressMutex.unlock();
}

void ProgressBase::requestCancel()
{
    mutexLock();
    m_isCancelRequested = true;
    mutexUnlock();
}

void ProgressBase::requestUncancel()
{
    mutexLock();
    m_isCancelRequested = false;
    mutexUnlock();
}

bool ProgressBase::isCancelRequested()
{
    mutexLock();
    bool res = m_isCancelRequested;
    mutexUnlock();
    return res;
}

class AddCubesProgress : public ProgressBase {
    unsigned long m_cubeCount;
    const unsigned m_depth;
    const unsigned m_processCount;
    unsigned m_processedCount;
    unsigned m_runningThreadCount;
    bool m_isFinish;

public:
    AddCubesProgress(unsigned itemCount, unsigned threadCount, unsigned depth)
        : m_cubeCount(0), m_depth(depth), m_processCount(itemCount*threadCount),
          m_processedCount(0), m_runningThreadCount(threadCount), m_isFinish(false)
    {
    }

    bool inc(int reqFd, unsigned long cubeCount);
    void threadFinished(int reqFd);
};

bool AddCubesProgress::inc(int reqFd, unsigned long cubeCount)
{
    unsigned long cubeCountTot;
    unsigned processedCount;
    bool isFinish;
    bool isCancelRequested;
    mutexLock();
    cubeCountTot = m_cubeCount += cubeCount;
    processedCount = ++m_processedCount;
    isFinish = m_isFinish;
    isCancelRequested = isCancelRequestedNoLock();
    mutexUnlock();
    if( m_depth >= 9 && !isFinish ) {
        unsigned procCountNext = 100 * (processedCount+1) / m_processCount;
        unsigned procCountCur = 100 * processedCount / m_processCount;
        if( procCountNext != procCountCur && (m_depth >= 10 || procCountCur % 10 == 0) )
            sendRespMessage(reqFd, "progress: depth %u cubes %llu, %u%%\n",
                    m_depth, cubeCountTot, 100 * processedCount / m_processCount);
    }
    return isCancelRequested;
}

void AddCubesProgress::threadFinished(int reqFd)
{
    unsigned runningThreadCount;
    mutexLock();
    runningThreadCount = --m_runningThreadCount;
    m_isFinish = true;
    mutexUnlock();
    if( m_depth >= 9 ) {
        sendRespMessage(reqFd, "progress: depth %d cubes %d threads still running\n",
                m_depth, runningThreadCount);
    }
}

static void addCubesT(
        CubesReprAtDepth *cubesReprByDepth,
		int depth, unsigned threadNo, int threadCount, int reqFd, AddCubesProgress *addCubesProgress)
{
    std::vector<EdgeReprCandidateTransform> otransformNew;

    const CubesReprAtDepth &ccpReprCubesC = cubesReprByDepth[depth-1];
    for(CubesReprAtDepth::ccpcubes_iter ccpCubesIt = ccpReprCubesC.ccpCubesBegin();
            ccpCubesIt != ccpReprCubesC.ccpCubesEnd(); ++ccpCubesIt)
    {
        const CornerPermReprCubes &cpermReprCubesC = ccpCubesIt->second;
        cubecorner_perms ccp = ccpCubesIt->first;
        if( !cpermReprCubesC.empty() ) {
            unsigned long cubeCount = 0;
            for(int rd = 0; rd < RCOUNT; ++rd) {
                for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
                    cubecorner_perms ccpNew = reversed ?
                        cubecorner_perms::compose(crotated[rd].cc.getPerms(), ccp) :
                        cubecorner_perms::compose(ccp, crotated[rd].cc.getPerms());
                    unsigned cornerPermReprIdxNew = cubecornerPermRepresentativeIdx(ccpNew);
                    if( cornerPermReprIdxNew % threadCount == threadNo ) {
                        const CornerPermReprCubes *ccpReprCubesNewP = depth == 1 ? NULL : &cubesReprByDepth[depth-2].getAt(cornerPermReprIdxNew);
                        const CornerPermReprCubes &ccpReprCubesNewC = cubesReprByDepth[depth-1].getAt(cornerPermReprIdxNew);
                        for(CornerPermReprCubes::ccocubes_iter ccoCubesItC = cpermReprCubesC.ccoCubesBegin();
                                ccoCubesItC != cpermReprCubesC.ccoCubesEnd(); ++ccoCubesItC)
                        {
                            const CornerOrientReprCubes &corientReprCubesC = ccoCubesItC->second;
                            cubecorner_orients cco = ccoCubesItC->first;
                            cubecorner_orients ccoNew = reversed ?
                                cubecorner_orients::compose(crotated[rd].cc.getOrients(), ccp, cco) :
                                cubecorners::orientsCompose(cco, crotated[rd].cc);
                            cubecorner_orients ccoReprNew = cubecornerOrientsRepresentative(ccpNew, ccoNew, otransformNew);
                            unsigned corientIdxNew = cornersOrientToIdx(ccoReprNew);
                            const CornerOrientReprCubes *corientReprCubesNewP = ccpReprCubesNewP == NULL ? NULL :
                                &ccpReprCubesNewP->cornerOrientCubesAt(corientIdxNew);
                            const CornerOrientReprCubes &corientReprCubesNewC = ccpReprCubesNewC.cornerOrientCubesAt(corientIdxNew);
                            std::vector<cubeedges> ceNewArr;
                            for(CornerOrientReprCubes::edges_iter edgeIt = corientReprCubesC.edgeBegin();
                                    edgeIt != corientReprCubesC.edgeEnd(); ++edgeIt)
                            {
                                const cubeedges ce = *edgeIt;
                                cubeedges cenew = reversed ? cubeedges::compose(crotated[rd].ce, ce) :
                                    cubeedges::compose(ce, crotated[rd].ce);
                                cubeedges cenewRepr = cubeedgesRepresentative(cenew, otransformNew);
                                if( corientReprCubesNewP != NULL && corientReprCubesNewP->containsCubeEdges(cenewRepr) )
                                    continue;
                                if( corientReprCubesNewC.containsCubeEdges(cenewRepr) )
                                    continue;
                                ceNewArr.push_back(cenewRepr);
                            }
                            if( !ceNewArr.empty() ) {
                                CornerPermReprCubes &ccpReprCubesNewN = cubesReprByDepth[depth].add(cornerPermReprIdxNew);
                                CornerOrientReprCubes &corientReprCubesNewN = ccpReprCubesNewN.cornerOrientCubesAdd(corientIdxNew);
                                for(cubeedges cenewRepr : ceNewArr) {
                                    if( corientReprCubesNewN.addCube(cenewRepr) )
                                        ++cubeCount;
                                }
                            }
                        }
                    }
                }
            }
            if( addCubesProgress->inc(reqFd, cubeCount) )
                break;
        }
    }
    addCubesProgress->threadFinished(reqFd);
}

static bool addCubes(
        CubesReprAtDepth *cubesReprByDepth,
        int depth, int threadCount, int reqFd)
{
    if( depth >= 8 )
        cubesReprByDepth[depth].initOccur();
    std::thread threads[threadCount];
    const CubesReprAtDepth &cubesArr = cubesReprByDepth[depth-1];
    AddCubesProgress addCubesProgress(
            std::distance(cubesArr.ccpCubesBegin(), cubesArr.ccpCubesEnd()),
            threadCount, depth);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesT, cubesReprByDepth,
                depth, t, threadCount, reqFd, &addCubesProgress);
    }
    addCubesT(cubesReprByDepth, depth, 0, threadCount, reqFd, &addCubesProgress);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    bool isCancelRequested = ProgressBase::isCancelRequested();
    if( isCancelRequested )
        sendRespMessage(reqFd, "canceled\n");
    else
        sendRespMessage(reqFd, "depth %d cubes=%lu\n", depth, cubesReprByDepth[depth].size());
    return isCancelRequested;
}

class SearchProgress : public ProgressBase {
    const CubesReprAtDepth::ccpcubes_iter m_ccpItBeg;
    const CubesReprAtDepth::ccpcubes_iter m_ccpItEnd;
    CubesReprAtDepth::ccpcubes_iter m_ccpItNext;
    const unsigned m_depth;
    unsigned m_runningThreadCount;
    bool m_isFinish;

public:
    SearchProgress(CubesReprAtDepth::ccpcubes_iter ccpItBeg,
            CubesReprAtDepth::ccpcubes_iter ccpItEnd, unsigned threadCount,
            unsigned depth)
        : m_ccpItBeg(ccpItBeg), m_ccpItEnd(ccpItEnd), m_ccpItNext(ccpItBeg),
          m_depth(depth), m_runningThreadCount(threadCount), m_isFinish(false)
    {
    }

    bool inc(int reqFd, CubesReprAtDepth::ccpcubes_iter *ccpItBuf);
    bool isFinish() const { return m_isFinish; }
    std::string progressStr();
};

bool SearchProgress::inc(int reqFd, CubesReprAtDepth::ccpcubes_iter *ccpItBuf)
{
    bool res;
    CubesReprAtDepth::ccpcubes_iter cornerPermIt;
    mutexLock();
    if( ccpItBuf == NULL )
        m_isFinish = true;
    res = !m_isFinish && m_ccpItNext != m_ccpItEnd && !isCancelRequestedNoLock();
    if( res )
        cornerPermIt = m_ccpItNext++;
    else
        --m_runningThreadCount;
    mutexUnlock();
    if( res ) {
        *ccpItBuf = cornerPermIt;
        if( m_depth >= 17 ) {
            unsigned itemCount = std::distance(m_ccpItBeg, m_ccpItEnd);
            unsigned itemIdx = std::distance(m_ccpItBeg, cornerPermIt);
            unsigned procCountNext = 100 * (itemIdx+1) / itemCount;
            unsigned procCountCur = 100 * itemIdx / itemCount;
            if( procCountNext != procCountCur && (m_depth>=18 || procCountCur % 10 == 0) )
                sendRespMessage(reqFd, "progress: depth %d search %d of %d, %d%%\n",
                        m_depth, itemIdx, itemCount, 100 * itemIdx / itemCount);
        }
    }else{
        if( m_depth >= 17 ) {
            sendRespMessage(reqFd, "progress: depth %d search %d threads still running\n",
                    m_depth, m_runningThreadCount);
        }
    }
    return res;
}

std::string SearchProgress::progressStr()
{
    unsigned itemCount = std::distance(m_ccpItBeg, m_ccpItEnd);
    unsigned itemIdx = std::distance(m_ccpItBeg, m_ccpItNext);
    std::ostringstream res;
    res << itemIdx << " of " << itemCount << ", " << 100 * itemIdx / itemCount << "%";
    return res.str();
}

static bool containsCube(const CubesReprAtDepth &cubesRepr, const cube &c)
{
    std::vector<EdgeReprCandidateTransform> otransform;

    unsigned ccpReprSearchIdx = cubecornerPermRepresentativeIdx(c.cc.getPerms());
    const CornerPermReprCubes &ccpReprSearchCubes = cubesRepr.getAt(ccpReprSearchIdx);
    cubecorner_orients ccoSearchRepr = cubecornerOrientsRepresentative(c.cc.getPerms(),
            c.cc.getOrients(), otransform);
    unsigned corientIdxSearch = cornersOrientToIdx(ccoSearchRepr);
    const CornerOrientReprCubes &ccoReprSearchCubes =
        ccpReprSearchCubes.cornerOrientCubesAt(corientIdxSearch);
    cubeedges ceSearchRepr = cubeedgesRepresentative(c.ce, otransform);
    return ccoReprSearchCubes.containsCubeEdges(ceSearchRepr);
}

static void searchMovesTa(const CubesReprAtDepth *cubesReprByDepth,
        unsigned depth, unsigned depthMax, const cube *csearch, int reqFd,
        SearchProgress *searchProgress)
{
    CubesReprAtDepth::ccpcubes_iter ccpIt;
    std::vector<EdgeReprCandidateTransform> otransform;
    cube csearchSymm = csearch->symmetric();

	while( searchProgress->inc(reqFd, &ccpIt) ) {
        const CornerPermReprCubes &ccpReprCubes = ccpIt->second;
        cubecorner_perms ccp = ccpIt->first;
        if( ccpReprCubes.empty() )
            continue;
        for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
            if( reversed )
                ccp = ccp.reverse();
            for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
                for(unsigned td = 0; td < TCOUNT; ++td) {
                    const cube cSearchT = cubeTransform(symmetric ? csearchSymm : *csearch, td);
                    cubecorner_perms ccpSearch = cubecorner_perms::compose(ccp, cSearchT.cc.getPerms());
                    unsigned ccpReprSearchIdx = cubecornerPermRepresentativeIdx(ccpSearch);
                    const CornerPermReprCubes &ccpReprSearchCubes = cubesReprByDepth[depthMax].getAt(ccpReprSearchIdx);

                    for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpReprCubes.ccoCubesBegin();
                            ccoCubesIt != ccpReprCubes.ccoCubesEnd(); ++ccoCubesIt)
                    {
                        const CornerOrientReprCubes &ccoReprCubes = ccoCubesIt->second;
                        cubecorner_orients cco = ccoCubesIt->first;
                        if( reversed )
                            cco = cco.reverse(ccpIt->first);
                        cubecorner_orients ccoSearch = cubecorners::orientsCompose(cco, cSearchT.cc);
                        cubecorner_orients ccoSearchRepr = cubecornerOrientsComposedRepresentative(
                                ccpSearch, ccoSearch, cSearchT.ce, otransform);
                        unsigned corientIdxSearch = cornersOrientToIdx(ccoSearchRepr);
                        const CornerOrientReprCubes &ccoReprSearchCubes =
                            ccpReprSearchCubes.cornerOrientCubesAt(corientIdxSearch);
                        if( ccoReprSearchCubes.empty() )
                            continue;
                        cubeedges ce;
                        if( otransform.size() == 1 )
                            ce = CornerOrientReprCubes::findSolutionEdge(
                                ccoReprCubes, ccoReprSearchCubes, otransform.front(), reversed);
                        else{
                            ce = CornerOrientReprCubes::findSolutionEdge(
                                ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
                        }
                        if( !ce.isNil() ) {
                            cubeedges ceSearch = cubeedges::compose(ce, cSearchT.ce);
                            cube cSearch = { .cc = cubecorners(ccpSearch, ccoSearch), .ce = ceSearch };
                            cube c = { .cc = cubecorners(ccp, cco), .ce = ce };
                            // cube found: cSearch = c ⊙  (csearch rev/symm/tr)
                            // rewriting equation:
                            //  cSearch ⊙  (cSearch rev) = csolved = c ⊙  (csearch rev/symm/tr) ⊙  (cSearch rev)
                            //  (c rev) = (c rev) ⊙  c ⊙  (csearch rev/symm/tr) ⊙  (cSearch rev)
                            //  (c rev) = (csearch rev/symm/tr) ⊙  (cSearch rev)
                            //  (c rev) ⊙  cSearch = (csearch rev/symm/tr) ⊙  (cSearch rev) ⊙  cSearch
                            //  (c rev) ⊙  cSearch = (csearch rev/symm/tr)
                            // moves solving (csearch rev/symm/tr): (cSearch rev) ⊙  c
                            std::string moves = "solution:";
                            cube cSearch1 = cubeTransform(cSearch, transformReverse(td));
                            if( symmetric )
                                cSearch1 = cSearch1.symmetric();
                            cube c1 = cubeTransform(c, transformReverse(td));
                            if( symmetric )
                                c1 = c1.symmetric();
                            moves += printMoves(cubesReprByDepth, cSearch1, true);
                            moves += printMoves(cubesReprByDepth, c1);
                            moves += "\n";
                            flockfile(stdout);
                            sendRespMessage(reqFd, moves.c_str());
                            funlockfile(stdout);
                            searchProgress->inc(reqFd, NULL);
                            return;
                        }
                    }
                }
            }
        }
    }
}

static bool searchMovesA(const CubesReprAtDepth *cubesReprByDepth,
		const cube &csearch, unsigned depth, unsigned depthMax, int threadCount, int fdReq)
{
    SearchProgress searchProgress(cubesReprByDepth[depth].ccpCubesBegin(),
            cubesReprByDepth[depth].ccpCubesEnd(), threadCount, depth+depthMax);
    std::thread threads[threadCount];
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTa, cubesReprByDepth,
                depth, depthMax, &csearch, fdReq, &searchProgress);
    }
    searchMovesTa(cubesReprByDepth, depth, depthMax, &csearch, fdReq, &searchProgress);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    if( searchProgress.isFinish() ) {
        sendRespMessage(fdReq, "finished at %s\n", searchProgress.progressStr().c_str());
        sendRespMessage(fdReq, "-- %d moves --\n", depth+depthMax);
        return true;
    }
    bool isCancelRequested = ProgressBase::isCancelRequested();
    if( isCancelRequested )
        sendRespMessage(fdReq, "canceled\n");
    else
        sendRespMessage(fdReq, "depth %d end\n", depth+depthMax);
    return isCancelRequested;
}

static void searchMovesTb(const CubesReprAtDepth *cubesReprByDepth,
		int depth, const cube *csearch, int reqFd, SearchProgress *searchProgress)
{
    CubesReprAtDepth::ccpcubes_iter cornerPerm2It;
    std::vector<EdgeReprCandidateTransform> o2transform;
    const CubesReprAtDepth &ccReprCubesC = cubesReprByDepth[depth];
	while( searchProgress->inc(reqFd, &cornerPerm2It) ) {
        const CornerPermReprCubes &ccpReprCubes2 = cornerPerm2It->second;
        cubecorner_perms ccp2 = cornerPerm2It->first;
        if( ccpReprCubes2.empty() )
            continue;
        for(CubesReprAtDepth::ccpcubes_iter ccpCubes1It = ccReprCubesC.ccpCubesBegin();
                ccpCubes1It != ccReprCubesC.ccpCubesEnd(); ++ccpCubes1It)
        {
            const CornerPermReprCubes &ccpCubes1 = ccpCubes1It->second;
            cubecorner_perms ccp1 = ccpCubes1It->first;
            if( ccpCubes1.empty() )
                continue;
            for(CornerPermReprCubes::ccocubes_iter ccoCubes1It = ccpCubes1.ccoCubesBegin();
                    ccoCubes1It != ccpCubes1.ccoCubesEnd(); ++ccoCubes1It)
            {
                const CornerOrientReprCubes &ccoCubes1 = ccoCubes1It->second;
                cubecorner_orients cco1 = ccoCubes1It->first;
                for(CornerOrientReprCubes::edges_iter edge1It = ccoCubes1.edgeBegin();
                        edge1It != ccoCubes1.edgeEnd(); ++edge1It)
                {
                    const cubeedges ce1 = *edge1It;
                    cube c1 = { .cc = cubecorners(ccp1, cco1), .ce = ce1 };
                    std::vector<cube> cubesChecked;
                    for(unsigned reversed1 = 0; reversed1 < (USEREVERSE ? 2 : 1); ++reversed1) {
                        cube c1r = reversed1 ? c1.reverse() : c1;
                        for(unsigned symmetric1 = 0; symmetric1 < 2; ++symmetric1) {
                            cube c1rs = symmetric1 ? c1r.symmetric() : c1r;
                            for(unsigned td1 = 0; td1 < TCOUNT; ++td1) {
                                const cube c1T = cubeTransform(c1rs, td1);
                                bool isDup = std::find(cubesChecked.begin(), cubesChecked.end(), c1T) != cubesChecked.end();
                                if( isDup )
                                    continue;
                                cubesChecked.push_back(c1T);
                                cube cSearch1 = cube::compose(c1T, *csearch);
                                for(unsigned reversed2 = 0; reversed2 < 2; ++reversed2) {
                                    cubecorner_perms ccp2r = reversed2 ? ccp2.reverse() : ccp2;
                                    for(unsigned symmetric2 = 0; symmetric2 < 2; ++symmetric2) {
                                        cube cSearch1s = symmetric2 ? cSearch1.symmetric() : cSearch1;
                                        for(unsigned td2 = 0; td2 < TCOUNT; ++td2) {
                                            cube cSearch1T = cubeTransform(cSearch1s, td2);
                                            cubecorner_perms ccpSearch2 = cubecorner_perms::compose(ccp2r, cSearch1T.cc.getPerms());
                                            unsigned ccpReprSearch2Idx = cubecornerPermRepresentativeIdx(ccpSearch2);
                                            const CornerPermReprCubes &ccpReprSearch2Cubes = cubesReprByDepth[DEPTH_MAX].getAt(ccpReprSearch2Idx);
                                            for(CornerPermReprCubes::ccocubes_iter ccoCubes2It = ccpReprCubes2.ccoCubesBegin();
                                                    ccoCubes2It != ccpReprCubes2.ccoCubesEnd(); ++ccoCubes2It)
                                            {
                                                const CornerOrientReprCubes &ccoReprCubes2 = ccoCubes2It->second;
                                                cubecorner_orients cco2 = ccoCubes2It->first;
                                                if( reversed2 )
                                                    cco2 = cco2.reverse(ccp2);
                                                cubecorner_orients ccoSearch2 = cubecorners::orientsCompose(cco2, cSearch1T.cc);
                                                cubecorner_orients ccoSearch2Repr = cubecornerOrientsComposedRepresentative(
                                                        ccpSearch2, ccoSearch2, cSearch1T.ce, o2transform);
                                                unsigned corientIdxSearch2 = cornersOrientToIdx(ccoSearch2Repr);
                                                const CornerOrientReprCubes &ccoReprSearch2Cubes = ccpReprSearch2Cubes.cornerOrientCubesAt(corientIdxSearch2);
                                                if( ccoReprSearch2Cubes.empty() )
                                                    continue;
                                                cubeedges ce2;
                                                if( o2transform.size() == 1 )
                                                    ce2 = CornerOrientReprCubes::findSolutionEdge(
                                                            ccoReprCubes2, ccoReprSearch2Cubes, o2transform.front(), reversed2);
                                                else
                                                    ce2 = CornerOrientReprCubes::findSolutionEdge(
                                                            ccoReprCubes2, ccoReprSearch2Cubes, o2transform, reversed2);
                                                if( !ce2.isNil() ) {
                                                    cubeedges cesearch2 = cubeedges::compose(ce2, cSearch1T.ce);
                                                    std::string moves = "solution:";
                                                    cube csearch2t = {
                                                        .cc = cubecorners(ccpSearch2, ccoSearch2),
                                                        .ce = cesearch2
                                                    };
                                                    cube csearch2 = cubeTransform(csearch2t, transformReverse(td2));
                                                    if( symmetric2 )
                                                        csearch2 = csearch2.symmetric();
                                                    moves += printMoves(cubesReprByDepth, csearch2, true);
                                                    cube c2t = { .cc = cubecorners(ccp2r, cco2), .ce = ce2 };
                                                    cube c2 = cubeTransform(c2t, transformReverse(td2));
                                                    if( symmetric2 )
                                                        c2 = c2.symmetric();
                                                    moves += printMoves(cubesReprByDepth, c2);
                                                    moves += printMoves(cubesReprByDepth, c1T);
                                                    moves += "\n";
                                                    flockfile(stdout);
                                                    sendRespMessage(reqFd, moves.c_str());
                                                    funlockfile(stdout);
                                                    searchProgress->inc(reqFd, NULL);
                                                    return;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
	}
}

static bool searchMovesB(const CubesReprAtDepth *cubesReprByDepth,
		const cube &csearch, unsigned depth, int threadCount, int fdReq)
{
    SearchProgress searchProgress(
            cubesReprByDepth[DEPTH_MAX].ccpCubesBegin(),
            cubesReprByDepth[DEPTH_MAX].ccpCubesEnd(), threadCount, 2*DEPTH_MAX+depth);
    std::thread threads[threadCount];
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTb, cubesReprByDepth, depth, &csearch,
                fdReq, &searchProgress);
    }
    searchMovesTb(cubesReprByDepth, depth, &csearch, fdReq, &searchProgress);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    if( searchProgress.isFinish() ) {
        sendRespMessage(fdReq, "finished at %s\n", searchProgress.progressStr().c_str());
        sendRespMessage(fdReq, "-- %d moves --\n", 2*DEPTH_MAX+depth);
        return true;
    }
    bool isCancelRequested = ProgressBase::isCancelRequested();
    if( isCancelRequested )
        sendRespMessage(fdReq, "canceled\n");
    else
        sendRespMessage(fdReq, "depth %d end\n", 2*DEPTH_MAX+depth);
    return isCancelRequested;
}

static void printStats(const CubesReprAtDepth *cubesReprByDepth, int depth)
{
    std::vector<EdgeReprCandidateTransform> otransform;

    std::cout << "depth " << depth << std::endl;
    const CubesReprAtDepth &ccReprCubesC = cubesReprByDepth[depth];
    for(CubesReprAtDepth::ccpcubes_iter ccpCubesIt = ccReprCubesC.ccpCubesBegin();
            ccpCubesIt != ccReprCubesC.ccpCubesEnd(); ++ccpCubesIt)
    {
        const CornerPermReprCubes &ccpCubes = ccpCubesIt->second;
        cubecorner_perms ccp = ccpCubesIt->first;
        cubecorner_perms ccpRepr = cubecornerPermsRepresentative(ccp);
        if( !(ccp == ccpRepr) ) {
            std::cout << "fatal: perm is not representative!" << std::endl;
        }
        for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpCubes.ccoCubesBegin();
                ccoCubesIt != ccpCubes.ccoCubesEnd(); ++ccoCubesIt)
        {
            const CornerOrientReprCubes &ccoCubes = ccoCubesIt->second;
            cubecorner_orients cco = ccoCubesIt->first;
            cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(ccp, cco, otransform);
            if( !(cco == ccoRepr) ) {
                std::cout << "fatal: orient is not representative!" << std::endl;
            }
            for(CornerOrientReprCubes::edges_iter edgeIt = ccoCubes.edgeBegin();
                    edgeIt != ccoCubes.edgeEnd(); ++edgeIt)
            {
                const cubeedges ce = *edgeIt;
                if( ce != cubeedgesRepresentative(ce, otransform) ) {
                    std::cout << "fatal: edge is not representative!" << std::endl;
                }
            }
        }
    }
}

static void searchMoves(const cube &csearch, int threadCount, int fdReq)
{
    static CubesReprAtDepth cubesReprByDepth[CSARR_SIZE];
    static unsigned depthAvailCount = 0;

    sendRespMessage(fdReq, "%s\n", getSetup().c_str());
    if( depthAvailCount == 0 ) {
        unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(csolved.cc.getPerms());
        CornerPermReprCubes &ccpCubes = cubesReprByDepth[0].add(cornerPermReprIdx);
        std::vector<EdgeReprCandidateTransform> otransform;
        cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(csolved.cc.getPerms(),
                csolved.cc.getOrients(), otransform);
        unsigned ccoIdx = cornersOrientToIdx(ccoRepr);
        CornerOrientReprCubes &ccoCubes = ccpCubes.cornerOrientCubesAdd(ccoIdx);
        cubeedges ceRepr = cubeedgesRepresentative(csolved.ce, otransform);
        ccoCubes.addCube(ceRepr);
        depthAvailCount = 1;
    }
    for(unsigned depth = 1; depth < depthAvailCount; ++depth) {
        if( searchMovesA(cubesReprByDepth, csearch, 0, depth, threadCount, fdReq) )
            return;
    }
    for(unsigned depth = 1; depth < depthAvailCount; ++depth) {
        if( searchMovesA(cubesReprByDepth, csearch, depth, depthAvailCount-1, threadCount, fdReq) )
            return;
    }
    for(unsigned depth = depthAvailCount; depth <= DEPTH_MAX; ++depth) {
        if( addCubes(cubesReprByDepth, depth, threadCount, fdReq) )
            return;
        //printStats(cubesReprByDepth, depth);
        depthAvailCount = depth+1;
        if( searchMovesA(cubesReprByDepth, csearch, depth-1, depth, threadCount, fdReq) )
            return;
        if( searchMovesA(cubesReprByDepth, csearch, depth, depth, threadCount, fdReq) )
            return;
    }
    for(unsigned depth = 1; depth <= DEPTH_MAX; ++depth) {
        if( searchMovesB(cubesReprByDepth, csearch, depth, threadCount, fdReq) )
            return;
    }
    sendRespMessage(fdReq, "not found\n");
}

static void processCubeReq(int fdReq, const char *squareColors)
{
    static std::mutex solverMutex;

    if( !strncmp(squareColors, "cancel", 6) ) {
        ProgressBase::requestCancel();
        char respHeader[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-length: 0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        write(fdReq, respHeader, strlen(respHeader));
        return;
    }
    if( solverMutex.try_lock() ) {
        ProgressBase::requestUncancel();
        char respHeader[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-type: text/plain\r\n"
            "Transfer-encoding: chunked\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        write(fdReq, respHeader, strlen(respHeader));
        cube c;
        if( cubeRead(fdReq, squareColors, c) ) {
            int threadCount = sysconf(_SC_NPROCESSORS_ONLN);
            sendRespMessage(fdReq, "thread count: %d\n", threadCount);
            if( threadCount <= 0 )
                threadCount = 4;
            if( c == csolved )
                sendRespMessage(fdReq, "already solved\n");
            else
                searchMoves(c, threadCount, fdReq);
        }
        write(fdReq, "0\r\n\r\n", 5);
        solverMutex.unlock();
        printf("%d solver end\n", fdReq);
    }else{
        char respHeader[] =
                "HTTP/1.1 200 OK\r\n"
                "Content-type: text/plain\r\n"
                "Content-length: 26\r\n"
                "Connection: keep-alive\r\n"
                "\r\n"
                "setup: the solver is busy\n";
        write(fdReq, respHeader, strlen(respHeader));
        printf("%d solver busy\n", fdReq);
    }
}

static void getFile(int fdReq, const char *fnameBeg) {
    const char *fnameEnd = strchr(fnameBeg, ' ');
    if( fnameEnd != NULL ) {
        std::string fname;
        fname.assign(fnameBeg, fnameEnd - fnameBeg);
        if( fname.empty())
            fname = "cube.html";
        FILE *fp = fopen(fname.c_str(), "r");
        if( fp != NULL ) {
            fseek(fp, 0, SEEK_END);
            long fsize = ftell(fp);
            const char *fnameExt = strrchr(fname.c_str(), '.');
            if( fnameExt == NULL )
                fnameExt = "txt";
            else
                ++fnameExt;
            const char *contentType = "application/octet-stream";
            if( !strcasecmp(fnameExt, "html") )
                contentType = "text/html; charset=utf-8";
            else if( !strcasecmp(fnameExt, "css") )
                contentType = "text/css";
            else if( !strcasecmp(fnameExt, "js") )
                contentType = "text/javascript";
            else if( !strcasecmp(fnameExt, "txt") )
                contentType = "text/plain";
            char respHeader[4096];
            sprintf(respHeader,
                "HTTP/1.1 200 OK\r\n"
                "Content-type: %s\r\n"
                "Content-length: %ld\r\n"
                "Connection: keep-alive\r\n"
                "\r\n", contentType, fsize);
            write(fdReq, respHeader, strlen(respHeader));
            rewind(fp);
            char fbuf[32768];
            long toWr = fsize;
            while( toWr > 0 ) {
                int toRd = (unsigned)toWr > sizeof(fbuf) ? sizeof(fbuf) : toWr;
                int rd = fread(fbuf, 1, toRd, fp);
                if( rd == 0 ) {
                    memset(fbuf, 0, toRd);
                    rd = toRd;
                }
                int wrTot = 0;
                while( wrTot < rd ) {
                    int wr = write(fdReq, fbuf + wrTot, rd - wrTot);
                    if( wr < 0 ) {
                        perror("socket write");
                        return;
                    }
                    wrTot += wr;
                }
                toWr -= rd;
            }
            fclose(fp);
            return;
        }
    }
    char respHeader[] =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-length: 0\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    write(fdReq, respHeader, strlen(respHeader));
}

static void processRequest(int fdReq, const char *reqHeader) {
    if( strncasecmp(reqHeader, "get ", 4) ) {
        char respHeader[] =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-length: 0\r\n"
            "Allow: GET\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        write(fdReq, respHeader, strlen(respHeader));
        return;
    }
    if( reqHeader[4] == '/' ) {
        if( reqHeader[5] == '?' )
            processCubeReq(fdReq, reqHeader+6);
        else
            getFile(fdReq, reqHeader+5);
    }else{
        char respHeader[] =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-length: 0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        write(fdReq, respHeader, strlen(respHeader));
    }
}

static void processConnection(int fdConn) {
    char reqHeaderBuf[16385], *reqHeaderEnd;
    int rdTot = 0;
    while( true ) {
        int rd = read(fdConn, reqHeaderBuf + rdTot, sizeof(reqHeaderBuf) - rdTot - 1);
        if( rd < 0 ) {
            perror("read");
            break;
        }
        if( rd == 0 ) {
            printf("%d closed\n", fdConn);
            break;
        }
        //write(STDOUT_FILENO, reqHeaderBuf+rdTot, rd);
        rdTot += rd;
        reqHeaderBuf[rdTot] = '\0';
        if( (reqHeaderEnd = strstr(reqHeaderBuf, "\r\n\r\n")) != NULL ) {
            printf("%d %.*s\n", fdConn,
                    (unsigned)(strchr(reqHeaderBuf, '\n') - reqHeaderBuf), reqHeaderBuf);
            reqHeaderEnd[2] = '\0';
            processRequest(fdConn, reqHeaderBuf);
            unsigned reqHeaderSize = reqHeaderEnd - reqHeaderBuf + 4;
            memmove(reqHeaderBuf, reqHeaderEnd+4, rdTot - reqHeaderSize);
            rdTot -= reqHeaderSize;
        }
        if( rdTot == sizeof(reqHeaderBuf)-1 ) {
            fprintf(stderr, "%d request header too long\n", fdConn);
            break;
        }
    }
    close(fdConn);
}

int main(int argc, char *argv[]) {
    int listenfd, acceptfd, opton = 1;
    struct sockaddr_in addr;
    std::set<cube> added;

    if( argc >= 2 ) {
        const char *s = argv[1];
        USEREVERSE = s[strlen(s)-1] == 'r';
        DEPTH_MAX = atoi(s);
        if( DEPTH_MAX <= 0 || DEPTH_MAX >= CSARR_SIZE ) {
            printf("DEPTH_MAX invalid: must be in range 1..%d\n", CSARR_SIZE-1);
            return 1;
        }
    }
    std::cout << getSetup() << std::endl;
    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket");
        return 1;
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opton, sizeof(opton));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    if( bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0 ) {
        perror("bind");
        return 1;
    }
    if( listen(listenfd, 1) < 0 ) {
        perror("listen");
        return 1;
    }
    permReprInit();
    while( (acceptfd = accept(listenfd, NULL, NULL)) >= 0 ) {
        //setsockopt(acceptfd, IPPROTO_TCP, TCP_NODELAY, &opton, sizeof(opton));
        std::thread t = std::thread(processConnection, acceptfd);
        t.detach();
    }
    perror("accept");
	return 1;
}

