#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <map>
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

static void sendRespMessage(int fd, const char *fmt, ...)
{
    va_list args;
    char message[4096], respBuf[5000];

    va_start(args, fmt);
    vsprintf(message, fmt, args);
    va_end(args);
    sprintf(respBuf, "%x\r\n%s\r\n", strlen(message), message);
    write(fd, respBuf, strlen(respBuf));
}

static std::string fmt_time()
{
	return "";
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
	bool operator==(const cubecorner_perms &ccp) const { return perms == ccp.perms; }
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
    static cubeedges transform(cubeedges, int idx);
	unsigned edgeN(unsigned idx) const { return edges >> 5 * idx & 0xf; }
	unsigned edgeR(unsigned idx) const { return edges >> (5*idx+4) & 1; }
	bool operator==(const cubeedges &ce) const { return edges == ce.edges; }
	bool operator!=(const cubeedges &ce) const { return edges != ce.edges; }
	bool operator<(const cubeedges &ce) const { return edges < ce.edges; }
    unsigned short getOrientIdx() const;
    unsigned long get() const { return edges; }
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
    unsigned long tmp1, tmp2;

    // needs: bmi2 (pext, pdep), sse3 (pshufb), sse4.1 (pinsrq, pextrq)
    asm(// store ce2 perm in xmm2
        "pext %[permMask], %[ce2], %[tmp1]\n"
        "pdep %[depPerm], %[tmp1], %[tmp2]\n"
        "pinsrq $0, %[tmp2], %%xmm2\n"
        "shr $32, %[tmp1]\n"
        "pdep %[depPerm], %[tmp1], %[tmp2]\n"
        "pinsrq $1, %[tmp2], %%xmm2\n"
        // store ce1 in xmm1
        "pdep %[depItem], %[ce1], %[tmp2]\n"
        "pinsrq $0, %[tmp2], %%xmm1\n"
        "shld $24, %[ce1], %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp2]\n"
        "pinsrq $1, %[tmp2], %%xmm1\n"
        // permute; result in xmm1
        "pshufb %%xmm2, %%xmm1\n"
        // store xmm1 in res
        "pextrq $0, %%xmm1, %[res]\n"
        "pext %[depItem], %[res], %[res]\n"
        "pextrq $1, %%xmm1, %[tmp1]\n"
        "pext %[depItem], %[tmp1], %[tmp1]\n"
        "shl $40, %[tmp1]\n"
        "or %[tmp1], %[res]\n"
            : [res]      "=&r"  (res.edges),
              [tmp1]     "=&r" (tmp1),
              [tmp2]     "=&r" (tmp2)
            : [permMask] "rm"  (0x7bdef7bdef7bdeful),
              [ce2]      "r"   (ce2.edges),
              [depPerm]  "rm"  (0xf0f0f0f0f0f0f0ful),
              [depItem]  "rm"  (0x1f1f1f1f1f1f1f1ful),
              [ce1]      "r"   (ce1.edges)
            : "xmm1", "xmm2", "cc"
       );
    res.edges &= (1ul<<60)-1;
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
    unsigned long edge2orients = ce2.edges & 0x842108421084210ul;
    res.edges ^= edge2orients;
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
    res = 0;
	for(int i = 10; i >= 0; --i)
        res = 2*res | (edges >> 5*i+4 & 1);
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
	struct cubecorners cc;
	struct cubeedges ce;

	bool operator==(const cube &c) const;
};

bool cube::operator==(const cube &c) const
{ 
	return cc == c.cc && ce == c.ce;
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
// |         | c  e  c |         | c     c |
// | e     e |         | e     e |         |
// |         | c  e  c |         | c     c |
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
	},{
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
        case 0: return 1;
        case 1: return 0;
        case 2: return 3;
        case 3: return 2;
        case 4: return 5;
        case 5: return 4;
        case 6: return 7;
        case 7: return 6;
        case 8: return 10;
        case 10: return 8;
        case 11: return 13;
        case 13: return 11;
        case 14: return 16;
        case 16: return 14;
    }
    return idx;
}

static cubecorner_perms cubecornerPermsCompose(const cubecorner_perms &ccp1,
		const cubecorner_perms &ccp2)
{
	cubecorner_perms res;
	for(int i = 0; i < 8; ++i)
		res.setAt(i, ccp1.getAt(ccp2.getAt(i)));
	return res;
}

static cubecorner_orients cubecornerOrientsCompose1(cubecorner_orients cco1,
		const cubecorners &cc2)
{
	static const unsigned char MOD3[] = { 0, 1, 2, 0, 1 };
	cubecorner_orients res;
	for(int i = 0; i < 8; ++i)
		res.setAt(i, MOD3[cco1.getAt(cc2.getPermAt(i)) + cc2.getOrientAt(i)]);
	return res;
}

static cubecorner_orients cubecornerOrientsCompose(cubecorner_orients cco1,
		const cubecorner_perms ccp2, cubecorner_orients cco2)
{
	unsigned short resOrients = 0;
#ifdef USE_ASM
    unsigned long tmp1;
    asm(
        // store ccp2 in xmm2
        "pdep %[depPerm], %[ccp2], %[tmp1]\n"
        "movq %[tmp1], %%xmm2\n"
        // store cco1 in xmm1
        "pdep %[depOrient], %[cco1], %[tmp1]\n"
        "movq %[tmp1], %%xmm1\n"
        // permute the cco1; result in xmm1
        "pshufb %%xmm2, %%xmm1\n"
        // store cco2 in xmm2
        "pdep %[depOrient], %[cco2], %[tmp1]\n"
        "movq %[tmp1], %%xmm2\n"
        // add the orients
        "paddb %%xmm2, %%xmm1\n"
        // calculate modulo 3
        "movd %[mod3], %%xmm2\n"
        "pshufb %%xmm1, %%xmm2\n"
        // store result
        "movq %%xmm2, %[tmp1]\n"
        "pext %[depOrient], %[tmp1], %[tmp1]\n"
        "mov %w[tmp1], %[resOrients]\n"
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

static cubecorner_orients cubecornerOrientsCompose(cubecorner_orients cco1,
		const cubecorners &cc2)
{
    return cubecornerOrientsCompose(cco1, cc2.getPerms(), cc2.getOrients());
}

static cubecorners cubecornersCompose(const cubecorners &cc1, const cubecorners &cc2)
{
	static const unsigned char MOD3[] = { 0, 1, 2, 0, 1 };
	cubecorners res;
	for(int i = 0; i < 8; ++i) {
		unsigned corner2Perm = cc2.getPermAt(i);
		res.setPermAt(i, cc1.getPermAt(corner2Perm));
		res.setOrientAt(i, MOD3[cc1.getOrientAt(corner2Perm) + cc2.getOrientAt(i)]);
	}
	return res;
}

static cube cubesCompose(const cube &c1, const cube &c2)
{
	cube res;
	res.cc = cubecornersCompose(c1.cc, c2.cc);
	res.ce = cubeedges::compose(c1.ce, c2.ce);
	return res;
}

static cube cubeTransform(const cube &c, int idx)
{
    const cube &ctrans = ctransformed[idx];
    cubecorners cc1 = cubecornersCompose(ctrans.cc, c.cc);
    cubeedges ce1 = cubeedges::compose(ctrans.ce, c.ce);
    const cube &ctransRev = ctransformed[transformReverse(idx)];
    cubecorners cc2 = cubecornersCompose(cc1, ctransRev.cc);
    cubeedges ce2 = cubeedges::compose(ce1, ctransRev.ce);
	return { .cc = cc2, .ce = ce2 };
}

static cubecorner_perms cubecornerPermsTransform1(cubecorner_perms ccp, int idx)
{
    cubecorner_perms ccp1 = cubecornerPermsCompose(ctransformed[idx].cc.getPerms(), ccp);
	return cubecornerPermsCompose(ccp1,
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
    cubecorner_orients cco1 = cubecornerOrientsCompose(ctransformed[idx].cc.getOrients(), ccp, cco);
	return cubecornerOrientsCompose(cco1, ctransformed[transformReverse(idx)].cc);
}

cubeedges cubeedges::transform(cubeedges ce, int idx)
{
    cubeedges cetrans = ctransformed[idx].ce;
	cubeedges res;
#ifdef USE_ASM
    cubeedges cetransRev = ctransformed[transformReverse(idx)].ce;
    unsigned long tmp1, tmp2;
    asm(
        // store ce perms in xmm2
        "pext %[permMask], %[ce], %[tmp1]\n"
        "pdep %[depPerm], %[tmp1], %[tmp2]\n"
        "pinsrq $0, %[tmp2], %%xmm2\n"
        "shr $32, %[tmp1]\n"
        "pdep %[depPerm], %[tmp1], %[tmp2]\n"
        "pinsrq $1, %[tmp2], %%xmm2\n"
        // store cetrans perms in xmm1
        "pext %[permMask], %[cetrans], %[tmp1]\n"
        "pdep %[depPerm], %[tmp1], %[tmp2]\n"
        "pinsrq $0, %[tmp2], %%xmm1\n"
        "shr $32, %[tmp1]\n"
        "pdep %[depPerm], %[tmp1], %[tmp2]\n"
        "pinsrq $1, %[tmp2], %%xmm1\n"
        // permute cetrans to ce locations; result in xmm1
        "pshufb %%xmm2, %%xmm1\n"
        // store ce orients in xmm2
        "mov %[ce], %[tmp1]\n"
        "and %[orientMask], %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp2]\n"
        "pinsrq $0, %[tmp2], %%xmm2\n"
        "shr $40, %[tmp1]\n"
        "pdep %[depItem], %[tmp1], %[tmp2]\n"
        "pinsrq $1, %[tmp2], %%xmm2\n"
        // or xmm1 with ce orients
        "por %%xmm2, %%xmm1\n"
        // store cetransRev perms in xmm2
        "pext %[permMask], %[cetransRev], %[tmp1]\n"
        "pdep %[depPerm], %[tmp1], %[tmp2]\n"
        "pinsrq $0, %[tmp2], %%xmm2\n"
        "shr $32, %[tmp1]\n"
        "pdep %[depPerm], %[tmp1], %[tmp2]\n"
        "pinsrq $1, %[tmp2], %%xmm2\n"
        // permute; result in xmm1
        "pshufb %%xmm2, %%xmm1\n"
        // store xmm1 in res
        "pextrq $0, %%xmm1, %[res]\n"
        "pext %[depItem], %[res], %[res]\n"
        "pextrq $1, %%xmm1, %[tmp1]\n"
        "pext %[depItem], %[tmp1], %[tmp1]\n"
        "shl $40, %[tmp1]\n"
        "or %[tmp1], %[res]\n"
        : [res]         "=&r"  (res.edges),
          [tmp1]        "=&r" (tmp1),
          [tmp2]        "=&r" (tmp2)
        :
          [ce]          "r"   (ce.edges),
          [permMask]    "rm"  (0x7bdef7bdef7bdeful),
          [orientMask]  "rm"  (0x842108421084210ul),
          [depPerm]     "rm"  (0xf0f0f0f0f0f0f0ful),
          [depItem]     "rm"  (0x1f1f1f1f1f1f1f1ful),
          [cetrans]     "rm"  (cetrans.edges),
          [cetransRev]  "r"   (cetransRev.edges)
        : "xmm1", "xmm2", "cc"
       );
    res.edges &= (1ul<<60)-1;
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

struct cubecorner_repr {
    unsigned reprIdx;
    unsigned transform;
};

static std::vector<cubecorner_perms> gPermRepr;
static std::map<cubecorner_perms, cubecorner_repr> gPermReprIdxs;

static void permReprInit()
{
    for(int i = 0; i < 40320; ++i) {
        cubecorner_perms perm = cornersIdxToPerm(i);
        cubecorner_perms permRepr = perm;
        unsigned transform = 1;
        for(int i = 0; i < TCOUNT; ++i) {
            cubecorner_perms cctr = ctransformed[i].cc.getPerms();
            cubecorner_perms ccrtr = ctransformed[transformReverse(i)].cc.getPerms();
            cubecorner_perms cand;
            for(int cno = 0; cno < 8; ++cno) {
                cand.setAt(cno, cctr.getAt(perm.getAt(ccrtr.getAt(cno))));
            }
            if( cand < permRepr ) {
                permRepr = cand;
                transform = 2 << i;
            }else if( cand == permRepr ) {
                transform |= 2 << i;
            }
        }
        std::map<cubecorner_perms, cubecorner_repr>::const_iterator it = gPermReprIdxs.find(permRepr);
        unsigned permReprIdx;
        if( it == gPermReprIdxs.end() ) {
            permReprIdx = gPermRepr.size();
            gPermRepr.push_back(permRepr);
            cubecorner_repr repr = { .reprIdx = permReprIdx, .transform = 0 };
            gPermReprIdxs[permRepr] = repr;
        }else
            permReprIdx = it->second.reprIdx;
        cubecorner_repr repr = { .reprIdx = permReprIdx, .transform = transform };
        gPermReprIdxs[perm] = repr;
    }
}

static cubecorner_perms cubecornerPermsRepresentative(cubecorner_perms ccp)
{
    unsigned permReprIdx = gPermReprIdxs.at(ccp).reprIdx;
    return gPermRepr[permReprIdx];
}

static cubecorner_orients cubecornerOrientsRepresentative(cubecorner_perms ccp, cubecorner_orients cco,
        std::vector<int> &transform)
{
    cubecorner_repr repr = gPermReprIdxs.at(ccp);
    cubecorner_orients orepr = cco;
    bool isInit = repr.transform & 1;
	transform.clear();
	transform.push_back(-1);
    for(int i = 0; i < TCOUNT; ++i) {
        if( repr.transform & 2 << i ) {
            cubecorner_orients ocand = cubecornerOrientsTransform(ccp, cco, i);
            if( !isInit || ocand < orepr ) {
                orepr = ocand;
				transform.clear();
				transform.push_back(i);
                isInit = true;
            }else if( isInit && ocand == orepr )
				transform.push_back(i);
        }
    }
    return orepr;
}

static unsigned cubecornerPermRepresentativeIdx(cubecorner_perms ccp)
{
    return gPermReprIdxs.at(ccp).reprIdx;
}

static cubeedges cubeedgesRepresentative(cubeedges ce, const std::vector<int> &transform)
{
    cubeedges cerepr = transform[0] == -1 ? ce : cubeedges::transform(ce, transform[0]);
    for(int i = 1; i < transform.size(); ++i) {
		cubeedges cand = cubeedges::transform(ce, transform[i]);
		if( cand < cerepr )
			cerepr = cand;
    }
    return cerepr;
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
	char line[100];
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
		unsigned m = (1 << p) - 1;
		unused = unused & m | unused >> 4 & ~m;
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

enum { CSARR_SIZE = 9, CSREPR_SIZE = 3 };

const struct {
    unsigned csarr, csrepr;
} depthMaxByMem[] = {
    { .csarr = 6, .csrepr = 2 },    // ~1.2GB
    { .csarr = 7, .csrepr = 1 },    // ~2.4GB
    { .csarr = 6, .csrepr = 3 },    // ~10.0GB
    { .csarr = 7, .csrepr = 2 },    // ~11.2GB
    { .csarr = 8, .csrepr = 1 },    // ~25.8GB
};

static int depthMaxSelFn() {
    long pageSize = sysconf(_SC_PAGESIZE);
    long pageCount = sysconf(_SC_PHYS_PAGES);
    long memSizeGB = pageSize * pageCount / 1073741824;
    int model = memSizeGB >= 27 ? 4 : memSizeGB >= 12 ? 3 : memSizeGB >= 11 ? 2 : memSizeGB >= 3 ? 1 : 0;
    printf("page size: %ld, count: %ld, mem GB: %ld, model: %d\n",
            pageSize, pageCount, memSizeGB, model);
    return model;
}

auto [DEPTH_MAX, DREPR_COUNT] = depthMaxByMem[depthMaxSelFn()];

static std::string getSetup() {
    std::ostringstream res;
    res << "setup: depth " << DEPTH_MAX << " repr " << DREPR_COUNT;
#ifdef USE_ASM
    res << " asm";
#endif
#ifdef ASMCHECK
    res << " with check";
#endif
    return res.str();
}

class CornerOrientCubes {
    cubecorner_orients m_orients;
    std::vector<cubeedges> m_items;

public:
    typedef std::vector<cubeedges>::const_iterator edges_iter;
    CornerOrientCubes(cubecorner_orients orients)
        : m_orients(orients)
    {
    }

    void swap(CornerOrientCubes &cs) {
        std::swap(m_orients, cs.m_orients);
        m_items.swap(cs.m_items);
    }
    cubecorner_orients cornerOrients() const { return m_orients; }
    bool addCube(cubeedges ce);
    unsigned addCubes(std::vector<cubeedges>&);
    edges_iter edgeBegin() const { return m_items.begin(); }
    edges_iter edgeEnd() const { return m_items.end(); }
    bool containsCubeEdges(cubeedges) const;
};

bool CornerOrientCubes::addCube(cubeedges ce)
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
	return true;
}

unsigned CornerOrientCubes::addCubes(std::vector<cubeedges> &cearr)
{
    unsigned res = 0;
    for(unsigned i = 0; i < cearr.size(); ++i) {
        if( addCube(cearr[i]) )
            ++res;
    }
    return res;
}

bool CornerOrientCubes::containsCubeEdges(cubeedges ce) const
{
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

class CornerPermCubes {
public:
    typedef std::vector<CornerOrientCubes>::const_iterator ccorients_iter;
	CornerPermCubes();
	~CornerPermCubes();
	const CornerOrientCubes *findCornerOrientCubes(cubecorner_orients) const;
	CornerOrientCubes &cornerOrientCubesAddIfAbsent(cubecorner_orients);
    bool empty() const { return m_coCubes.empty(); }
    ccorients_iter ccorientsBegin() const { return m_coCubes.begin(); }
    ccorients_iter ccorientsEnd() const { return m_coCubes.end(); }

private:
	std::vector<CornerOrientCubes> m_coCubes;
};

CornerPermCubes::CornerPermCubes()
{
}

CornerPermCubes::~CornerPermCubes()
{
}

const CornerOrientCubes *CornerPermCubes::findCornerOrientCubes(cubecorner_orients cco) const
{
	unsigned lo = 0, hi = m_coCubes.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_coCubes[mi].cornerOrients() < cco )
			lo = mi + 1;
		else
			hi = mi;
	}
	return lo < m_coCubes.size() && cco == m_coCubes[lo].cornerOrients() ?
		&m_coCubes[lo] : NULL;
}

CornerOrientCubes &CornerPermCubes::cornerOrientCubesAddIfAbsent(cubecorner_orients cco)
{
	unsigned lo = 0, hi = m_coCubes.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_coCubes[mi].cornerOrients() < cco )
			lo = mi + 1;
		else
			hi = mi;
	}
	if( lo == m_coCubes.size() || cco < m_coCubes[lo].cornerOrients() ) {
		m_coCubes.push_back(CornerOrientCubes(cco));
		hi = m_coCubes.size();
		while( --hi > lo )
			m_coCubes[hi].swap(m_coCubes[hi-1]);
	}
	return m_coCubes[lo];
}

class CornerOrientReprCubes {
	std::vector<cubeedges> m_items;
    std::vector<unsigned long> m_orientOccur;
public:
    typedef std::vector<cubeedges>::const_iterator edges_iter;
    void initOccur() { m_orientOccur.resize(32); }
	bool addCube(cubeedges);
	bool containsCubeEdges(cubeedges) const;
    bool empty() const { return m_items.empty(); }
    edges_iter edgeBegin() const { return m_items.begin(); }
    edges_iter edgeEnd() const { return m_items.end(); }
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
        m_orientOccur[orientIdx >> 6] |= 1ul << (orientIdx & 0x3f);
    }
	return true;
}

bool CornerOrientReprCubes::containsCubeEdges(cubeedges ce) const
{
    unsigned short orientIdx = ce.getOrientIdx();
    if( ! m_orientOccur.empty() ) {
        unsigned short orientIdx = ce.getOrientIdx();
        if( (m_orientOccur[orientIdx >> 6] & 1ul << (orientIdx & 0x3f)) == 0 )
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

class CornerPermReprCubes {
	CornerOrientReprCubes m_coreprCubes[2187];

public:
	CornerPermReprCubes();
	~CornerPermReprCubes();
    void initOccur();
	const CornerOrientReprCubes &cornerOrientCubesAt(unsigned cornerOrientIdx) const {
        return m_coreprCubes[cornerOrientIdx];
    }
	CornerOrientReprCubes &cornerOrientCubesAt(unsigned cornerOrientIdx) {
        return m_coreprCubes[cornerOrientIdx];
    }
};

CornerPermReprCubes::CornerPermReprCubes()
{
}

CornerPermReprCubes::~CornerPermReprCubes()
{
}

void CornerPermReprCubes::initOccur() {
    for(int i = 0; i < 2187; ++i)
        m_coreprCubes[i].initOccur();
}

class CubesAtDepth {
    std::vector<CornerPermCubes> m_cornerPermCubes;
public:
    CubesAtDepth();
    CubesAtDepth(const CubesAtDepth&) = delete;
    ~CubesAtDepth();
    bool empty() const { return m_cornerPermCubes.empty(); }
    void init() { m_cornerPermCubes.resize(40320); }
    CornerPermCubes &operator[](unsigned idx) { return m_cornerPermCubes[idx]; }
    const CornerPermCubes &operator[](unsigned idx) const { return m_cornerPermCubes[idx]; }
};

CubesAtDepth::CubesAtDepth() {
}

CubesAtDepth::~CubesAtDepth() {
}

class CubesReprAtDepth {
    std::vector<CornerPermReprCubes> m_cornerPermReprCubes;
public:
    CubesReprAtDepth();
    CubesReprAtDepth(const CubesReprAtDepth&) = delete;
    ~CubesReprAtDepth();
    bool empty() const { return m_cornerPermReprCubes.empty(); }
    void init() { m_cornerPermReprCubes.resize(1844); }
    void initOccur();
    CornerPermReprCubes &operator[](unsigned idx) { return m_cornerPermReprCubes[idx]; }
    const CornerPermReprCubes &operator[](unsigned idx) const { return m_cornerPermReprCubes[idx]; }
};

CubesReprAtDepth::CubesReprAtDepth() {
}

CubesReprAtDepth::~CubesReprAtDepth() {
}

void CubesReprAtDepth::initOccur()
{
    for(auto it = m_cornerPermReprCubes.begin(); it != m_cornerPermReprCubes.end(); ++it)
        it->initOccur();
}

static std::string printMoves(const CubesAtDepth *cubesByDepth, const cube &c)
{
	std::vector<int> rotateDirs;
	unsigned short cornersPerm = cornersPermToIdx(c.cc.getPerms());
	int depth = 0;
	while( true ) {
		const CornerOrientCubes *corientCubes =
            cubesByDepth[depth][cornersPerm].findCornerOrientCubes(c.cc.getOrients());
		if( corientCubes != NULL && corientCubes->containsCubeEdges(c.ce) )
			break;
		++depth;
	}
	cube cc = c;
	while( depth-- > 0 ) {
		int cm = 0;
		cube cc1;
		while( true ) {
			cc1 = cubesCompose(cc, crotated[cm]);
			cornersPerm = cornersPermToIdx(cc1.cc.getPerms());
            const CornerOrientCubes *corientCubes =
                cubesByDepth[depth][cornersPerm].findCornerOrientCubes(cc1.cc.getOrients());
			if( corientCubes != NULL && corientCubes->containsCubeEdges(cc1.ce) )
				break;
			++cm;
		}
		rotateDirs.push_back(rotateDirReverse(cm));
		cc = cc1;
	}
    std::string res;
	for(int j = rotateDirs.size(); j > 0; --j) {
		res += " ";
        res += rotateDirName(rotateDirs[j-1]);
    }
    return res;
}

static std::string printMovesRev(const CubesAtDepth *cubesByDepth,
		const cube &c)
{
	unsigned short cornersPerm = cornersPermToIdx(c.cc.getPerms());
	int depth = 0;
	while( true ) {
        const CornerOrientCubes *corientCubes =
            cubesByDepth[depth][cornersPerm].findCornerOrientCubes(c.cc.getOrients());
		if( corientCubes != NULL && corientCubes->containsCubeEdges(c.ce) )
			break;
		++depth;
        if( depth == DEPTH_MAX+1)
            break;
	}
	cube cc = c;
    std::string res;
	while( depth-- > 0 ) {
		int cm = 0;
		cube cc1;
		while( true ) {
			cc1 = cubesCompose(cc, crotated[cm]);
			cornersPerm = cornersPermToIdx(cc1.cc.getPerms());
            const CornerOrientCubes *corientCubes =
                cubesByDepth[depth][cornersPerm].findCornerOrientCubes(cc1.cc.getOrients());
			if( corientCubes != NULL && corientCubes->containsCubeEdges(cc1.ce) )
				break;
			++cm;
		}
        res += " ";
        res += rotateDirName(cm);
		cc = cc1;
	}
    return res;
}

static std::string printMovesRevDeep(const CubesAtDepth *cubesByDepth,
		const cube &c)
{
    for(int depthMax = 1;; ++depthMax) {
        for(int depth = depthMax - 1; depth <= depthMax; ++depth) {
            for(int cornerPerm = 0; cornerPerm < 40320; ++cornerPerm) {
                const CornerPermCubes &ccpCubes = cubesByDepth[depth][cornerPerm];
                cubecorner_perms ccp = cornersIdxToPerm(cornerPerm);
                cubecorner_perms ccpNew = cubecornerPermsCompose(c.cc.getPerms(), ccp);
                unsigned cornerPermSearch = cornersPermToIdx(ccpNew);
                const CornerPermCubes &cornerPermCubesSearch = cubesByDepth[depthMax][cornerPermSearch];
                for(CornerPermCubes::ccorients_iter ccorientIt = ccpCubes.ccorientsBegin();
                        ccorientIt != ccpCubes.ccorientsEnd(); ++ccorientIt)
                {
                    const CornerOrientCubes &ccoCubes = *ccorientIt;
                    cubecorner_orients cco = ccoCubes.cornerOrients();
                    cubecorner_orients ccoNew = cubecornerOrientsCompose(c.cc.getOrients(), ccp, cco);
                    const CornerOrientCubes *corientCubesSearch = cornerPermCubesSearch.findCornerOrientCubes(ccoNew);
                    if( corientCubesSearch != NULL ) {
                        for(CornerOrientCubes::edges_iter edgeIt = ccoCubes.edgeBegin();
                                edgeIt != ccoCubes.edgeEnd(); ++edgeIt)
                        {
                            const cubeedges ce = *edgeIt;
                            cubeedges ceNew = cubeedges::compose(c.ce, ce);
                            if( corientCubesSearch->containsCubeEdges(ceNew) ) {
                                cube c = { .cc = cubecorners(ccp, cco), .ce = ce };
                                std::string moves = printMoves(cubesByDepth, c);
                                cube cnew = { .cc = cubecorners(ccpNew, ccoNew), .ce = ceNew };
                                moves += printMovesRev(cubesByDepth, cnew);
                                return moves;
                            }
                        }
                    }
                }
            }
        }
    }
}

static void addCubesT(CubesAtDepth *cubesByDepth,
		int depth, int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
	unsigned cubeCountT = 0;
    const CubesAtDepth *cubesP = depth == 1 ? NULL : cubesByDepth + depth-2;
    const CubesAtDepth &cubesC = cubesByDepth[depth-1];
    CubesAtDepth &cubesN = cubesByDepth[depth];
    std::vector<cubeedges> arrCeNew;

	for(int cornerPerm = 0; cornerPerm < 40320; ++cornerPerm) {
		const CornerPermCubes &ccpCubes = cubesC[cornerPerm];
        if( ccpCubes.empty() )
            continue;
		cubecorner_perms ccp = cornersIdxToPerm(cornerPerm);
		for(int rd = 0; rd < RCOUNT; ++rd) {
            cubecorner_perms ccpNew = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
			unsigned short cornerPermNew = cornersPermToIdx(ccpNew);
			if( cornerPermNew % threadCount != threadNo )
                continue;
            const CornerPermCubes *ccpCubesNewP = cubesP == NULL ? NULL : &(*cubesP)[cornerPermNew];
            const CornerPermCubes &ccpCubesNewC = cubesC[cornerPermNew];
            CornerPermCubes &ccpCubesNewN = cubesN[cornerPermNew];
            for(CornerPermCubes::ccorients_iter ccorientIt = ccpCubes.ccorientsBegin();
                    ccorientIt != ccpCubes.ccorientsEnd(); ++ccorientIt)
            {
                const CornerOrientCubes &ccoCubes = *ccorientIt;
                cubecorner_orients cco = ccoCubes.cornerOrients();
                cubecorner_orients ccoNew = cubecornerOrientsCompose(cco, crotated[rd].cc);
                const CornerOrientCubes *ccoCubesNewP = ccpCubesNewP == NULL ? NULL :
                    ccpCubesNewP->findCornerOrientCubes(ccoNew);
                const CornerOrientCubes *ccoCubesNewC = ccpCubesNewC.findCornerOrientCubes(ccoNew);
                arrCeNew.clear();
                for(CornerOrientCubes::edges_iter edgeIt = ccoCubes.edgeBegin();
                        edgeIt != ccoCubes.edgeEnd(); ++edgeIt)
                {
                    cubeedges cenew = cubeedges::compose(*edgeIt, crotated[rd].ce);
                    if( (ccoCubesNewP == NULL || !ccoCubesNewP->containsCubeEdges(cenew) ) &&
                            (ccoCubesNewC == NULL || !ccoCubesNewC->containsCubeEdges(cenew) ) )
                        arrCeNew.push_back(cenew);
                }
                if( !arrCeNew.empty() ) {
                    CornerOrientCubes &ccoCubesNewN = ccpCubesNewN.cornerOrientCubesAddIfAbsent(ccoNew);
                    cubeCountT += ccoCubesNewN.addCubes(arrCeNew);
                }
            }
		}
	}
	cubeCount->fetch_add(cubeCountT);
}

static void addCubes(CubesAtDepth *cubesByDepth, int depth,
        int threadCount, int fdReq)
{
    cubesByDepth[depth].init();
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesT, cubesByDepth, depth, t, threadCount,
                &cubeCount);
    }
    addCubesT(cubesByDepth, depth, 0, threadCount, &cubeCount);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d cubes=%u\n", fmt_time().c_str(), depth,
            cubeCount.load());
}

static void addCubesTrepr(const CubesAtDepth *cubesByDepth,
        CubesReprAtDepth *cubesReprByDepth,
		int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
	unsigned cubeCountT = 0;
    std::vector<int> otransform, otransformNew;

	for(int cornerPerm = 0; cornerPerm < 40320; ++cornerPerm) {
		const CornerPermCubes &ccpCubes = cubesByDepth[DEPTH_MAX][cornerPerm];
        if( ccpCubes.empty() )
            continue;
		cubecorner_perms ccp = cornersIdxToPerm(cornerPerm);
        if( !(ccp == cubecornerPermsRepresentative(ccp)) )
            continue;
		for(int rd = 0; rd < RCOUNT; ++rd) {
            cubecorner_perms ccpNew = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
            unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpNew);
            if( cornerPermReprIdx % threadCount != threadNo )
                continue;
            CornerPermReprCubes &ccpReprCubes = cubesReprByDepth[0][cornerPermReprIdx];
            cubecorner_perms ccpReprNew = cubecornerPermsRepresentative(ccpNew);
            unsigned short cornerPermReprNew = cornersPermToIdx(ccpReprNew);
            const CornerPermCubes &ccpCubesNewP = cubesByDepth[DEPTH_MAX-1][cornerPermReprNew];
            const CornerPermCubes &ccpCubesNewC = cubesByDepth[DEPTH_MAX][cornerPermReprNew];
            for(CornerPermCubes::ccorients_iter ccorientIt = ccpCubes.ccorientsBegin();
                    ccorientIt != ccpCubes.ccorientsEnd(); ++ccorientIt)
            {
                const CornerOrientCubes &ccoCubes = *ccorientIt;
                cubecorner_orients cco = ccoCubes.cornerOrients();
                cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(ccp, cco, otransform);
                if( cco != ccoRepr )
                    continue;
                cubecorner_orients ccoNew = cubecornerOrientsCompose(cco, crotated[rd].cc);
                cubecorner_orients ccoReprNew = cubecornerOrientsRepresentative(ccpNew, ccoNew, otransformNew);
                const CornerOrientCubes *ccoCubesNewP = ccpCubesNewP.findCornerOrientCubes(ccoReprNew);
                const CornerOrientCubes *ccoCubesNewC = ccpCubesNewC.findCornerOrientCubes(ccoReprNew);
                unsigned corientIdxNew = cornersOrientToIdx(ccoReprNew);
                CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(corientIdxNew);
                for(CornerOrientCubes::edges_iter edgeIt = ccoCubes.edgeBegin();
                        edgeIt != ccoCubes.edgeEnd(); ++edgeIt)
                {
                    const cubeedges ce = *edgeIt;
                    if( ce != cubeedgesRepresentative(ce, otransform) )
                        continue;
                    cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
                    cubeedges cenewRepr = cubeedgesRepresentative(cenew, otransformNew);
                    if( ccoCubesNewP != NULL && ccoCubesNewP->containsCubeEdges(cenewRepr) )
                        continue;
                    if( ccoCubesNewC != NULL && ccoCubesNewC->containsCubeEdges(cenewRepr) )
                        continue;
                    if( ccoReprCubes.addCube(cenewRepr) )
                        ++cubeCountT;
                }
            }
        }
	}
	cubeCount->fetch_add(cubeCountT);
}

static void addCubesRepr(const CubesAtDepth *cubesByDepth,
        CubesReprAtDepth *cubesReprByDepth, int threadCount, int fdReq)
{
    cubesReprByDepth[0].init();
    if( DEPTH_MAX >= 8 )
        cubesReprByDepth[0].initOccur();
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesTrepr, cubesByDepth, cubesReprByDepth, t, threadCount,
                &cubeCount);
    }
    addCubesTrepr(cubesByDepth, cubesReprByDepth, 0, threadCount, &cubeCount);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d repr cubes=%u\n", fmt_time().c_str(), DEPTH_MAX+1,
            cubeCount.load());
}

static void addCubesTrepr2(const CubesAtDepth *cubesByDepth,
        CubesReprAtDepth *cubesReprByDepth,
		int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
    unsigned cubeCountT = 0;
    std::vector<int> otransform, otransformNew;

    for(int cornerPermReprIdx = 0; cornerPermReprIdx < 1844; ++cornerPermReprIdx) {
        const CornerPermReprCubes &cpermReprCubesC = cubesReprByDepth[0][cornerPermReprIdx];
        cubecorner_perms ccp = gPermRepr[cornerPermReprIdx];
        for(int rd = 0; rd < RCOUNT; ++rd) {
            cubecorner_perms ccpNew = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
            unsigned cornerPermReprIdxNew = cubecornerPermRepresentativeIdx(ccpNew);
            if( cornerPermReprIdxNew % threadCount != threadNo )
                continue;
            cubecorner_perms ccpReprNew = cubecornerPermsRepresentative(ccpNew);
            unsigned short cornerPermReprNew = cornersPermToIdx(ccpReprNew);
            const CornerPermCubes &cpermCubesNewP = cubesByDepth[DEPTH_MAX][cornerPermReprNew];
            const CornerPermReprCubes &ccpReprCubesNewC = cubesReprByDepth[0][cornerPermReprIdxNew];
            CornerPermReprCubes &ccpReprCubesNewN = cubesReprByDepth[1][cornerPermReprIdxNew];
            for(int corientIdx = 0; corientIdx < 2187; ++corientIdx) {
                const CornerOrientReprCubes &corientReprCubesC = cpermReprCubesC.cornerOrientCubesAt(corientIdx);
                if( corientReprCubesC.empty() )
                    continue;
                cubecorner_orients cco = cornersIdxToOrient(corientIdx);
                cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(ccp, cco, otransform);
                if( cco != ccoRepr )
                    continue;
                cubecorner_orients ccoNew = cubecornerOrientsCompose(cco, crotated[rd].cc);
                cubecorner_orients ccoReprNew = cubecornerOrientsRepresentative(ccpNew, ccoNew, otransformNew);
                const CornerOrientCubes *ccoCubesNewP = cpermCubesNewP.findCornerOrientCubes(ccoReprNew);
                unsigned corientIdxNew = cornersOrientToIdx(ccoReprNew);
                const CornerOrientReprCubes &ccoReprCubesNewC = ccpReprCubesNewC.cornerOrientCubesAt(corientIdxNew);
                CornerOrientReprCubes &ccoReprCubesNewN = ccpReprCubesNewN.cornerOrientCubesAt(corientIdxNew);
                for(CornerOrientReprCubes::edges_iter edgeIt = corientReprCubesC.edgeBegin();
                        edgeIt != corientReprCubesC.edgeEnd(); ++edgeIt)
                {
                    const cubeedges ce = *edgeIt;
                    if( ce != cubeedgesRepresentative(ce, otransform) )
                        continue;
                    cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
                    cubeedges cenewRepr = cubeedgesRepresentative(cenew, otransformNew);
                    if( ccoCubesNewP != NULL && ccoCubesNewP->containsCubeEdges(cenewRepr) )
                        continue;
                    if( ccoReprCubesNewC.containsCubeEdges(cenewRepr) )
                        continue;
                    if( ccoReprCubesNewN.addCube(cenewRepr) )
                        ++cubeCountT;
                }
            }
        }
    }
    cubeCount->fetch_add(cubeCountT);
}

static void addCubesRepr2(const CubesAtDepth *cubesByDepth,
        CubesReprAtDepth *cubesReprByDepth,
        int threadCount, int fdReq)
{
    cubesReprByDepth[1].init();
    if( DEPTH_MAX >= 7 )
        cubesReprByDepth[1].initOccur();
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesTrepr2, cubesByDepth, cubesReprByDepth,
                t, threadCount, &cubeCount);
    }
    addCubesTrepr2(cubesByDepth, cubesReprByDepth, 0, threadCount, &cubeCount);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d repr cubes=%u\n", fmt_time().c_str(), DEPTH_MAX+2,
            cubeCount.load());
}

static void addCubesTrepr3(
        CubesReprAtDepth *cubesReprByDepth,
		int depthRepr, int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
    unsigned cubeCountT = 0;
    std::vector<int> otransform, otransformNew;

    for(int cornerPermReprIdx = 0; cornerPermReprIdx < 1844; ++cornerPermReprIdx) {
        const CornerPermReprCubes &cpermReprCubesC = cubesReprByDepth[depthRepr-1][cornerPermReprIdx];
        cubecorner_perms ccp = gPermRepr[cornerPermReprIdx];
        for(int rd = 0; rd < RCOUNT; ++rd) {
            cubecorner_perms ccpNew = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
            unsigned cornerPermReprIdxNew = cubecornerPermRepresentativeIdx(ccpNew);
            if( cornerPermReprIdxNew % threadCount != threadNo )
                continue;
            const CornerPermReprCubes &ccpReprCubesNewP = cubesReprByDepth[depthRepr-2][cornerPermReprIdxNew];
            const CornerPermReprCubes &ccpReprCubesNewC = cubesReprByDepth[depthRepr-1][cornerPermReprIdxNew];
            CornerPermReprCubes &ccpReprCubesNewN = cubesReprByDepth[depthRepr][cornerPermReprIdxNew];
            for(int corientIdx = 0; corientIdx < 2187; ++corientIdx) {
                const CornerOrientReprCubes &corientReprCubesC = cpermReprCubesC.cornerOrientCubesAt(corientIdx);
                if( corientReprCubesC.empty() )
                    continue;
                cubecorner_orients cco = cornersIdxToOrient(corientIdx);
                cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(ccp, cco, otransform);
                if( !(cco == ccoRepr) )
                    continue;
                cubecorner_orients ccoNew = cubecornerOrientsCompose(cco, crotated[rd].cc);
                cubecorner_orients ccoReprNew = cubecornerOrientsRepresentative(ccpNew, ccoNew, otransformNew);
                unsigned corientIdxNew = cornersOrientToIdx(ccoReprNew);
                const CornerOrientReprCubes &corientReprCubesNewP = ccpReprCubesNewP.cornerOrientCubesAt(corientIdxNew);
                const CornerOrientReprCubes &corientReprCubesNewC = ccpReprCubesNewC.cornerOrientCubesAt(corientIdxNew);
                CornerOrientReprCubes &corientReprCubesNewN = ccpReprCubesNewN.cornerOrientCubesAt(corientIdxNew);
                for(CornerOrientReprCubes::edges_iter edgeIt = corientReprCubesC.edgeBegin();
                        edgeIt != corientReprCubesC.edgeEnd(); ++edgeIt)
                {
                    const cubeedges ce = *edgeIt;
                    if( ce != cubeedgesRepresentative(ce, otransform) )
                        continue;
                    cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
                    cubeedges cenewRepr = cubeedgesRepresentative(cenew, otransformNew);
                    if( corientReprCubesNewP.containsCubeEdges(cenewRepr) )
                        continue;
                    if( corientReprCubesNewC.containsCubeEdges(cenewRepr) )
                        continue;
                    if( corientReprCubesNewN.addCube(cenewRepr) )
                        ++cubeCountT;
                }
            }
        }
    }
    cubeCount->fetch_add(cubeCountT);
}

static void addCubesRepr3(
        CubesReprAtDepth *cubesReprByDepth,
        int depthRepr, int threadCount, int fdReq)
{
    cubesReprByDepth[depthRepr].init();
    if( DEPTH_MAX + depthRepr >= 8 )
        cubesReprByDepth[depthRepr].initOccur();
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesTrepr3, cubesReprByDepth,
                depthRepr, t, threadCount, &cubeCount);
    }
    addCubesTrepr3(cubesReprByDepth, depthRepr, 0, threadCount, &cubeCount);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d repr cubes=%u\n", fmt_time().c_str(), DEPTH_MAX+depthRepr+1,
            cubeCount.load());
}

static void searchMovesTa(const CubesAtDepth *cubesByDepth,
		const cube *csearch, int depth, int depthMax, std::atomic_int *lastCornersPerm,
		std::atomic_bool *isFinish, int reqFd)
{
    int cornerPerm;

	while( (cornerPerm = ++*lastCornersPerm) < 40320 && !isFinish->load() ) {
        const CornerPermCubes &ccpCubes = cubesByDepth[depth][cornerPerm];
        cubecorner_perms ccp = cornersIdxToPerm(cornerPerm);
        cubecorner_perms ccpSearch = cubecornerPermsCompose(ccp, csearch->cc.getPerms());
        unsigned cornerPermSearch = cornersPermToIdx(ccpSearch);
        const CornerPermCubes &cornerPermCubesSearch = cubesByDepth[depthMax][cornerPermSearch];
        for(CornerPermCubes::ccorients_iter ccorientIt = ccpCubes.ccorientsBegin();
                ccorientIt != ccpCubes.ccorientsEnd(); ++ccorientIt)
        {
            const CornerOrientCubes &ccoCubes = *ccorientIt;
            cubecorner_orients cco = ccoCubes.cornerOrients();
            cubecorner_orients ccoSearch = cubecornerOrientsCompose(cco, csearch->cc);
            const CornerOrientCubes *corientCubesSearch = cornerPermCubesSearch.findCornerOrientCubes(ccoSearch);
            if( corientCubesSearch != NULL ) {
                for(CornerOrientCubes::edges_iter edgeIt = ccoCubes.edgeBegin();
                        edgeIt != ccoCubes.edgeEnd(); ++edgeIt)
                {
                    const cubeedges ce = *edgeIt;
                    cubeedges ceSearch = cubeedges::compose(ce, csearch->ce);
                    if( corientCubesSearch->containsCubeEdges(ceSearch) ) {
                        cube c = { .cc = cubecorners(ccp, cco), .ce = ce };
                        std::string moves = "solution:";
                        cube cnew = { .cc = cubecorners(ccpSearch, ccoSearch), .ce = ceSearch };
                        moves += printMovesRev(cubesByDepth, cnew);
                        moves += printMoves(cubesByDepth, c);
                        moves += "\n";
                        flockfile(stdout);
                        sendRespMessage(reqFd, moves.c_str());
                        funlockfile(stdout);
                        isFinish->store(true);
                        return;
                    }
                }
            }
        }
    }
}

static bool searchMovesA(const CubesAtDepth *cubesByDepth,
		const cube &csearch, int depth, int depthMax,
        int threadCount, int fdReq)
{
    std::thread threads[threadCount];
    std::atomic_bool isFinish(false);
    std::atomic_int idx(-1);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTa, cubesByDepth, &csearch, depth, depthMax,
                &idx, &isFinish, fdReq);
    }
    searchMovesTa(cubesByDepth, &csearch, depth, depthMax, &idx, &isFinish, fdReq);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    if( isFinish.load() ) {
        sendRespMessage(fdReq, "%s -- %d moves --\n", fmt_time().c_str(), depth+depthMax);
        return true;
    }
    sendRespMessage(fdReq, "%s depth %d end\n", fmt_time().c_str(), depth+depthMax);
    return false;
}

static void searchMovesTaRepr(const CubesAtDepth *cubesByDepth,
        const CubesReprAtDepth *cubesReprByDepth,
		const cube *csearch, std::atomic_int *lastCornersPerm,
		std::atomic_bool *isFinish, int reqFd)
{
    int cornerPerm;
    std::vector<int> otransform;

	while( (cornerPerm = ++*lastCornersPerm) < 40320 && !isFinish->load() ) {
        const CornerPermCubes &ccpCubes = cubesByDepth[DEPTH_MAX][cornerPerm];
        cubecorner_perms ccp = cornersIdxToPerm(cornerPerm);
        cubecorner_perms ccpSearch = cubecornerPermsCompose(ccp, csearch->cc.getPerms());
        unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpSearch);
        const CornerPermReprCubes &ccpReprCubes = cubesReprByDepth[0][cornerPermReprIdx];
        for(CornerPermCubes::ccorients_iter ccorientIt = ccpCubes.ccorientsBegin();
                ccorientIt != ccpCubes.ccorientsEnd(); ++ccorientIt)
        {
            const CornerOrientCubes &ccoCubes = *ccorientIt;
            cubecorner_orients cco = ccoCubes.cornerOrients();
            cubecorner_orients ccoSearch = cubecornerOrientsCompose(cco, csearch->cc);
            cubecorner_orients ccoSearchRepr = cubecornerOrientsRepresentative(ccpSearch, ccoSearch, otransform);
            unsigned corientIdxSearch = cornersOrientToIdx(ccoSearchRepr);
            const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(corientIdxSearch);
            if( ccoReprCubes.empty() )
                continue;
            for(CornerOrientCubes::edges_iter edgeIt = ccoCubes.edgeBegin();
                    edgeIt != ccoCubes.edgeEnd(); ++edgeIt)
            {
                const cubeedges ce = *edgeIt;
                cubeedges ceSearch = cubeedges::compose(ce, csearch->ce);
                cubeedges ceSearchRepr = cubeedgesRepresentative(ceSearch, otransform);
                if( ccoReprCubes.containsCubeEdges(ceSearchRepr) ) {
                    cube c = { .cc = cubecorners(ccp, cco), .ce = ce };
                    std::string moves = "solution:";
                    cube csearch = { .cc = cubecorners(ccpSearch, ccoSearch), .ce = ceSearch };
                    moves += printMovesRevDeep(cubesByDepth, csearch);
                    moves += printMoves(cubesByDepth, c);
                    moves += "\n";
                    flockfile(stdout);
                    sendRespMessage(reqFd, moves.c_str());
                    funlockfile(stdout);
                    isFinish->store(true);
                    return;
                }
            }
        }
    }
}

static bool searchMovesArepr(const CubesAtDepth *cubesByDepth,
        const CubesReprAtDepth *cubesReprByDepth,
		const cube &csearch, int depthRepr, int threadCount, int fdReq)
{
    std::thread threads[threadCount];
    std::atomic_bool isFinish(false);
    std::atomic_int idx(-1);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTaRepr, cubesByDepth, cubesReprByDepth+depthRepr, &csearch,
                &idx, &isFinish, fdReq);
    }
    searchMovesTaRepr(cubesByDepth, cubesReprByDepth+depthRepr, &csearch, &idx, &isFinish, fdReq);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    if( isFinish.load() ) {
        sendRespMessage(fdReq, "%s -- %d moves --\n", fmt_time().c_str(), 2*DEPTH_MAX+depthRepr+1);
        return true;
    }
    sendRespMessage(fdReq, "%s depth %d end\n", fmt_time().c_str(), 2*DEPTH_MAX+depthRepr+1);
    return false;
}

struct SearchProgress {
    static std::mutex progressMutex;
    unsigned nextCornerPerm1Idx;
    unsigned nextCornerPerm2Idx;
    bool isFinish;
    std::vector<unsigned> filledCornerPerm1;
    std::vector<unsigned> filledCornerPerm2;
    const int threadCount;
    int runningThreadCount;

    bool inc(int reqFd, unsigned *cornerPerm1Buf,
            std::vector<unsigned>::const_iterator *cornerPerm2BegBuf,
            std::vector<unsigned>::const_iterator *cornerPerm2EndBuf)
    {
        bool res;
        unsigned cornerPerm1Idx, cornerPerm2Idx;
        progressMutex.lock();
        if( cornerPerm1Buf == NULL )
            isFinish = true;
        res = !isFinish && nextCornerPerm1Idx < filledCornerPerm1.size();
        if( res ) {
            cornerPerm1Idx = nextCornerPerm1Idx;
            cornerPerm2Idx = nextCornerPerm2Idx;
            nextCornerPerm2Idx += 1000;
            if( nextCornerPerm2Idx >= filledCornerPerm2.size() ) {
                ++nextCornerPerm1Idx;
                nextCornerPerm2Idx = 0;
            }
        }else
            --runningThreadCount;
        progressMutex.unlock();
        if( res ) {
            *cornerPerm1Buf = filledCornerPerm1[cornerPerm1Idx];
            *cornerPerm2BegBuf = filledCornerPerm2.begin() + cornerPerm2Idx;
            *cornerPerm2EndBuf = cornerPerm2Idx + 1000 >= filledCornerPerm2.size() ?
                filledCornerPerm2.end() : *cornerPerm2BegBuf + 1000;
            if( cornerPerm2Idx == 0 ) {
                unsigned itemCount = filledCornerPerm1.size();
                sendRespMessage(reqFd, "progress: %d of %d, %.0f%%\n",
                        cornerPerm1Idx, itemCount, 100.0 * cornerPerm1Idx / itemCount);
            }
        }else{
            sendRespMessage(reqFd, "progress: %d threads still running\n",
                    runningThreadCount);
        }
        return res;
    }

    SearchProgress(int threadCount)
        : nextCornerPerm1Idx(0), nextCornerPerm2Idx(0), isFinish(false),
        threadCount(threadCount), runningThreadCount(threadCount)
    {
    }
};

std::mutex SearchProgress::progressMutex;

static void searchMovesTb(const CubesAtDepth *cubesByDepth,
        const CubesReprAtDepth *cubesReprByDepth,
		const cube *csearch, int depth,
        int reqFd, SearchProgress *searchProgress)
{
    unsigned cornerPerm1;
    std::vector<int> o2transform;

    std::vector<unsigned>::const_iterator cornerPerm2Beg, cornerPerm2End;
	while( searchProgress->inc(reqFd, &cornerPerm1, &cornerPerm2Beg, &cornerPerm2End) ) {
		const CornerPermCubes &ccpCubes1 = cubesByDepth[depth][cornerPerm1];
		cubecorner_perms ccp1 = cornersIdxToPerm(cornerPerm1);
		cubecorner_perms ccpSearch1 = cubecornerPermsCompose(ccp1, csearch->cc.getPerms());
        for(std::vector<unsigned>::const_iterator cornerPerm2It = cornerPerm2Beg;
                cornerPerm2It != cornerPerm2End; ++cornerPerm2It)
        {
            unsigned cornerPerm2 = *cornerPerm2It;
            const CornerPermCubes &ccpCubes2 = cubesByDepth[DEPTH_MAX][cornerPerm2];
            cubecorner_perms ccp2 = cornersIdxToPerm(cornerPerm2);
            cubecorner_perms ccpSearch2 = cubecornerPermsCompose(ccp2, ccpSearch1);
            unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpSearch2);
            const CornerPermReprCubes &ccpReprCubes = cubesReprByDepth[0][cornerPermReprIdx];
            for(CornerPermCubes::ccorients_iter ccorient1It = ccpCubes1.ccorientsBegin();
                    ccorient1It != ccpCubes1.ccorientsEnd(); ++ccorient1It)
            {
                const CornerOrientCubes &ccoCubes1 = *ccorient1It;
                cubecorner_orients cco1 = ccoCubes1.cornerOrients();
                cubecorner_orients ccoSearch1 = cubecornerOrientsCompose(cco1, csearch->cc);
                for(CornerPermCubes::ccorients_iter ccorient2It = ccpCubes2.ccorientsBegin();
                        ccorient2It != ccpCubes2.ccorientsEnd(); ++ccorient2It)
                {
                    const CornerOrientCubes &ccoCubes2 = *ccorient2It;
                    cubecorner_orients cco2 = ccoCubes2.cornerOrients();
                    cubecorner_orients ccoSearch2 = cubecornerOrientsCompose(cco2, ccpSearch1, ccoSearch1);
                    cubecorner_orients ccoSearch2Repr = cubecornerOrientsRepresentative(
                            ccpSearch2, ccoSearch2, o2transform);
                    unsigned corientIdxSearch2 = cornersOrientToIdx(ccoSearch2Repr);
                    const CornerOrientReprCubes &corientReprCubes2 = ccpReprCubes.cornerOrientCubesAt(corientIdxSearch2);
                    if( corientReprCubes2.empty() )
                        continue;
                    for(CornerOrientCubes::edges_iter edge1It = ccoCubes1.edgeBegin();
                            edge1It != ccoCubes1.edgeEnd(); ++edge1It)
                    {
                        const cubeedges ce1 = *edge1It;
                        cubeedges ceSearch1 = cubeedges::compose(ce1, csearch->ce);
                        for(CornerOrientCubes::edges_iter edge2It = ccoCubes2.edgeBegin();
                                edge2It != ccoCubes2.edgeEnd(); ++edge2It)
                        {
                            const cubeedges ce2 = *edge2It;
                            cubeedges cesearch2 = cubeedges::compose(ce2, ceSearch1);
                            cubeedges cesearch2Repr = cubeedgesRepresentative(cesearch2, o2transform);
                            if( corientReprCubes2.containsCubeEdges(cesearch2Repr) ) {
                                std::string moves = "solution:";
                                cube csearch2 = {
                                    .cc = cubecorners(ccpSearch2, ccoSearch2),
                                    .ce = cesearch2
                                };
                                moves += printMovesRevDeep(cubesByDepth, csearch2);

                                cube csearch1 = {
                                    .cc = cubecorners(ccpSearch1, ccoSearch1),
                                    .ce = ceSearch1
                                };
                                cube c2 = { .cc = cubecorners(ccp2, cco2), .ce = ce2 };
                                moves += printMoves(cubesByDepth, c2);

                                cube c1 = { .cc = cubecorners(ccp1, cco1), .ce = ce1 };
                                moves += printMoves(cubesByDepth, c1);
                                moves += "\n";
                                flockfile(stdout);
                                sendRespMessage(reqFd, moves.c_str());
                                funlockfile(stdout);
                                searchProgress->inc(reqFd, NULL, NULL, NULL);
                                return;
                            }
                        }
                    }
                }
            }
        }
	}
}

static bool searchMovesB(const CubesAtDepth *cubesByDepth,
        const CubesReprAtDepth *cubesReprByDepth,
		const cube &csearch, int depth, int depthRepr, int threadCount, int fdReq)
{
    SearchProgress searchProgress(threadCount);
    for(unsigned i = 0; i < 40320; ++i) {
        if( !cubesByDepth[depth][i].empty() )
            searchProgress.filledCornerPerm1.push_back(i);
        if( !cubesByDepth[DEPTH_MAX][i].empty() )
            searchProgress.filledCornerPerm2.push_back(i);
    }
    std::thread threads[threadCount];
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTb, cubesByDepth, cubesReprByDepth+depthRepr, &csearch, depth,
                fdReq, &searchProgress);
    }
    searchMovesTb(cubesByDepth, cubesReprByDepth+depthRepr, &csearch, depth, fdReq, &searchProgress);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    if( searchProgress.isFinish ) {
        sendRespMessage(fdReq, "%s -- %d moves --\n", fmt_time().c_str(), depth+2*DEPTH_MAX+depthRepr+1);
        return true;
    }
    sendRespMessage(fdReq, "%s depth %d end\n", fmt_time().c_str(), depth+2*DEPTH_MAX+depthRepr+1);
    return false;
}

static void searchMoves(const cube &csearch, int threadCount, int fdReq)
{
    static CubesAtDepth cubesByDepth[CSARR_SIZE];
    static CubesReprAtDepth cubesReprByDepth[CSREPR_SIZE];

    fmt_time(); // reset time elapsed
    sendRespMessage(fdReq, "%s\n", getSetup().c_str());
    if( cubesByDepth[0].empty() ) {
        cubesByDepth[0].init();
        unsigned short cornerPermSolved = cornersPermToIdx(csolved.cc.getPerms());
        CornerOrientCubes &ccoCubes = cubesByDepth[0][cornerPermSolved].cornerOrientCubesAddIfAbsent(csolved.cc.getOrients());
        ccoCubes.addCube(csolved.ce);
    }
	for(int depth = 1; depth <= DEPTH_MAX; ++depth) {
        if( cubesByDepth[depth].empty() )
            addCubes(cubesByDepth, depth, threadCount, fdReq);
        if( searchMovesA(cubesByDepth, csearch, depth-1, depth, threadCount, fdReq) )
            return;
        if( searchMovesA(cubesByDepth, csearch, depth, depth, threadCount, fdReq) )
            return;
    }
    if( cubesReprByDepth[0].empty() )
        addCubesRepr(cubesByDepth, cubesReprByDepth, threadCount, fdReq);
    if( searchMovesArepr(cubesByDepth, cubesReprByDepth, csearch, 0, threadCount, fdReq) )
        return;
    if( DREPR_COUNT > 1 ) {
        if( cubesReprByDepth[1].empty() )
            addCubesRepr2(cubesByDepth, cubesReprByDepth, threadCount, fdReq);
        if( searchMovesArepr(cubesByDepth, cubesReprByDepth, csearch, 1, threadCount, fdReq) )
            return;
    }
    for(int depthRepr = 2; depthRepr < DREPR_COUNT; ++depthRepr) {
        if( cubesReprByDepth[depthRepr].empty() )
            addCubesRepr3(cubesReprByDepth, depthRepr, threadCount, fdReq);
        if( searchMovesArepr(cubesByDepth, cubesReprByDepth, csearch, depthRepr, threadCount, fdReq) )
            return;
    }
	for(int depth = 1; depth <= DEPTH_MAX; ++depth) {
        if( searchMovesB(cubesByDepth, cubesReprByDepth, csearch, depth, DREPR_COUNT-1, threadCount, fdReq) )
            return;
	}
    sendRespMessage(fdReq, "not found\n");
}

static void processCubeReq(int fdReq, const char *squareColors)
{
    char respHeader[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-type: text/plain\r\n"
        "Transfer-encoding: chunked\r\n"
        "Connection: close\r\n"
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
}

static void processRequest(int fdReq, const char *reqHeader) {
    if( strncasecmp(reqHeader, "get ", 4) ) {
        char respHeader[] =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-length: 0\r\n"
            "Allow: GET\r\n"
            "Connection: close\r\n"
            "\r\n"; 
        write(fdReq, respHeader, strlen(respHeader));
        return;
    }
    bool resourceFound = false;
    if( reqHeader[4] == '/' ) {
        if( reqHeader[5] == '?' ) {
            resourceFound = true;
            processCubeReq(fdReq, reqHeader+6);
        }else{
            const char *fnameBeg = reqHeader + 5;
            const char *fnameEnd = strchr(fnameBeg, ' ');
            if( fnameEnd != NULL ) {
                std::string fname;
                fname.assign(fnameBeg, fnameEnd - fnameBeg);
                if( fname.empty() )
                    fname = "cube.html";
                FILE *fp = fopen(fname.c_str(), "r");
                if( fp != NULL ) {
                    resourceFound = true;
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
                        "Connection: close\r\n"
                        "\r\n", contentType, fsize); 
                    write(fdReq, respHeader, strlen(respHeader));
                    rewind(fp);
                    char fbuf[32768];
                    long toWr = fsize;
                    while( toWr > 0 ) {
                        int toRd = toWr > sizeof(fbuf) ? sizeof(fbuf) : toWr;
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
                }
            }
        }
    }
    if( ! resourceFound ) {
        char respHeader[] =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-length: 0\r\n"
            "Connection: close\r\n"
            "\r\n"; 
        write(fdReq, respHeader, strlen(respHeader));
    }
}

static void processConnection(int fdConn) {
    char reqHeaderBuf[16385] = { 0 }, *reqHeaderEnd;
    int rdTot = 0;
    while( true ) {
        int rd = read(fdConn, reqHeaderBuf + rdTot, sizeof(reqHeaderBuf) - rdTot - 1);
        if( rd < 0 ) {
            perror("read");
            return;
        }
        if( rd == 0 ) {
            printf("connection closed by peer\n");
            return;
        }
        write(STDOUT_FILENO, reqHeaderBuf+rdTot, rd);
        rdTot += rd;
        if( (reqHeaderEnd = strstr(reqHeaderBuf, "\r\n\r\n")) != NULL ) {
            printf("---- sending response ----\n");
            reqHeaderEnd[2] = '\0';
            processRequest(fdConn, reqHeaderBuf);
            return;
        }
        if( rdTot == sizeof(reqHeaderBuf)-1 ) {
            fprintf(stderr, "request header too long\n");
            return;
        }
    }
}

int main(int argc, char *argv[]) {
    int listenfd, acceptfd, opton = 1;
    struct sockaddr_in addr;

    if( argc >= 3 ) {
        DEPTH_MAX = atoi(argv[1]);
        DREPR_COUNT = atoi(argv[2]);
        if( DEPTH_MAX <= 0 || DEPTH_MAX >= CSARR_SIZE ) {
            printf("DEPTH_MAX invalid: must be in range 1..%d\n", CSARR_SIZE-1);
            return 1;
        }
        if( DREPR_COUNT < 1 || DREPR_COUNT > DEPTH_MAX || DREPR_COUNT > CSREPR_SIZE ) {
            printf("DREPR_COUNT invalid: must be in range 1..%d\n", CSREPR_SIZE);
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
        processConnection(acceptfd);
        close(acceptfd);
    }
    perror("accept");
	return 1;
}

