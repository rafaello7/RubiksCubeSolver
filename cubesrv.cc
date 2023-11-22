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
static unsigned TWOPHASE_SEARCHREV = 2;
static const unsigned THREAD_COUNT = std::max(4u, std::thread::hardware_concurrency());

enum {
    TWOPHASE_DEPTH1_CATCHFIRST_MAX = 5u,
    TWOPHASE_DEPTH1_MULTI_MAX = 6u,
    TWOPHASE_DEPTH2_MAX = 8u
};


class Responder {
public:
    void message(const char *fmt, ...);
    void progress(const char *fmt, ...);
    void movecount(const char *fmt, ...);
    void solution(const char *fmt, ...);
protected:
    enum MessageType {
        MT_UNQUALIFIED,
        MT_PROGRESS,
        MT_MOVECOUNT,
        MT_SOLUTION
    };
    virtual void handleMessage(MessageType, const char*) = 0;
};

void Responder::message(const char *fmt, ...) {
    char msg[2000];
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);
    handleMessage(MT_UNQUALIFIED, msg);
}

void Responder::progress(const char *fmt, ...) {
    char msg[2000];
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);
    handleMessage(MT_PROGRESS, msg);
}

void Responder::movecount(const char *fmt, ...) {
    char msg[2000];
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);
    handleMessage(MT_MOVECOUNT, msg);
}

void Responder::solution(const char *fmt, ...) {
    char msg[2000];
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);
    handleMessage(MT_SOLUTION, msg);
}

template<typename Fn, typename... Ts>
static void runInThreadPool(Fn fn, Ts... args)
{
    std::thread threads[THREAD_COUNT-1];
    for(unsigned t = 1; t < THREAD_COUNT; ++t)
        threads[t-1] = std::thread(fn, t, args...);
    fn(0, args...);
    for(unsigned t = 1; t < THREAD_COUNT; ++t)
        threads[t-1].join();
}

template<unsigned l>
class MeasureTimeHelper {
    static std::atomic_uint m_callCnt;
    static std::atomic_ulong m_durTot;
    std::chrono::time_point<std::chrono::system_clock> m_pre;
    const char *m_fnname;
public:
    MeasureTimeHelper(const char *fnname)
        : m_pre(std::chrono::system_clock::now()), m_fnname(fnname)
    {
        ++m_callCnt;
    }

    ~MeasureTimeHelper() {
        std::chrono::time_point<std::chrono::system_clock> post(std::chrono::system_clock::now());
        auto durns = post - m_pre;
        m_durTot += durns.count();
        std::chrono::milliseconds dur = std::chrono::duration_cast<std::chrono::milliseconds>(durns);
        std::cout << m_fnname << " " << m_callCnt << " " << dur.count() << " ms, total " << m_durTot/1000000 << " ms" << std::endl;
    }
};

template<unsigned l>
std::atomic_uint MeasureTimeHelper<l>::m_callCnt;
template<unsigned l>
std::atomic_ulong MeasureTimeHelper<l>::m_durTot;

#define MeasureTime MeasureTimeHelper<__LINE__> measureTime(__func__)

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

// The cube corner and edge numbers
//           ___________
//           | YELLOW  |
//           | 4  8  5 |
//           | 4     5 |
//           | 0  0  1 |
//  _________|_________|_________ _________
// |  ORANGE |   BLUE  |   RED   |  GREEN  |
// | 4  4  0 | 0  0  1 | 1  5  5 | 5  8  4 |
// | 9     1 | 1     2 | 2    10 | 10    9 |
// | 6  6  2 | 2  3  3 | 3  7  7 | 7 11  6 |
// |_________|_________|_________|_________|
//           |  WHITE  |
//           | 2  3  3 |
//           | 6     7 |
//           | 6 11  7 |
//           |_________|
//
// The cube corner and edge orientation referential squares
// if a referential square of a corner/edge being moved goes into a
// referential square, the corner/edge orientation is not changed
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

static int rotateNameToDir(const char *rotateName) {
    int rd;
    for(rd = 0; rd < RCOUNT; ++rd) {
        const char *dirName = rotateDirName(rd);
        if( !strncmp(rotateName, dirName, strlen(dirName) ) )
            break;
    }
    return rd;
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

// The cube transformation (colors switch) through the cube rotation
enum transform_dir {
    TD_0,           // no rotation
    TD_C0_7_CW,     // rotation along axis from corner 0 to 7, 120 degree clockwise
    TD_C0_7_CCW,    // rotation along axis from corner 0 to 7, counterclockwise
    TD_C1_6_CW,
    TD_C1_6_CCW,
    TD_C2_5_CW,
    TD_C2_5_CCW,
    TD_C3_4_CW,
    TD_C3_4_CCW,
    TD_BG_CW,       // rotation along axis through the wall middle, from blue to green wall, clockwise
    TD_BG_180,
    TD_BG_CCW,
    TD_YW_CW,
    TD_YW_180,
    TD_YW_CCW,
    TD_OR_CW,
    TD_OR_180,
    TD_OR_CCW,
    TD_E0_11,       // rotation along axis from edge 0 to 11, 180 degree
    TD_E1_10,
    TD_E2_9,
    TD_E3_8,
    TD_E4_7,
    TD_E5_6,
    TCOUNT
};

static int transformReverse(int idx) {
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

class cubecorners_perm {
	unsigned perm;
public:
	cubecorners_perm() : perm(0) {}
	cubecorners_perm(unsigned corner0perm, unsigned corner1perm, unsigned corner2perm,
			unsigned corner3perm, unsigned corner4perm, unsigned corner5perm,
			unsigned corner6perm, unsigned corner7perm);

	void setAt(unsigned idx, unsigned char p) {
		perm &= ~(7 << 3*idx);
		perm |= p << 3*idx;
	}
	unsigned getAt(unsigned idx) const { return perm >> 3*idx & 7; }
	unsigned get() const { return perm; }
	void set(unsigned p) { perm = p; }
    static cubecorners_perm compose(cubecorners_perm, cubecorners_perm);
    static cubecorners_perm compose3(cubecorners_perm, cubecorners_perm, cubecorners_perm);
    cubecorners_perm symmetric() const {
        unsigned permRes = perm ^ 0x924924;
        cubecorners_perm ccpres;
        ccpres.perm = (permRes >> 12 | permRes << 12) & 0xffffff;
        return ccpres;
    }
    cubecorners_perm reverse() const;
    cubecorners_perm transform(unsigned transformDir) const;
	bool operator==(const cubecorners_perm &ccp) const { return perm == ccp.perm; }
	bool operator!=(const cubecorners_perm &ccp) const { return perm != ccp.perm; }
	bool operator<(const cubecorners_perm &ccp) const { return perm < ccp.perm; }
    unsigned short getPermIdx() const;
    static cubecorners_perm fromPermIdx(unsigned short);
    bool isPermParityOdd() const;
};

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
            cubecorners_perm ccp2, cubecorner_orients cco2);
    static cubecorner_orients compose3(cubecorner_orients cco1,
            cubecorners_perm ccp2, cubecorner_orients cco2,
            cubecorners_perm ccp3, cubecorner_orients cco3);
    cubecorner_orients symmetric() const {
        cubecorner_orients ccores;
        // set orient 2 -> 1, 1 -> 2, 0 unchanged
        unsigned short orie = (orients & 0xaaaa) >> 1 | (orients & 0x5555) << 1;
        ccores.orients = (orie >> 8 | orie << 8) & 0xffff;
        return ccores;
    }
    cubecorner_orients reverse(cubecorners_perm) const;
    cubecorner_orients transform(cubecorners_perm, unsigned transformDir) const;
	bool operator==(const cubecorner_orients &cco) const { return orients == cco.orients; }
	bool operator!=(const cubecorner_orients &cco) const { return orients != cco.orients; }
	bool operator<(const cubecorner_orients &cco) const { return orients < cco.orients; }
    unsigned short getOrientIdx() const;
    static cubecorner_orients fromOrientIdx(unsigned short);
    bool isBGspace() const;
    bool isYWspace(cubecorners_perm) const;
    bool isORspace(cubecorners_perm) const;
    cubecorner_orients representativeBG(cubecorners_perm) const;
    cubecorner_orients representativeYW(cubecorners_perm) const;
    cubecorner_orients representativeOR(cubecorners_perm) const;
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

class cubeedges {
	unsigned long edges;
public:
	cubeedges() : edges(0) {}
	cubeedges(unsigned edge0perm, unsigned edge1perm, unsigned edge2perm,
              unsigned edge3perm, unsigned edge4perm, unsigned edge5perm,
              unsigned edge6perm, unsigned edge7perm, unsigned edge8perm,
              unsigned edge9perm, unsigned edge10perm,unsigned edge11perm,
              unsigned edge0orient, unsigned edge1orient, unsigned edge2orient,
              unsigned edge3orient, unsigned edge4orient, unsigned edge5orient,
              unsigned edge6orient, unsigned edge7orient, unsigned edge8orient,
              unsigned edge9orient, unsigned edge10orient, unsigned edge11orient);

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
    cubeedges symmetric() const {
        cubeedges ceres;
        // map perm: 0 1 2 3 4 5 6 7 8 9 10 11 -> 8 9 10 11 4 5 6 7 0 1 2 3
        unsigned long edg = edges ^ (~edges & 0x210842108421084ul) << 1;
        ceres.edges = edg >> 40 | edg & 0xfffff00000ul | (edg & 0xfffff) << 40;
        return ceres;
    }
    cubeedges reverse() const;
    cubeedges transform(int idx) const;
	unsigned getPermAt(unsigned idx) const { return edges >> 5 * idx & 0xf; }
	unsigned getOrientAt(unsigned idx) const { return edges >> (5*idx+4) & 1; }
    unsigned getItem11val() const {
        unsigned item11 = edges >> 55;
        item11 -= item11 >> 2 & 4;
        return item11;
    }
	bool operator==(const cubeedges &ce) const { return edges == ce.edges; }
	bool operator!=(const cubeedges &ce) const { return edges != ce.edges; }
	bool operator<(const cubeedges &ce) const { return edges < ce.edges; }
    unsigned getPermIdx() const;
    unsigned short getOrientIdx() const;
    static cubeedges fromPermAndOrientIdx(unsigned permIdx, unsigned short orientIdx);
    bool isPermParityOdd() const;
    unsigned long get() const { return edges; }
    void set(unsigned long e) { edges = e; }
    bool isNil() const { return edges == 0; }
    bool isBGspace() const;
    bool isYWspace() const;
    bool isORspace() const;
    cubeedges representativeBG() const;
    cubeedges representativeYW() const;
    cubeedges representativeOR() const;
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
    cube cchk = { .ccp = csolved.ccp, .cco = csolved.cco, .ce = cerepr };
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

struct cube {
	cubecorners_perm ccp;
    cubecorner_orients cco;
	cubeedges ce;

    static cube compose(const cube &c1, const cube &c2) {
        return {
            .ccp = cubecorners_perm::compose(c1.ccp, c2.ccp),
            .cco = cubecorner_orients::compose(c1.cco, c2.ccp, c2.cco),
            .ce = cubeedges::compose(c1.ce, c2.ce)
        };
    }
    cube symmetric() const {
        return {
            .ccp = ccp.symmetric(),
            .cco = cco.symmetric(),
            .ce = ce.symmetric()
        };
    }
    cube reverse() const {
        return {
            .ccp = ccp.reverse(),
            .cco = cco.reverse(ccp),
            .ce = ce.reverse()
        };
    }
    cube transform(unsigned transformDir) const;
	bool operator==(const cube &c) const;
	bool operator!=(const cube &c) const;
	bool operator<(const cube &c) const;
    bool isBGspace() const { return cco.isBGspace() && ce.isBGspace(); }
    bool isYWspace() const { return cco.isYWspace(ccp) && ce.isYWspace(); }
    bool isORspace() const { return cco.isORspace(ccp) && ce.isORspace(); }
    cube representativeBG() const;
    cube representativeYW() const;
    cube representativeOR() const;
    std::string toParamText() const;
};

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

struct ReprCandidateTransform {
    bool reversed;
    bool symmetric;
    unsigned short transformIdx;
};

struct CubecornerPermToRepr {
    int reprIdx = -1;       // index in gReprPerms
    std::vector<ReprCandidateTransform> transform;
};

static std::vector<cubecorners_perm> gReprPerms;
static std::vector<CubecornerPermToRepr> gPermToRepr(40320);

static void permReprInit()
{
    gReprPerms.reserve(USEREVERSE ? 654 : 984);
    for(unsigned pidx = 0; pidx < 40320; ++pidx) {
        cubecorners_perm perm = cubecorners_perm::fromPermIdx(pidx);
        cubecorners_perm permRepr;
        std::vector<ReprCandidateTransform> transform;
        for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
            cubecorners_perm permr = reversed ? perm.reverse() : perm;
            for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
                cubecorners_perm permchk = symmetric ? permr.symmetric() : permr;
                for(unsigned short td = 0; td < TCOUNT; ++td) {
                    cubecorners_perm cand = permchk.transform(td);
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
        CubecornerPermToRepr &permToReprRepr = gPermToRepr[permRepr.getPermIdx()];
        if( permToReprRepr.reprIdx < 0 ) {
            permToReprRepr.reprIdx = gReprPerms.size();
            gReprPerms.push_back(permRepr);
        }
        CubecornerPermToRepr &permToRepr = gPermToRepr[perm.getPermIdx()];
        permToRepr.reprIdx = permToReprRepr.reprIdx;
        permToRepr.transform.swap(transform);
    }
    std::cout << "repr size=" << gReprPerms.size() << std::endl;
}

static unsigned cubecornerPermRepresentativeIdx(cubecorners_perm ccp)
{
    return gPermToRepr.at(ccp.getPermIdx()).reprIdx;
}

static cubecorners_perm cubecornerPermsRepresentative(cubecorners_perm ccp)
{
    unsigned permReprIdx = gPermToRepr.at(ccp.getPermIdx()).reprIdx;
    return gReprPerms[permReprIdx];
}

struct EdgeReprCandidateTransform {
    unsigned transformedIdx;
    bool reversed;
    bool symmetric;
    cubeedges ceTrans;
};

static cubecorner_orients cubecornerOrientsRepresentative(cubecorners_perm ccp, cubecorner_orients cco,
         std::vector<EdgeReprCandidateTransform> &transform)
{
    CubecornerPermToRepr permToRepr = gPermToRepr.at(ccp.getPermIdx());
    cubecorners_perm ccpsymm, ccprev, ccprevsymm;
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
                ocand = ccorevsymm.transform(ccprevsymm, rct.transformIdx);
            }else{
                ocand = ccorev.transform(ccprev, rct.transformIdx);
            }
        }else{
            if( rct.symmetric ) {
                if( !isSymmInit ) {
                    ccpsymm = ccp.symmetric();
                    ccosymm = cco.symmetric();
                    isSymmInit = true;
                }
                ocand = ccosymm.transform(ccpsymm, rct.transformIdx);
            }else{
                ocand = cco.transform(ccp, rct.transformIdx);
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
        cerepr = cechk.transform(erct.transformedIdx);
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
                    cand = cerevsymm.transform(erct.transformedIdx);
                }else{
                    cand = cerev.transform(erct.transformedIdx);
                }
            }else{
                if( erct.symmetric ) {
                    if( ! isSymmInit ) {
                        cesymm = ce.symmetric();
                        isSymmInit = true;
                    }
                    cand = cesymm.transform(erct.transformedIdx);
                }else{
                    cand = ce.transform(erct.transformedIdx);
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
        cubecorners_perm ccpComposed, cubecorner_orients ccoComposed,
        cubeedges ce2, std::vector<EdgeReprCandidateTransform> &transform)
{
    const CubecornerPermToRepr &permToRepr = gPermToRepr.at(ccpComposed.getPermIdx());
    cubecorners_perm ccpsymm, ccprev, ccprevsymm;
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
                ocand = ccorevsymm.transform(ccprevsymm, rct.transformIdx);
            }else{
                ocand = ccorev.transform(ccprev, rct.transformIdx);
            }
        }else{
            if( rct.symmetric ) {
                if( !isSymmInit ) {
                    ccpsymm = ccpComposed.symmetric();
                    ccosymm = ccoComposed.symmetric();
                    isSymmInit = true;
                }
                ocand = ccosymm.transform(ccpsymm, rct.transformIdx);
            }else{
                ocand = ccoComposed.transform(ccpComposed, rct.transformIdx);
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

static bool isSingleTransform(cubecorners_perm ccp) {
    return gPermToRepr[ccp.getPermIdx()].transform.size() == 1;
}

static cubecorner_orients cubecornerOrientsForComposedRepresentative(cubecorners_perm ccpSearch,
        cubecorner_orients ccoSearchRepr, const cube &cSearchT,
        cubecorners_perm ccprev, std::vector<EdgeReprCandidateTransform> &transform)
{
    const CubecornerPermToRepr &permToRepr = gPermToRepr.at(ccpSearch.getPermIdx());
    cubecorners_perm ccpSearchRepr = gReprPerms[permToRepr.reprIdx];
    cube cSearchTrev = cSearchT.reverse();
    cubecorner_orients ccorev;

    cubecorners_perm ccpSearchRev;
    cubecorner_orients ccoSearch;
    bool isRevInit = false;
	transform.clear();
    for(const ReprCandidateTransform &rct : permToRepr.transform) {
        unsigned transformIdxRev = transformReverse(rct.transformIdx);
        cubecorner_orients ocand;
        if( rct.reversed ) {
            if( !isRevInit ) {
                ccpSearchRev = ccpSearch.reverse();
                isRevInit = true;
            }
            cubecorner_orients ccoSearchRev;
            if( rct.symmetric ) {
                cubecorner_orients ccoSearchRevSymm = ccoSearchRepr.transform(ccpSearchRepr, transformIdxRev);
                ccoSearchRev = ccoSearchRevSymm.symmetric();
            }else{
                ccoSearchRev = ccoSearchRepr.transform(ccpSearchRepr, transformIdxRev);
            }
            ccoSearch = ccoSearchRev.reverse(ccpSearchRev);
        }else{
            if( rct.symmetric ) {
                cubecorner_orients ccoSearchSymm = ccoSearchRepr.transform(ccpSearchRepr, transformIdxRev);
                ccoSearch = ccoSearchSymm.symmetric();
            }else{
                ccoSearch = ccoSearchRepr.transform(ccpSearchRepr, transformIdxRev);
            }
        }
        ccorev = cubecorner_orients::compose(ccoSearch, cSearchTrev.ccp, cSearchTrev.cco);
        transform.push_back({ .transformedIdx = rct.transformIdx,
                    .reversed = rct.reversed, .symmetric = rct.symmetric });
    }
    for(EdgeReprCandidateTransform &erct : transform) {
        cubeedges ce2chk = erct.symmetric ? cSearchT.ce.symmetric() : cSearchT.ce;
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
    return ccorev;
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

    cubecorners_perm ccpRepr = cubecornerPermsRepresentative(c.ccp);
    cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(c.ccp,
            c.cco, transform);
    cubeedges ceRepr = cubeedgesRepresentative(c.ce, transform);
    return { .ccp = ccpRepr, .cco = ccoRepr, .ce = ceRepr };
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

static bool isCubeSolvable(Responder &responder, const cube &c)
{
    bool isSwapsOdd = false;
    unsigned permScanned = 0;
    for(int i = 0; i < 8; ++i) {
        if( permScanned & 1 << i )
            continue;
        permScanned |= 1 << i;
        int p = i;
        while( (p = c.ccp.getAt(p)) != i ) {
            if( permScanned & 1 << p ) {
                responder.message("corner perm %d is twice", p);
                return false;
            }
            permScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
    permScanned = 0;
    for(int i = 0; i < 12; ++i) {
        if( permScanned & 1 << i )
            continue;
        permScanned |= 1 << i;
        int p = i;
        while( (p = c.ce.getPermAt(p)) != i ) {
            if( permScanned & 1 << p ) {
                responder.message("edge perm %d is twice", p);
                return false;
            }
            permScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
	if( isSwapsOdd ) {
		responder.message("cube unsolvable due to permutation parity");
		return false;
	}
	unsigned sumOrient = 0;
	for(int i = 0; i < 8; ++i)
		sumOrient += c.cco.getAt(i);
	if( sumOrient % 3 ) {
		responder.message("cube unsolvable due to corner orientations");
		return false;
	}
	sumOrient = 0;
	for(int i = 0; i < 12; ++i)
		sumOrient += c.ce.getOrientAt(i);
	if( sumOrient % 2 ) {
		responder.message("cube unsolvable due to edge orientations");
		return false;
	}
    return true;
}

struct elemLoc {
	int wall;
    int row;
	int col;
};

static bool cubeRead(Responder &responder, const char *squareColors, cube &c)
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
            responder.message("bad letter at column %d", cno);
            return false;
        }
        walls[cno / 9][cno % 9 / 3][cno % 3] = lpos - colorLetters;
    }
	for(int i = 0; i < 6; ++i) {
		if( walls[i][1][1] != i ) {
			responder.message("bad orientation: wall=%d exp=%c is=%c",
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
				c.ccp.setAt(i, n);
				c.cco.setAt(i, 0);
				break;
			}
			match = true;
			for(int r = 0; r < 3 && match; ++r) {
				const elemLoc &el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[R120[r]] )
					match = false;
			}
			if( match ) {
				c.ccp.setAt(i, n);
				c.cco.setAt(i, 1);
				break;
			}
			match = true;
			for(int r = 0; r < 3 && match; ++r) {
				const elemLoc &el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[R240[r]] )
					match = false;
			}
			if( match ) {
				c.ccp.setAt(i, n);
				c.cco.setAt(i, 2);
				break;
			}
		}
		if( ! match ) {
			responder.message("corner %d not found", i);
			return false;
		}
		for(int j = 0; j < i; ++j) {
			if( c.ccp.getAt(i) == c.ccp.getAt(j) ) {
				responder.message("corner %d is twice: at %d and %d",
						c.ccp.getAt(i), j, i);
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
			responder.message("edge %d not found", i);
			return false;
		}
		for(int j = 0; j < i; ++j) {
			if( c.ce.getPermAt(i) == c.ce.getPermAt(j) ) {
				responder.message("edge %d is twice: at %d and %d",
						c.ce.getPermAt(i), j, i);
				return false;
			}
		}
	}
    if( ! isCubeSolvable(responder, c) )
        return false;
    return true;
}

enum SpaceKind {
    SPACEBG, SPACEYW, SPACEOR, SPACECOUNT
};

static bool isRotateKind(unsigned spaceKind, unsigned rotateDir) {
    switch( spaceKind ) {
        case SPACEBG:
            switch(rotateDir) {
                case ORANGE180:
                case RED180:
                case YELLOW180:
                case WHITE180:
                case GREENCW:
                case GREEN180:
                case GREENCCW:
                case BLUECW:
                case BLUE180:
                case BLUECCW:
                    return true;
            }
            break;
        case SPACEYW:
            switch(rotateDir) {
                case ORANGE180:
                case RED180:
                case YELLOW180:
                case WHITE180:
                case GREEN180:
                case BLUE180:
                case YELLOWCW:
                case YELLOWCCW:
                case WHITECW:
                case WHITECCW:
                    return true;
            }
            break;
        case SPACEOR:
            switch(rotateDir) {
                case ORANGE180:
                case RED180:
                case YELLOW180:
                case WHITE180:
                case GREEN180:
                case BLUE180:
                case ORANGECW:
                case ORANGECCW:
                case REDCW:
                case REDCCW:
                    return true;
            }
            break;
        default:
            break;
    }
    return false;
}

const char spaceKindName[SPACECOUNT][3] = { "BG", "YW", "OR" };

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
    std::vector<unsigned> m_item11idxs;
    cubecorner_orients m_orients;
    CornerOrientReprCubes(const CornerOrientReprCubes&) = delete;
    static cubeedges findSolutionEdgeMulti(
            const CornerOrientReprCubes &ccoReprCubes,
            const CornerOrientReprCubes &ccoReprSearchCubes,
            const std::vector<EdgeReprCandidateTransform> &otransform,
            bool edgeReverse);
    static cubeedges findSolutionEdgeSingle(
            const CornerOrientReprCubes &ccoReprCubes,
            const CornerOrientReprCubes &ccoReprSearchCubes,
            const EdgeReprCandidateTransform&, bool edgeReverse);
public:
    explicit CornerOrientReprCubes(cubecorner_orients orients)
        : m_orients(orients)
    {
    }

    CornerOrientReprCubes(CornerOrientReprCubes &&other) {
        swap(other);
    }

    typedef std::vector<cubeedges>::const_iterator edges_iter;
    cubecorner_orients getOrients() const { return m_orients; }
	bool addCube(cubeedges);
    void initOccur();
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
    void swap(CornerOrientReprCubes &other) {
        m_items.swap(other.m_items);
        m_orientOccur.swap(other.m_orientOccur);
        m_item11idxs.swap(other.m_item11idxs);
        std::swap(m_orients, other.m_orients);
    }
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
    return true;
}

void CornerOrientReprCubes::initOccur() {
    m_orientOccur.resize(64);
    m_item11idxs.resize(24);
    for(unsigned i = 0; i < m_items.size(); ++i) {
        cubeedges ce = m_items[i];
        unsigned short orientIdx = ce.getOrientIdx();
        m_orientOccur[orientIdx >> 5] |= 1ul << (orientIdx & 0x1f);
        unsigned item11val = ce.getItem11val();
        if( item11val+1 < m_item11idxs.size() )
            m_item11idxs[item11val+1] = i+1;
    }
}

bool CornerOrientReprCubes::containsCubeEdges(cubeedges ce) const
{
    if( ! m_orientOccur.empty() ) {
        unsigned short orientIdx = ce.getOrientIdx();
        if( (m_orientOccur[orientIdx >> 5] & 1ul << (orientIdx & 0x1f)) == 0 )
            return false;
    }
	unsigned lo = 0, itemsEnd = m_items.size();
    if( !m_item11idxs.empty() ) {
        unsigned item11val = ce.getItem11val();
        lo = m_item11idxs[item11val];
        if( item11val+1 < m_item11idxs.size() )
            itemsEnd = m_item11idxs[item11val+1];
    }
    unsigned hi = itemsEnd;
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_items[mi] < ce )
			lo = mi + 1;
		else
			hi = mi;
	}
	return lo < itemsEnd && m_items[lo] == ce;
}

cubeedges CornerOrientReprCubes::findSolutionEdgeMulti(
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

cubeedges CornerOrientReprCubes::findSolutionEdgeSingle(
        const CornerOrientReprCubes &ccoReprCubes,
        const CornerOrientReprCubes &ccoReprSearchCubes,
        const EdgeReprCandidateTransform &erct,
        bool edgeReverse)
{
    cubeedges cetrans = ctransformed[erct.transformedIdx].ce;
    cubeedges cetransRev = ctransformed[transformReverse(erct.transformedIdx)].ce;
    if( ccoReprCubes.size() <= ccoReprSearchCubes.size() ) {
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
    }else{
        cubeedges erctCeTransRev = erct.ceTrans.reverse();
        for(CornerOrientReprCubes::edges_iter edgeIt = ccoReprSearchCubes.edgeBegin();
                edgeIt != ccoReprSearchCubes.edgeEnd(); ++edgeIt)
        {
            cubeedges ce = *edgeIt;
            cubeedges ceSearch;
            if( erct.reversed ) {
                ceSearch = cubeedges::compose3(erctCeTransRev, ce, cetrans);
            }else{
                ceSearch = cubeedges::compose3(cetransRev, ce, erctCeTransRev);
            }
            cubeedges cesymmSearch = erct.symmetric ? ceSearch.symmetric() : ceSearch;
            cubeedges cesymmrevSearch = edgeReverse == erct.reversed ? cesymmSearch : cesymmSearch.reverse();
            if( ccoReprCubes.containsCubeEdges(cesymmrevSearch) )
                return edgeReverse ? cesymmrevSearch.reverse() : cesymmrevSearch;
        }
    }
    return cubeedges();
}

cubeedges CornerOrientReprCubes::findSolutionEdge(
        const CornerOrientReprCubes &ccoReprCubes,
        const CornerOrientReprCubes &ccoReprSearchCubes,
        const std::vector<EdgeReprCandidateTransform> &otransform,
        bool edgeReverse)
{
    cubeedges cerev;
    if( otransform.size() == 1 )
        cerev = CornerOrientReprCubes::findSolutionEdgeSingle(
                ccoReprCubes, ccoReprSearchCubes, otransform.front(), edgeReverse);
    else{
        cerev = CornerOrientReprCubes::findSolutionEdgeMulti(
                ccoReprCubes, ccoReprSearchCubes, otransform, edgeReverse);
    }
    return cerev;
}

class CornerPermReprCubes {
    static const CornerOrientReprCubes m_coreprCubesEmpty;
    std::vector<CornerOrientReprCubes> m_coreprCubes;

public:
    typedef std::vector<CornerOrientReprCubes>::const_iterator ccocubes_iter;
	CornerPermReprCubes();
	~CornerPermReprCubes();
    bool empty() const { return m_coreprCubes.empty(); }
    unsigned size() const { return m_coreprCubes.size(); }
    size_t cubeCount() const;
    void initOccur();
    const CornerOrientReprCubes &cornerOrientCubesAt(cubecorner_orients cco) const {
        unsigned lo = 0, hi = m_coreprCubes.size();
        while( lo < hi ) {
            unsigned mi = (lo+hi) / 2;
            if( m_coreprCubes[mi].getOrients() < cco )
                lo = mi + 1;
            else
                hi = mi;
        }
        if( lo < m_coreprCubes.size() && m_coreprCubes[lo].getOrients() == cco )
            return m_coreprCubes[lo];
        return m_coreprCubesEmpty;
    }
	CornerOrientReprCubes &cornerOrientCubesAdd(cubecorner_orients);
    ccocubes_iter ccoCubesBegin() const { return m_coreprCubes.begin(); }
    ccocubes_iter ccoCubesEnd() const { return m_coreprCubes.end(); }
};

const CornerOrientReprCubes CornerPermReprCubes::m_coreprCubesEmpty = CornerOrientReprCubes(cubecorner_orients());

CornerPermReprCubes::CornerPermReprCubes()
{
}

CornerPermReprCubes::~CornerPermReprCubes()
{
}

size_t CornerPermReprCubes::cubeCount() const {
    size_t res = 0;
    for(ccocubes_iter it = ccoCubesBegin(); it != ccoCubesEnd(); ++it)
        res += it->size();
    return res;
}

void CornerPermReprCubes::initOccur() {
    for(unsigned i = 0; i < m_coreprCubes.size(); ++i)
        m_coreprCubes[i].initOccur();
}

CornerOrientReprCubes &CornerPermReprCubes::cornerOrientCubesAdd(cubecorner_orients cco) {
    unsigned lo = 0, hi = m_coreprCubes.size();
    while( lo < hi ) {
        unsigned mi = (lo+hi) / 2;
        if( m_coreprCubes[mi].getOrients() < cco )
            lo = mi + 1;
        else
            hi = mi;
    }
    if( lo == m_coreprCubes.size() || cco < m_coreprCubes[lo].getOrients() ) {
        hi = m_coreprCubes.size();
        m_coreprCubes.emplace_back(cco);
        while( hi > lo ) {
            m_coreprCubes[hi].swap(m_coreprCubes[hi-1]);
            --hi;
        }
    }
    return m_coreprCubes[lo];
}

class CubesReprAtDepth {
    std::vector<std::pair<cubecorners_perm, CornerPermReprCubes>> m_cornerPermReprCubes;
public:
    typedef std::vector<std::pair<cubecorners_perm, CornerPermReprCubes>>::const_iterator ccpcubes_iter;
    CubesReprAtDepth();
    CubesReprAtDepth(const CubesReprAtDepth&) = delete;
    ~CubesReprAtDepth();
    unsigned size() const { return m_cornerPermReprCubes.size(); }
    size_t cubeCount() const;
    CornerPermReprCubes &add(unsigned idx);
    void initOccur(unsigned idx);
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

size_t CubesReprAtDepth::cubeCount() const {
    size_t res = 0;
    for(const std::pair<cubecorners_perm, CornerPermReprCubes> &reprCube : m_cornerPermReprCubes)
        res += reprCube.second.cubeCount();
    return res;
}

void CubesReprAtDepth::initOccur(unsigned idx)
{
    m_cornerPermReprCubes[idx].second.initOccur();
}

CornerPermReprCubes &CubesReprAtDepth::add(unsigned idx) {
    return m_cornerPermReprCubes[idx].second;
}

class CubesReprByDepth {
    std::vector<CubesReprAtDepth> m_cubesAtDepths;
    unsigned m_availCount;
    CubesReprByDepth(const CubesReprByDepth&) = delete;
    CubesReprByDepth &operator=(const CubesReprByDepth&) = delete;
public:
    CubesReprByDepth(unsigned depthMax)
        : m_cubesAtDepths(depthMax+1), m_availCount(0)
    {
    }

    unsigned availCount() const { return m_availCount; }
    void incAvailCount() { ++m_availCount; }
    unsigned availMaxCount() const { return m_cubesAtDepths.size(); }
    const CubesReprAtDepth &operator[](unsigned idx) const { return m_cubesAtDepths[idx]; }
    CubesReprAtDepth &operator[](unsigned idx) { return m_cubesAtDepths[idx]; }
};

class SpaceReprCubesAtDepth {
    std::vector<std::pair<cubeedges, unsigned>> m_itemsArr[2187];
    std::vector<std::vector<cube>> m_cubeArr[2187];
public:
    bool addCube(unsigned ccoReprIdx, cubeedges ceRepr, const cube&);
    bool containsCCOrients(unsigned ccoReprIdx) const {
        return !m_itemsArr[ccoReprIdx].empty();
    }
    const std::vector<cube> *getCubesForCE(unsigned ccoReprIdx, cubeedges ceRepr) const;
};

bool SpaceReprCubesAtDepth::addCube(unsigned ccoReprIdx, cubeedges ceRepr, const cube &c)
{
    std::vector<std::pair<cubeedges, unsigned>> &items = m_itemsArr[ccoReprIdx];

    unsigned lo = 0, hi = items.size();
    while( lo < hi ) {
        unsigned mi = (lo+hi) / 2;
        if( items[mi].first < ceRepr )
            lo = mi + 1;
        else
            hi = mi;
    }
    bool res = lo == items.size() || items[lo].first != ceRepr;
    if( res ) {
        unsigned idx = m_cubeArr[ccoReprIdx].size();
        m_cubeArr[ccoReprIdx].emplace_back();
        if( lo == items.size() ) {
            items.emplace_back(std::make_pair(ceRepr, idx));
        }else{
            items.resize(items.size()+1);
            std::copy_backward(items.begin()+lo, items.end()-1, items.end());
            items[lo] = std::make_pair(ceRepr, idx);
        }
    }
    m_cubeArr[ccoReprIdx][items[lo].second].push_back(c);
    return res;
}

const std::vector<cube> *SpaceReprCubesAtDepth::getCubesForCE(unsigned ccoReprIdx, cubeedges ceRepr) const
{
    const std::vector<std::pair<cubeedges, unsigned>> &items = m_itemsArr[ccoReprIdx];

	unsigned lo = 0, hi = items.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( items[mi].first < ceRepr )
			lo = mi + 1;
		else
			hi = mi;
	}
	return lo < items.size() && items[lo].first == ceRepr ?
        &m_cubeArr[ccoReprIdx][items[lo].second] : NULL;
}

class SpaceReprCubes {
    std::vector<SpaceReprCubesAtDepth> m_cubesAtDepths;
    unsigned m_availCount;
    SpaceReprCubes(const SpaceReprCubes&) = delete;
    SpaceReprCubes &operator=(const SpaceReprCubes&) = delete;
public:
    SpaceReprCubes(unsigned size)
        : m_cubesAtDepths(size), m_availCount(0)
    {
    }

    unsigned availCount() const { return m_availCount; }
    void incAvailCount() { ++m_availCount; }
    unsigned availMaxCount() const { return m_cubesAtDepths.size(); }
    const SpaceReprCubesAtDepth &operator[](unsigned idx) const { return m_cubesAtDepths[idx]; }
    SpaceReprCubesAtDepth &operator[](unsigned idx) { return m_cubesAtDepths[idx]; }
};

static std::string printMoves(const CubesReprByDepth &cubesByDepth, const cube &c, bool movesRev = false)
{
	std::vector<int> rotateDirs;
    std::vector<int>::iterator insertPos = rotateDirs.end();
    cube crepr = cubeRepresentative(c);
    unsigned ccpReprIdx = cubecornerPermRepresentativeIdx(crepr.ccp);
	unsigned depth = 0;
	while( true ) {
        const CornerPermReprCubes &ccpReprCubes = cubesByDepth[depth].getAt(ccpReprIdx);
        const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(crepr.cco);
		if( ccoReprCubes.containsCubeEdges(crepr.ce) )
			break;
		++depth;
        if( depth >= cubesByDepth.availCount() ) {
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
            ccpReprIdx = cubecornerPermRepresentativeIdx(cc1repr.ccp);
            const CornerPermReprCubes &ccpReprCubes = cubesByDepth[depth].getAt(ccpReprIdx);
            const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(cc1repr.cco);
            if( ccoReprCubes.containsCubeEdges(cc1repr.ce) )
                break;
			cc1 = cube::compose(ccRev, crotated[cm]);
            cc1repr = cubeRepresentative(cc1);
            ccpReprIdx = cubecornerPermRepresentativeIdx(cc1repr.ccp);
            const CornerPermReprCubes &ccpReprCubesRev = cubesByDepth[depth].getAt(ccpReprIdx);
            const CornerOrientReprCubes &ccoReprCubesRev = ccpReprCubesRev.cornerOrientCubesAt(cc1repr.cco);
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
    static bool m_isStopRequested;
protected:
    static void mutexLock();
    static bool isStopRequestedNoLock() { return m_isStopRequested; }
    static void mutexUnlock();
public:
    static void requestStop();
    static void requestRestart();
    static bool isStopRequested();
};

std::mutex ProgressBase::m_progressMutex;
bool ProgressBase::m_isStopRequested;

void ProgressBase::mutexLock() {
    m_progressMutex.lock();
}

void ProgressBase::mutexUnlock() {
    m_progressMutex.unlock();
}

void ProgressBase::requestStop()
{
    mutexLock();
    m_isStopRequested = true;
    mutexUnlock();
}

void ProgressBase::requestRestart()
{
    mutexLock();
    m_isStopRequested = false;
    mutexUnlock();
}

bool ProgressBase::isStopRequested()
{
    mutexLock();
    bool res = m_isStopRequested;
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
    AddCubesProgress(unsigned itemCount, unsigned depth)
        : m_cubeCount(0), m_depth(depth), m_processCount(itemCount*THREAD_COUNT),
          m_processedCount(0), m_runningThreadCount(THREAD_COUNT), m_isFinish(false)
    {
    }

    bool inc(Responder &responder, unsigned long cubeCount);
    void threadFinished(Responder &responder);
};

bool AddCubesProgress::inc(Responder &responder, unsigned long cubeCount)
{
    unsigned long cubeCountTot;
    unsigned processedCount;
    bool isFinish;
    bool isStopRequested;
    mutexLock();
    cubeCountTot = m_cubeCount += cubeCount;
    processedCount = ++m_processedCount;
    isFinish = m_isFinish;
    isStopRequested = isStopRequestedNoLock();
    mutexUnlock();
    if( m_depth >= 9 && !isFinish ) {
        unsigned procCountNext = 100 * (processedCount+1) / m_processCount;
        unsigned procCountCur = 100 * processedCount / m_processCount;
        if( procCountNext != procCountCur && (m_depth >= 10 || procCountCur % 10 == 0) )
            responder.progress("depth %u cubes %llu, %u%%",
                    m_depth, cubeCountTot, 100 * processedCount / m_processCount);
    }
    return isStopRequested;
}

void AddCubesProgress::threadFinished(Responder &responder)
{
    unsigned runningThreadCount;
    mutexLock();
    runningThreadCount = --m_runningThreadCount;
    m_isFinish = true;
    mutexUnlock();
    if( m_depth >= 9 ) {
        responder.progress("depth %d cubes %d threads still running",
                m_depth, runningThreadCount);
    }
}

static void addCubesT(unsigned threadNo,
        CubesReprByDepth *cubesReprByDepth, int depth, bool onlyInSpace, 
        Responder *responder, AddCubesProgress *addCubesProgress)
{
    std::vector<EdgeReprCandidateTransform> otransformNew;

    const CubesReprAtDepth &ccpReprCubesC = (*cubesReprByDepth)[depth-1];
    for(CubesReprAtDepth::ccpcubes_iter ccpCubesIt = ccpReprCubesC.ccpCubesBegin();
            ccpCubesIt != ccpReprCubesC.ccpCubesEnd(); ++ccpCubesIt)
    {
        const CornerPermReprCubes &cpermReprCubesC = ccpCubesIt->second;
        cubecorners_perm ccp = ccpCubesIt->first;
        if( !cpermReprCubesC.empty() ) {
            unsigned long cubeCount = 0;
            for(int rd = 0; rd < RCOUNT; ++rd) {
                for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
                    cubecorners_perm ccpNew = reversed ?
                        cubecorners_perm::compose(crotated[rd].ccp, ccp) :
                        cubecorners_perm::compose(ccp, crotated[rd].ccp);
                    unsigned cornerPermReprIdxNew = cubecornerPermRepresentativeIdx(ccpNew);
                    if( cornerPermReprIdxNew % THREAD_COUNT == threadNo ) {
                        const CornerPermReprCubes *ccpReprCubesNewP = depth == 1 ? NULL : &(*cubesReprByDepth)[depth-2].getAt(cornerPermReprIdxNew);
                        const CornerPermReprCubes &ccpReprCubesNewC = (*cubesReprByDepth)[depth-1].getAt(cornerPermReprIdxNew);
                        for(CornerPermReprCubes::ccocubes_iter ccoCubesItC = cpermReprCubesC.ccoCubesBegin();
                                ccoCubesItC != cpermReprCubesC.ccoCubesEnd(); ++ccoCubesItC)
                        {
                            const CornerOrientReprCubes &corientReprCubesC = *ccoCubesItC;
                            cubecorner_orients cco = corientReprCubesC.getOrients();
                            bool isBGorient, isYWorient, isORorient;
                            if( onlyInSpace ) {
                                isBGorient = cco.isBGspace();
                                isYWorient = cco.isYWspace(ccp);
                                isORorient = cco.isORspace(ccp);
                                if( !isBGorient && !isYWorient && !isRotateKind(SPACEOR, rd) )
                                    continue;
                                if( !isBGorient && !isORorient && !isRotateKind(SPACEYW, rd) )
                                    continue;
                                if( !isYWorient && !isORorient && !isRotateKind(SPACEBG, rd) )
                                    continue;
                            }else
                                isBGorient = isYWorient = isORorient = false;
                            cubecorner_orients ccoNew = reversed ?
                                cubecorner_orients::compose(crotated[rd].cco, ccp, cco) :
                                cubecorner_orients::compose(cco, crotated[rd].ccp, crotated[rd].cco);
                            cubecorner_orients ccoReprNew = cubecornerOrientsRepresentative(ccpNew, ccoNew, otransformNew);
                            const CornerOrientReprCubes *corientReprCubesNewP = ccpReprCubesNewP == NULL ? NULL :
                                &ccpReprCubesNewP->cornerOrientCubesAt(ccoReprNew);
                            const CornerOrientReprCubes &corientReprCubesNewC = ccpReprCubesNewC.cornerOrientCubesAt(ccoReprNew);
                            std::vector<cubeedges> ceNewArr;
                            for(CornerOrientReprCubes::edges_iter edgeIt = corientReprCubesC.edgeBegin();
                                    edgeIt != corientReprCubesC.edgeEnd(); ++edgeIt)
                            {
                                const cubeedges ce = *edgeIt;

                                if( onlyInSpace ) {
                                    bool isBGcube = isBGorient && ce.isBGspace();
                                    bool isYWcube = isYWorient && ce.isYWspace();
                                    bool isORcube = isORorient && ce.isORspace();
                                    if( !isBGcube && !isYWcube && !isRotateKind(SPACEOR, rd) )
                                        continue;
                                    if( !isBGcube && !isORcube && !isRotateKind(SPACEYW, rd) )
                                        continue;
                                    if( !isYWcube && !isORcube && !isRotateKind(SPACEBG, rd) )
                                        continue;
                                }

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
                                CornerPermReprCubes &ccpReprCubesNewN = (*cubesReprByDepth)[depth].add(cornerPermReprIdxNew);
                                CornerOrientReprCubes &corientReprCubesNewN = ccpReprCubesNewN.cornerOrientCubesAdd(ccoReprNew);
                                for(cubeedges cenewRepr : ceNewArr) {
                                    if( corientReprCubesNewN.addCube(cenewRepr) )
                                        ++cubeCount;
                                }
                            }
                        }
                    }
                }
            }
            if( addCubesProgress->inc(*responder, cubeCount) )
                break;
        }
    }
    addCubesProgress->threadFinished(*responder);
}

static void initOccurT(unsigned threadNo, CubesReprAtDepth *cubesReprAtDepth)
{
    for(unsigned i = threadNo; i < cubesReprAtDepth->size(); i += THREAD_COUNT)
        cubesReprAtDepth->initOccur(i);
}

static bool addCubes(CubesReprByDepth &cubesReprByDepth, unsigned requestedDepth,
        bool onlyInSpace, Responder &responder)
{
    if( requestedDepth >= cubesReprByDepth.availMaxCount() ) {
        std::cout << "fatal: addCubes" << (onlyInSpace?" in-space":"") <<
            " requested depth=" << requestedDepth << ", max=" <<
            cubesReprByDepth.availMaxCount()-1 << std::endl;
        exit(1);
    }
    if( cubesReprByDepth.availCount() == 0 ) {
        // insert the solved cube
        unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(csolved.ccp);
        CornerPermReprCubes &ccpCubes = cubesReprByDepth[0].add(cornerPermReprIdx);
        std::vector<EdgeReprCandidateTransform> otransform;
        cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(csolved.ccp,
                csolved.cco, otransform);
        CornerOrientReprCubes &ccoCubes = ccpCubes.cornerOrientCubesAdd(ccoRepr);
        cubeedges ceRepr = cubeedgesRepresentative(csolved.ce, otransform);
        ccoCubes.addCube(ceRepr);
        cubesReprByDepth.incAvailCount();
    }
    while( cubesReprByDepth.availCount() <= requestedDepth ) {
        unsigned depth = cubesReprByDepth.availCount();
        const CubesReprAtDepth &cubesArr = cubesReprByDepth[depth-1];
        AddCubesProgress addCubesProgress(
                std::distance(cubesArr.ccpCubesBegin(), cubesArr.ccpCubesEnd()),
                depth);
        runInThreadPool(addCubesT, &cubesReprByDepth,
                    depth, onlyInSpace, &responder, &addCubesProgress);
        if( ProgressBase::isStopRequested() ) {
            responder.message("canceled");
            return true;
        }
        if( !onlyInSpace && depth >= 8 ) {
            responder.progress("depth %d init occur", depth);
            runInThreadPool(initOccurT, &cubesReprByDepth[depth]);
        }
        responder.message("depth %d %scubes=%lu", depth, onlyInSpace? "in-space " : "",
                    cubesReprByDepth[depth].cubeCount());
        cubesReprByDepth.incAvailCount();
    }
    return false;
}

static void addBGSpaceReprCubesT(unsigned threadNo,
        const CubesReprByDepth *cubesReprByDepth, SpaceReprCubes *bgSpaceCubes,
        unsigned depth, std::atomic_ulong *reprCubeCount)
{
    unsigned reprCubeCountT = 0;

    const CubesReprAtDepth &ccReprCubesC = (*cubesReprByDepth)[depth];
    for(CubesReprAtDepth::ccpcubes_iter ccpCubesIt = ccReprCubesC.ccpCubesBegin();
            ccpCubesIt != ccReprCubesC.ccpCubesEnd(); ++ccpCubesIt)
    {
        const CornerPermReprCubes &ccpCubes = ccpCubesIt->second;
        cubecorners_perm ccp = ccpCubesIt->first;
        for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpCubes.ccoCubesBegin();
                ccoCubesIt != ccpCubes.ccoCubesEnd(); ++ccoCubesIt)
        {
            const CornerOrientReprCubes &ccoCubes = *ccoCubesIt;
            cubecorner_orients cco = ccoCubes.getOrients();

            for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
                cubecorners_perm ccprev = reversed ? ccp.reverse() : ccp;
                cubecorner_orients ccorev = reversed ? cco.reverse(ccp) : cco;
                for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
                    cubecorners_perm ccprevsymm = symmetric ? ccprev.symmetric() : ccprev;
                    cubecorner_orients ccorevsymm = symmetric ? ccorev.symmetric() : ccorev;
                    for(unsigned td = 0; td < TCOUNT; ++td) {
                        cubecorners_perm ccpT = ccprevsymm.transform(td);
                        cubecorner_orients ccoT = ccorevsymm.transform(ccprevsymm, td);
                        cubecorner_orients ccoReprBG = ccoT.representativeBG(ccpT);
                        unsigned reprCOrientIdx = ccoReprBG.getOrientIdx();

                        if( reprCOrientIdx % THREAD_COUNT == threadNo ) {
                            for(CornerOrientReprCubes::edges_iter edgeIt = ccoCubes.edgeBegin();
                                    edgeIt != ccoCubes.edgeEnd(); ++edgeIt)
                            {
                                cubeedges ce = *edgeIt;
                                cubeedges cerev = reversed ? ce.reverse() : ce;
                                cubeedges cerevsymm = symmetric ? cerev.symmetric() : cerev;
                                cubeedges ceT = cerevsymm.transform(td);
                                cubeedges ceReprBG = ceT.representativeBG();
                                cube c = { .ccp = ccpT, .cco = ccoT, .ce = ceT };
                                if( (*bgSpaceCubes)[depth].addCube(reprCOrientIdx, ceReprBG, c) )
                                    ++reprCubeCountT;
                            }
                        }
                    }
                }
            }
        }
    }
    *reprCubeCount += reprCubeCountT;
}

static void addBGSpaceReprCubes(const CubesReprByDepth &cubesReprByDepth,
        SpaceReprCubes &bgSpaceCubes, unsigned requestedDepth, Responder &responder)
{
    while( bgSpaceCubes.availCount() <= requestedDepth ) {
        unsigned depth = bgSpaceCubes.availCount();
        std::atomic_ulong reprCubeCount(0);
        runInThreadPool(addBGSpaceReprCubesT, &cubesReprByDepth, &bgSpaceCubes, depth,
                    &reprCubeCount);
        responder.message("depth %d space repr cubes=%lu", depth, reprCubeCount.load());
        bgSpaceCubes.incAvailCount();
    }
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
            CubesReprAtDepth::ccpcubes_iter ccpItEnd,
            unsigned depth)
        : m_ccpItBeg(ccpItBeg), m_ccpItEnd(ccpItEnd), m_ccpItNext(ccpItBeg),
          m_depth(depth), m_runningThreadCount(THREAD_COUNT), m_isFinish(false)
    {
    }

    bool inc(Responder &responder, CubesReprAtDepth::ccpcubes_iter *ccpItBuf);
    bool isFinish() const { return m_isFinish; }
    std::string progressStr();
};

bool SearchProgress::inc(Responder &responder, CubesReprAtDepth::ccpcubes_iter *ccpItBuf)
{
    bool res;
    CubesReprAtDepth::ccpcubes_iter cornerPermIt;
    mutexLock();
    if( ccpItBuf == NULL )
        m_isFinish = true;
    res = !m_isFinish && m_ccpItNext != m_ccpItEnd && !isStopRequestedNoLock();
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
                responder.progress("depth %d search %d of %d, %d%%",
                        m_depth, itemIdx, itemCount, 100 * itemIdx / itemCount);
        }
    }else{
        if( m_depth >= 17 ) {
            responder.progress("depth %d search %d threads still running",
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

    unsigned ccpReprSearchIdx = cubecornerPermRepresentativeIdx(c.ccp);
    const CornerPermReprCubes &ccpReprSearchCubes = cubesRepr.getAt(ccpReprSearchIdx);
    cubecorner_orients ccoSearchRepr = cubecornerOrientsRepresentative(c.ccp,
            c.cco, otransform);
    const CornerOrientReprCubes &ccoReprSearchCubes =
        ccpReprSearchCubes.cornerOrientCubesAt(ccoSearchRepr);
    cubeedges ceSearchRepr = cubeedgesRepresentative(c.ce, otransform);
    return ccoReprSearchCubes.containsCubeEdges(ceSearchRepr);
}

static std::string getMovesForMatch(const CubesReprByDepth &cubesReprByDepth,
        const cube &cSearch, const cube &c, unsigned searchRev, unsigned searchTd,
        unsigned reversed, unsigned symmetric, unsigned td)
{
    // cube found: cSearch = c ⊙  (csearch rev/symm/tr)
    // rewriting equation:
    //  cSearch ⊙  (cSearch rev) = csolved = c ⊙  (csearch rev/symm/tr) ⊙  (cSearch rev)
    //  (c rev) = (c rev) ⊙  c ⊙  (csearch rev/symm/tr) ⊙  (cSearch rev)
    //  (c rev) = (csearch rev/symm/tr) ⊙  (cSearch rev)
    //  (c rev) ⊙  cSearch = (csearch rev/symm/tr) ⊙  (cSearch rev) ⊙  cSearch
    //  (c rev) ⊙  cSearch = (csearch rev/symm/tr)
    // moves solving (csearch rev/symm/tr): (cSearch rev) ⊙  c
    cube cSearch1 = cSearch.transform(transformReverse(td));
    if( symmetric )
        cSearch1 = cSearch1.symmetric();
    cube c1 = c.transform(transformReverse(td));
    if( symmetric )
        c1 = c1.symmetric();
    if( searchTd ) {
        cSearch1 = cSearch1.transform(transformReverse(searchTd));
        c1 = c1.transform(transformReverse(searchTd));
    }
    std::string moves;
    if( searchRev ) {
        moves = printMoves(cubesReprByDepth, c1, true);
        moves += printMoves(cubesReprByDepth, cSearch1);
    }else{
        moves = printMoves(cubesReprByDepth, cSearch1, true);
        moves += printMoves(cubesReprByDepth, c1);
    }
    return moves;
}

static bool searchMovesForCcp(const CubesReprByDepth &cubesReprByDepth,
        unsigned depthMax, const cube &csearch, cubecorners_perm ccp,
        const CornerPermReprCubes &ccpReprCubes, std::string &moves,
        unsigned searchRev, unsigned searchTd)
{
    std::vector<EdgeReprCandidateTransform> otransform;
    cube csearchSymm = csearch.symmetric();

    if( ccpReprCubes.empty() )
        return false;
    for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
        cubecorners_perm ccprev = reversed ? ccp.reverse() : ccp;
        for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
            for(unsigned td = 0; td < TCOUNT; ++td) {
                const cube cSearchT = (symmetric ? csearchSymm : csearch).transform(td);
                cubecorners_perm ccpSearch = cubecorners_perm::compose(ccprev, cSearchT.ccp);
                unsigned ccpReprSearchIdx = cubecornerPermRepresentativeIdx(ccpSearch);
                const CornerPermReprCubes &ccpReprSearchCubes =
                    cubesReprByDepth[depthMax].getAt(ccpReprSearchIdx);
                if( ccpReprCubes.size() <= ccpReprSearchCubes.size() || !isSingleTransform(ccpSearch) ) {
                    for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpReprCubes.ccoCubesBegin();
                            ccoCubesIt != ccpReprCubes.ccoCubesEnd(); ++ccoCubesIt)
                    {
                        const CornerOrientReprCubes &ccoReprCubes = *ccoCubesIt;
                        cubecorner_orients cco = ccoReprCubes.getOrients();
                        cubecorner_orients ccorev = reversed ? cco.reverse(ccp) : cco;
                        cubecorner_orients ccoSearch = cubecorner_orients::compose(ccorev, cSearchT.ccp, cSearchT.cco);
                        cubecorner_orients ccoSearchRepr = cubecornerOrientsComposedRepresentative(
                                ccpSearch, ccoSearch, cSearchT.ce, otransform);
                        const CornerOrientReprCubes &ccoReprSearchCubes =
                            ccpReprSearchCubes.cornerOrientCubesAt(ccoSearchRepr);
                        if( ccoReprSearchCubes.empty() )
                            continue;
                        cubeedges cerev = CornerOrientReprCubes::findSolutionEdge(
                                ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
                        if( !cerev.isNil() ) {
                            cubeedges ceSearch = cubeedges::compose(cerev, cSearchT.ce);
                            cube cSearch = { .ccp = ccpSearch, .cco = ccoSearch, .ce = ceSearch };
                            cube c = { .ccp = ccprev, .cco = ccorev, .ce = cerev };
                            moves = getMovesForMatch(cubesReprByDepth, cSearch, c, searchRev,
                                    searchTd, reversed, symmetric, td);
                            return true;
                        }
                    }
                }else{
                    for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpReprSearchCubes.ccoCubesBegin();
                            ccoCubesIt != ccpReprSearchCubes.ccoCubesEnd(); ++ccoCubesIt)
                    {
                        const CornerOrientReprCubes &ccoReprSearchCubes = *ccoCubesIt;
                        cubecorner_orients ccoSearchRepr = ccoReprSearchCubes.getOrients();
                        cubecorner_orients ccorev = cubecornerOrientsForComposedRepresentative(
                                ccpSearch, ccoSearchRepr, cSearchT, ccprev, otransform);
                        cubecorner_orients cco = reversed ? ccorev.reverse(ccprev) : ccorev;
                        const CornerOrientReprCubes &ccoReprCubes =
                            ccpReprCubes.cornerOrientCubesAt(cco);
                        if( ccoReprCubes.empty() )
                            continue;
                        cubeedges cerev = CornerOrientReprCubes::findSolutionEdge(
                                ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
                        if( !cerev.isNil() ) {
                            cubecorner_orients ccoSearch = cubecorner_orients::compose(ccorev,
                                    cSearchT.ccp, cSearchT.cco);
                            cubeedges ceSearch = cubeedges::compose(cerev, cSearchT.ce);
                            cube cSearch = { .ccp = ccpSearch, .cco = ccoSearch, .ce = ceSearch };
                            cube c = { .ccp = ccprev, .cco = ccorev, .ce = cerev };
                            moves = getMovesForMatch(cubesReprByDepth, cSearch, c, searchRev,
                                    searchTd, reversed, symmetric, td);
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

static void searchMovesTa(unsigned threadNo,
        const CubesReprByDepth *cubesReprByDepth,
        unsigned depth, unsigned depthMax, const cube *csearch, Responder *responder,
        SearchProgress *searchProgress)
{
    CubesReprAtDepth::ccpcubes_iter ccpIt;

	while( searchProgress->inc(*responder, &ccpIt) ) {
        const CornerPermReprCubes &ccpReprCubes = ccpIt->second;
        cubecorners_perm ccp = ccpIt->first;
        std::string moves;
        if( searchMovesForCcp(*cubesReprByDepth, depthMax, *csearch, ccp, ccpReprCubes, moves, 0, 0) ) {
            responder->solution(moves.c_str());
            searchProgress->inc(*responder, NULL);
            return;
        }
    }
}

static void searchMovesTb(unsigned threadNo,
        const CubesReprByDepth *cubesReprByDepth,
		int depth, const cube *csearch, Responder *responder, SearchProgress *searchProgress)
{
    CubesReprAtDepth::ccpcubes_iter cornerPerm2It;
    std::vector<EdgeReprCandidateTransform> o2transform;
    const CubesReprAtDepth &ccReprCubesC = (*cubesReprByDepth)[depth];
	while( searchProgress->inc(*responder, &cornerPerm2It) ) {
        const CornerPermReprCubes &ccpReprCubes2 = cornerPerm2It->second;
        cubecorners_perm ccp2 = cornerPerm2It->first;
        if( ccpReprCubes2.empty() )
            continue;
        for(CubesReprAtDepth::ccpcubes_iter ccpCubes1It = ccReprCubesC.ccpCubesBegin();
                ccpCubes1It != ccReprCubesC.ccpCubesEnd(); ++ccpCubes1It)
        {
            const CornerPermReprCubes &ccpCubes1 = ccpCubes1It->second;
            cubecorners_perm ccp1 = ccpCubes1It->first;
            if( ccpCubes1.empty() )
                continue;
            for(CornerPermReprCubes::ccocubes_iter ccoCubes1It = ccpCubes1.ccoCubesBegin();
                    ccoCubes1It != ccpCubes1.ccoCubesEnd(); ++ccoCubes1It)
            {
                const CornerOrientReprCubes &ccoCubes1 = *ccoCubes1It;
                cubecorner_orients cco1 = ccoCubes1.getOrients();
                for(CornerOrientReprCubes::edges_iter edge1It = ccoCubes1.edgeBegin();
                        edge1It != ccoCubes1.edgeEnd(); ++edge1It)
                {
                    const cubeedges ce1 = *edge1It;
                    cube c1 = { .ccp = ccp1, .cco = cco1, .ce = ce1 };
                    std::vector<cube> cubesChecked;
                    for(unsigned reversed1 = 0; reversed1 < (USEREVERSE ? 2 : 1); ++reversed1) {
                        cube c1r = reversed1 ? c1.reverse() : c1;
                        for(unsigned symmetric1 = 0; symmetric1 < 2; ++symmetric1) {
                            cube c1rs = symmetric1 ? c1r.symmetric() : c1r;
                            for(unsigned td1 = 0; td1 < TCOUNT; ++td1) {
                                const cube c1T = c1rs.transform(td1);
                                bool isDup = std::find(cubesChecked.begin(), cubesChecked.end(), c1T) != cubesChecked.end();
                                if( isDup )
                                    continue;
                                cubesChecked.push_back(c1T);
                                cube cSearch1 = cube::compose(c1T, *csearch);
                                std::string moves2;
                                if( searchMovesForCcp(*cubesReprByDepth,
                                        DEPTH_MAX, cSearch1, ccp2, ccpReprCubes2, moves2, 0, 0) )
                                {
                                    std::string moves = moves2;
                                    moves += printMoves(*cubesReprByDepth, c1T);
                                    responder->solution(moves.c_str());
                                    searchProgress->inc(*responder, NULL);
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

static const CubesReprByDepth *getCubesInSpace(unsigned depth, Responder &responder)
{
    static CubesReprByDepth cubesReprByDepthInSpace(TWOPHASE_DEPTH2_MAX+1);
    static std::mutex mtx;
    bool isCanceled;

    mtx.lock();
    isCanceled =  addCubes(cubesReprByDepthInSpace, depth, true, responder);
    mtx.unlock();
    return isCanceled ? NULL : &cubesReprByDepthInSpace;
}

static bool searchMovesInSpaceA(const CubesReprByDepth &cubesReprByDepthBG,
        const cube &cSpace, unsigned searchRev, unsigned searchTd, unsigned depth, unsigned depthMax,
        std::string &moves)
{
    const CubesReprAtDepth &ccReprCubesC = cubesReprByDepthBG[depth];
    for(CubesReprAtDepth::ccpcubes_iter ccpCubesIt = ccReprCubesC.ccpCubesBegin();
            ccpCubesIt != ccReprCubesC.ccpCubesEnd(); ++ccpCubesIt)
    {
        const CornerPermReprCubes &ccpCubes = ccpCubesIt->second;
        cubecorners_perm ccp = ccpCubesIt->first;
        if( searchMovesForCcp(cubesReprByDepthBG, depthMax, cSpace, ccp, ccpCubes, moves, searchRev, searchTd) )
            return true;
    }
    return false;
}

static bool searchMovesInSpaceB(const CubesReprByDepth &cubesReprByDepthBG,
		const cube &cSpace, unsigned searchRev, unsigned searchTd, unsigned depth, unsigned depthMax,
        std::string &moves)
{
    std::vector<EdgeReprCandidateTransform> o2transform;
    const CubesReprAtDepth &ccReprCubesC = cubesReprByDepthBG[depth];
    for(CubesReprAtDepth::ccpcubes_iter ccpCubes1It = ccReprCubesC.ccpCubesBegin();
            ccpCubes1It != ccReprCubesC.ccpCubesEnd(); ++ccpCubes1It)
    {
        const CornerPermReprCubes &ccpCubes1 = ccpCubes1It->second;
        cubecorners_perm ccp1 = ccpCubes1It->first;
        if( ccpCubes1.empty() )
            continue;
        for(CornerPermReprCubes::ccocubes_iter ccoCubes1It = ccpCubes1.ccoCubesBegin();
                ccoCubes1It != ccpCubes1.ccoCubesEnd(); ++ccoCubes1It)
        {
            const CornerOrientReprCubes &ccoCubes1 = *ccoCubes1It;
            cubecorner_orients cco1 = ccoCubes1.getOrients();
            for(CornerOrientReprCubes::edges_iter edge1It = ccoCubes1.edgeBegin();
                    edge1It != ccoCubes1.edgeEnd(); ++edge1It)
            {
                const cubeedges ce1 = *edge1It;
                cube c1 = { .ccp = ccp1, .cco = cco1, .ce = ce1 };
                std::vector<cube> cubesChecked;
                for(unsigned reversed1 = 0; reversed1 < (USEREVERSE ? 2 : 1); ++reversed1) {
                    cube c1r = reversed1 ? c1.reverse() : c1;
                    for(unsigned symmetric1 = 0; symmetric1 < 2; ++symmetric1) {
                        cube c1rs = symmetric1 ? c1r.symmetric() : c1r;
                        for(unsigned td1 = 0; td1 < TCOUNT; ++td1) {
                            const cube c1T = c1rs.transform(td1);
                            bool isDup = std::find(cubesChecked.begin(), cubesChecked.end(), c1T) != cubesChecked.end();
                            if( isDup )
                                continue;
                            cubesChecked.push_back(c1T);
                            cube cSearch1 = cube::compose(c1T, cSpace);
                            std::string moves2;
                            if( searchMovesInSpaceA(cubesReprByDepthBG, cSearch1, searchRev,
                                        searchTd, depthMax, depthMax, moves2) )
                            {
                                cube c1Trev = c1T.transform(transformReverse(searchTd));
                                if( searchRev ) {
                                    moves = printMoves(cubesReprByDepthBG, c1Trev, true) + moves2;
                                }else{
                                    moves = moves2 + printMoves(cubesReprByDepthBG, c1Trev);
                                }
                                return true;
                            }
                        }
                    }
                }
            }
        }
	}
    return false;
}

static int searchMovesInSpace(const cube &cSpace, unsigned searchRev, unsigned searchTd,
        unsigned movesMax, Responder &responder, std::string &moves)
{
    const CubesReprByDepth *cubesReprByDepthBG = getCubesInSpace(0, responder);
    if( cubesReprByDepthBG == NULL )
        return -1;
    for(unsigned depthSearch = 0; depthSearch <= movesMax && depthSearch <= 3*TWOPHASE_DEPTH2_MAX;
            ++depthSearch)
    {
        if( depthSearch < cubesReprByDepthBG->availCount() ) {
            if( containsCube((*cubesReprByDepthBG)[depthSearch], cSpace) ) {
                cube cSpaceT = cSpace;
                if( searchTd )
                    cSpaceT = cSpace.transform(transformReverse(searchTd));
                moves = printMoves(*cubesReprByDepthBG, cSpaceT, !searchRev);
                return depthSearch;
            }
        }else if( depthSearch < 2*cubesReprByDepthBG->availCount()-1 && depthSearch <= movesMax )
        {
            unsigned depthMax = cubesReprByDepthBG->availCount() - 1;
            if( searchMovesInSpaceA(*cubesReprByDepthBG, cSpace, searchRev, searchTd,
                        depthSearch-depthMax, depthMax, moves) )
                return depthSearch;
        }else if( depthSearch <= 2*TWOPHASE_DEPTH2_MAX ) {
            unsigned depth = depthSearch / 2;
            unsigned depthMax = depthSearch - depth;
            cubesReprByDepthBG = getCubesInSpace(depthMax, responder);
            if( cubesReprByDepthBG == NULL )
                return -1;
            if( searchMovesInSpaceA(*cubesReprByDepthBG, cSpace, searchRev, searchTd, depth, depthMax, moves) )
                return depthSearch;
            ++depthSearch;
        }else{
            cubesReprByDepthBG = getCubesInSpace(TWOPHASE_DEPTH2_MAX, responder);
            if( cubesReprByDepthBG == NULL )
                return -1;
            unsigned depth = depthSearch - 2*TWOPHASE_DEPTH2_MAX;
            if( searchMovesInSpaceB(*cubesReprByDepthBG, cSpace, searchRev, searchTd,
                        depth, TWOPHASE_DEPTH2_MAX, moves) )
                return 2*TWOPHASE_DEPTH2_MAX + depth;
        }
    }
    return -1;
}

/* The cSearchMid is an intermediate cube, cube1 ⊙  csearch.
 * Assumes the cSearchMid was found in SpaceReprCubes at depthMax.
 * Searches for a cube from the same BG space among cubesReprByDepth[depthMax].
 */
static int searchPhase1Cube2(const CubesReprByDepth &cubesReprByDepth,
        const cube &cSearchMid, const std::vector<cube> &cubes,
        unsigned searchRev, unsigned searchTd, unsigned cube2Depth, unsigned movesMax,
        bool catchFirst, Responder &responder, std::string &moves)
{
    int bestMoveCount = -1;

    for(const cube &cube2 : cubes) {
        // cSearchMid = cube1 ⊙  (csearch rev) = cube2 ⊙  cSpace
        // csearch rev = (cube1 rev) ⊙  cube2 ⊙  cSpace
        //
        // cSearchMid = cube1 ⊙  csearch = cube2 ⊙  cSpace
        // cSpace = (cube2 rev) ⊙  cube1 ⊙  csearch
        // csearch = (cube1 rev) ⊙  cube2 ⊙  cSpace
        // csearch rev = (cSpace rev) ⊙  (cube2 rev) ⊙  cube1
        cube cSpace = cube::compose(cube2.reverse(), cSearchMid);
        std::string movesInSpace;
        int depthInSpace = searchMovesInSpace(cSpace, searchRev, searchTd,
                movesMax-cube2Depth, responder, movesInSpace);
        if( depthInSpace >= 0 ) {
            //responder.message("found in-space cube at depth %d", depthInSpace);
            cube cube2T = cube2.transform(transformReverse(searchTd));
            std::string cube2Moves = printMoves(cubesReprByDepth, cube2T, !searchRev);
            if( searchRev )
                moves = cube2Moves + movesInSpace;
            else
                moves = movesInSpace + cube2Moves;
            bestMoveCount = cube2Depth + depthInSpace;
            if( catchFirst || depthInSpace == 0 )
                return bestMoveCount;
            movesMax = bestMoveCount-1;
        }
    }
    return bestMoveCount;
}

class QuickSearchProgress : public ProgressBase {
    const CubesReprAtDepth::ccpcubes_iter m_ccpItBeg;
    const CubesReprAtDepth::ccpcubes_iter m_ccpItEnd;
    CubesReprAtDepth::ccpcubes_iter m_ccpItNext;
    const unsigned m_depthSearch;
    const bool m_catchFirst;
    int m_movesMax;
    int m_bestMoveCount;
    std::string m_bestMoves;

public:
    QuickSearchProgress(CubesReprAtDepth::ccpcubes_iter ccpItBeg,
            CubesReprAtDepth::ccpcubes_iter ccpItEnd, unsigned depthSearch,
            bool catchFirst, int movesMax)
        : m_ccpItBeg(ccpItBeg), m_ccpItEnd(ccpItEnd), m_ccpItNext(ccpItBeg),
          m_depthSearch(depthSearch), m_catchFirst(catchFirst),
          m_movesMax(movesMax), m_bestMoveCount(-1)
    {
    }

    bool isCatchFirst() const { return m_catchFirst; }
    int inc(Responder &responder, CubesReprAtDepth::ccpcubes_iter *ccpItBuf);
    int setBestMoves(const std::string &moves, int moveCount, Responder &responder);
    int getBestMoves(std::string &moves) const;
};

int QuickSearchProgress::inc(Responder &responder, CubesReprAtDepth::ccpcubes_iter *ccpItBuf) {
    int movesMax;
    CubesReprAtDepth::ccpcubes_iter cornerPermIt;
    mutexLock();
    if( m_movesMax >= 0 && m_ccpItNext != m_ccpItEnd && !isStopRequestedNoLock() ) {
        cornerPermIt = m_ccpItNext++;
        movesMax = m_movesMax;
    }else
        movesMax = -1;
    mutexUnlock();
    if( movesMax >= 0 ) {
        *ccpItBuf = cornerPermIt;
        if( m_depthSearch >= 9 ) {
            unsigned itemCount = std::distance(m_ccpItBeg, m_ccpItEnd);
            unsigned itemIdx = std::distance(m_ccpItBeg, cornerPermIt);
            unsigned procCountNext = 100 * (itemIdx+1) / itemCount;
            unsigned procCountCur = 100 * itemIdx / itemCount;
            if( procCountNext != procCountCur && (m_depthSearch>=10 || procCountCur % 10 == 0) )
                responder.progress("depth %d search %d of %d, %d%%",
                        m_depthSearch, itemIdx, itemCount, 100 * itemIdx / itemCount);
        }
    }
    return movesMax;
}

int QuickSearchProgress::setBestMoves(const std::string &moves, int moveCount, Responder &responder) {
    int movesMax;
    mutexLock();
    if( m_bestMoveCount < 0 || moveCount < m_bestMoveCount ) {
        m_bestMoves = moves;
        m_bestMoveCount = moveCount;
        m_movesMax = m_catchFirst ? -1 : moveCount-1;
        responder.movecount("%u at depth: %u", moveCount, m_depthSearch);
    }
    movesMax = m_movesMax;
    mutexUnlock();
    return movesMax;
}

int QuickSearchProgress::getBestMoves(std::string &moves) const {
    moves = m_bestMoves;
    return m_bestMoveCount;
}

static bool searchMovesQuickForCcp(cubecorners_perm ccp, const CornerPermReprCubes &ccpReprCubes,
        const CubesReprByDepth &cubesReprByDepth, const SpaceReprCubes *bgSpaceCubes,
        const std::vector<std::pair<cube, std::string>> &csearchWithMovesAppend,
        unsigned depth, unsigned depth1Max, unsigned movesMax,
        Responder &responder, QuickSearchProgress *searchProgress)
{
    std::vector<cube> csearchTarr[2][3];

    for(auto [c, moves] : csearchWithMovesAppend) {
        csearchTarr[0][0].emplace_back(c);
        csearchTarr[0][1].emplace_back(c.transform(1));
        csearchTarr[0][2].emplace_back(c.transform(2));
        cube crev = c.reverse();
        csearchTarr[1][0].emplace_back(crev);
        csearchTarr[1][1].emplace_back(crev.transform(1));
        csearchTarr[1][2].emplace_back(crev.transform(2));
    }
    for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
        cubecorners_perm ccprev = reversed ? ccp.reverse() : ccp;
        for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
            cubecorners_perm ccprevsymm = symmetric ? ccprev.symmetric() : ccprev;
            for(unsigned td = 0; td < TCOUNT; ++td) {
                cubecorners_perm ccpT = ccprevsymm.transform(td);
                for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpReprCubes.ccoCubesBegin();
                        ccoCubesIt != ccpReprCubes.ccoCubesEnd(); ++ccoCubesIt)
                {
                    const CornerOrientReprCubes &ccoReprCubes = *ccoCubesIt;
                    cubecorner_orients cco = ccoReprCubes.getOrients();
                    cubecorner_orients ccorev = reversed ? cco.reverse(ccp) : cco;
                    cubecorner_orients ccorevsymm = symmetric ? ccorev.symmetric() : ccorev;
                    cubecorner_orients ccoT = ccorevsymm.transform(ccprevsymm, td);
                    std::vector<cubeedges> ceTarr;
                    for(unsigned srchItem = 0; srchItem < csearchTarr[0][0].size(); ++srchItem) {
                        for(unsigned searchRev = 0; searchRev < TWOPHASE_SEARCHREV; ++searchRev) {
                            for(unsigned searchTd = 0; searchTd < 3; ++searchTd) {
                                const cube &csearchT = csearchTarr[searchRev][searchTd][srchItem];
                                cubecorners_perm ccpSearch = cubecorners_perm::compose(ccpT, csearchT.ccp);
                                cubecorner_orients ccoSearch = cubecorner_orients::compose(ccoT,
                                        csearchT.ccp, csearchT.cco);
                                cubecorner_orients ccoSearchReprBG = ccoSearch.representativeBG(ccpSearch);
                                unsigned short searchReprCOrientIdx = ccoSearchReprBG.getOrientIdx();
                                if( (*bgSpaceCubes)[depth1Max].containsCCOrients(searchReprCOrientIdx) ) {
                                    if( ceTarr.empty() ) {
                                        for(CornerOrientReprCubes::edges_iter edgeIt = ccoReprCubes.edgeBegin();
                                                edgeIt != ccoReprCubes.edgeEnd(); ++edgeIt)
                                        {
                                            cubeedges ce = *edgeIt;
                                            cubeedges cerev = reversed ? ce.reverse() : ce;
                                            cubeedges cerevsymm = symmetric ? cerev.symmetric() : cerev;
                                            cubeedges ceT = cerevsymm.transform(td);
                                            ceTarr.push_back(ceT);
                                        }
                                    }
                                    for(const cubeedges &ceT : ceTarr) {
                                        cubeedges ceSearch = cubeedges::compose(ceT, csearchT.ce);
                                        cubeedges ceSearchSpaceRepr = ceSearch.representativeBG();
                                        const std::vector<cube> *cubesForCE =
                                            (*bgSpaceCubes)[depth1Max].getCubesForCE(
                                                    searchReprCOrientIdx, ceSearchSpaceRepr);
                                        if( cubesForCE ) {
                                            cube cSearch1 = { .ccp = ccpSearch, .cco = ccoSearch, .ce = ceSearch };
                                            std::string inspaceWithCube2Moves;
                                            int moveCount = searchPhase1Cube2(cubesReprByDepth,
                                                    cSearch1, *cubesForCE, searchRev, searchTd,
                                                    depth1Max, movesMax-depth, searchProgress->isCatchFirst(),
                                                    responder, inspaceWithCube2Moves);
                                            if( moveCount >= 0 ) {
                                                cube cube1 = { .ccp = ccpT, .cco = ccoT, .ce = ceT };
                                                cube cube1T = cube1.transform(transformReverse(searchTd));
                                                std::string cube1Moves = printMoves(cubesReprByDepth, cube1T, searchRev);
                                                const std::string &cubeMovesAppend =
                                                    csearchWithMovesAppend[srchItem].second;
                                                std::string moves;
                                                if( searchRev )
                                                    moves = cube1Moves + inspaceWithCube2Moves;
                                                else
                                                    moves = inspaceWithCube2Moves + cube1Moves;
                                                moves += cubeMovesAppend;
                                                movesMax = searchProgress->setBestMoves(moves,
                                                        depth+moveCount, responder);
                                                if( movesMax < 0 )
                                                    return true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if( searchProgress->isStopRequested() )
                        return true;
                }
            }
        }
    }
    return false;
}

static void searchMovesQuickTa(unsigned threadNo,
        const CubesReprByDepth *cubesReprByDepth, const SpaceReprCubes *bgSpaceCubes,
        const cube *csearch, unsigned depth, unsigned depth1Max,
        Responder *responder, QuickSearchProgress *searchProgress)
{
    //const char transformToSpace[][3] = { "BG", "OR", "YW" };
    CubesReprAtDepth::ccpcubes_iter ccpIt;
    int movesMax;

    while( (movesMax = searchProgress->inc(*responder, &ccpIt)) >= 0 ) {
        const CornerPermReprCubes &ccpReprCubes = ccpIt->second;
        cubecorners_perm ccp = ccpIt->first;
        if( !ccpReprCubes.empty() ) {
            std::vector<std::pair<cube, std::string>> cubesWithMoves;
            cubesWithMoves.emplace_back(std::make_pair(*csearch, std::string()));
            if( searchMovesQuickForCcp(ccp, ccpReprCubes, *cubesReprByDepth, bgSpaceCubes,
                    cubesWithMoves, depth, depth1Max, movesMax, *responder,
                    searchProgress) )
                return;
        }
    }
}

static void searchMovesQuickTb1(unsigned threadNo, const CubesReprByDepth *cubesReprByDepth,
        const SpaceReprCubes *bgSpaceCubes, const cube *csearch, unsigned depth1Max,
        Responder *responder, QuickSearchProgress *searchProgress)
{
    CubesReprAtDepth::ccpcubes_iter ccp2It;
    int movesMax;
    while( (movesMax = searchProgress->inc(*responder, &ccp2It)) >= 0 ) {
        const CornerPermReprCubes &ccp2ReprCubes = ccp2It->second;
        cubecorners_perm ccp2 = ccp2It->first;
        if( ccp2ReprCubes.empty() )
            continue;
        std::vector<std::pair<cube, std::string>> cubesWithMoves;
        for(unsigned rd = 0; rd < RCOUNT; ++rd) {
            cube c1Search = cube::compose(crotated[rd], *csearch);
            std::string cube1Moves = printMoves(*cubesReprByDepth, crotated[rd]);
            cubesWithMoves.emplace_back(std::make_pair(c1Search, cube1Moves));
        }
        if( searchMovesQuickForCcp(ccp2, ccp2ReprCubes, *cubesReprByDepth, bgSpaceCubes,
                    cubesWithMoves, depth1Max+1, depth1Max, movesMax,
                    *responder, searchProgress) )
            return;
    }
}

static void searchMovesQuickTb(unsigned threadNo, const CubesReprByDepth *cubesReprByDepth,
        const SpaceReprCubes *bgSpaceCubes, const cube *csearch, unsigned depth, unsigned depth1Max,
        Responder *responder, QuickSearchProgress *searchProgress)
{
    CubesReprAtDepth::ccpcubes_iter ccp2It;
    int movesMax;
    const CubesReprAtDepth &ccReprCubesC = (*cubesReprByDepth)[depth];

    while( (movesMax = searchProgress->inc(*responder, &ccp2It)) >= 0 ) {
        const CornerPermReprCubes &ccp2ReprCubes = ccp2It->second;
        cubecorners_perm ccp2 = ccp2It->first;
        if( ccp2ReprCubes.empty() )
            continue;
        for(CubesReprAtDepth::ccpcubes_iter ccp1It = ccReprCubesC.ccpCubesBegin();
                ccp1It != ccReprCubesC.ccpCubesEnd(); ++ccp1It)
        {
            const CornerPermReprCubes &ccp1ReprCubes = ccp1It->second;
            cubecorners_perm ccp1 = ccp1It->first;
            if( ccp1ReprCubes.empty() )
                continue;
            for(CornerPermReprCubes::ccocubes_iter cco1CubesIt = ccp1ReprCubes.ccoCubesBegin();
                    cco1CubesIt != ccp1ReprCubes.ccoCubesEnd(); ++cco1CubesIt)
            {
                const CornerOrientReprCubes &cco1ReprCubes = *cco1CubesIt;
                cubecorner_orients cco1 = cco1ReprCubes.getOrients();
                for(CornerOrientReprCubes::edges_iter edge1It = cco1ReprCubes.edgeBegin();
                        edge1It != cco1ReprCubes.edgeEnd(); ++edge1It)
                {
                    cubeedges ce1 = *edge1It;
                    std::vector<cube> cubesChecked;
                    std::vector<std::pair<cube, std::string>> cubesWithMoves;
                    for(unsigned reversed1 = 0; reversed1 < (USEREVERSE ? 2 : 1); ++reversed1) {
                        cubecorners_perm ccp1rev = reversed1 ? ccp1.reverse() : ccp1;
                        cubecorner_orients cco1rev = reversed1 ? cco1.reverse(ccp1) : cco1;
                        cubeedges ce1rev = reversed1 ? ce1.reverse() : ce1;
                        for(unsigned symmetric1 = 0; symmetric1 < 2; ++symmetric1) {
                            cubecorners_perm ccp1revsymm = symmetric1 ? ccp1rev.symmetric() : ccp1rev;
                            cubecorner_orients cco1revsymm = symmetric1 ? cco1rev.symmetric() : cco1rev;
                            cubeedges ce1revsymm = symmetric1 ? ce1rev.symmetric() : ce1rev;
                            for(unsigned td1 = 0; td1 < TCOUNT; ++td1) {
                                cubecorners_perm ccp1T = ccp1revsymm.transform(td1);
                                cubecorner_orients cco1T = cco1revsymm.transform(ccp1revsymm, td1);
                                cubeedges ce1T = ce1revsymm.transform(td1);

                                cube c1T = { .ccp = ccp1T, .cco = cco1T, .ce = ce1T };
                                bool isDup = std::find(cubesChecked.begin(), cubesChecked.end(), c1T) != cubesChecked.end();
                                if( isDup )
                                    continue;
                                cubesChecked.push_back(c1T);

                                cubecorners_perm ccp1Search = cubecorners_perm::compose(ccp1T, csearch->ccp);
                                cubecorner_orients cco1Search = cubecorner_orients::compose(cco1T,
                                        csearch->ccp, csearch->cco);
                                cubeedges ce1Search = cubeedges::compose(ce1T, csearch->ce);
                                cube c1Search = { .ccp = ccp1Search, .cco = cco1Search, .ce = ce1Search };
                                std::string cube1Moves = printMoves(*cubesReprByDepth, c1T);
                                cubesWithMoves.emplace_back(std::make_pair(c1Search, cube1Moves));
                            }
                        }
                    }
                    if( searchMovesQuickForCcp(ccp2, ccp2ReprCubes, *cubesReprByDepth, bgSpaceCubes,
                                cubesWithMoves, depth+depth1Max, depth1Max, movesMax,
                                *responder, searchProgress) )
                        return;
                }
            }
        }
    }
}

static const CubesReprByDepth *getReprCubes(unsigned depth, Responder &responder)
{
    static CubesReprByDepth cubesReprByDepth(DEPTH_MAX);
    bool isCanceled =  addCubes(cubesReprByDepth, depth, false, responder);
    return isCanceled ? NULL : &cubesReprByDepth;
}

static const SpaceReprCubes *getBGSpaceReprCubes(unsigned depth, Responder &responder)
{
    static SpaceReprCubes bgSpaceCubes(
            std::max(TWOPHASE_DEPTH1_CATCHFIRST_MAX, TWOPHASE_DEPTH1_MULTI_MAX)+1);

    if( bgSpaceCubes.availCount() <= depth ) {
        const CubesReprByDepth *cubesReprByDepth = getReprCubes(depth, responder);
        if( cubesReprByDepth == NULL )
            return NULL;
        addBGSpaceReprCubes(*cubesReprByDepth, bgSpaceCubes, depth, responder);
    }
    return &bgSpaceCubes;
}

static bool searchMovesQuickA(const cube &csearch, unsigned depthSearch, bool catchFirst,
        Responder &responder, unsigned movesMax, int &moveCount, std::string &moves)
{
    moveCount = -1;
    const CubesReprByDepth *cubesReprByDepth = getReprCubes(0, responder);
    const SpaceReprCubes *bgSpaceCubes = getBGSpaceReprCubes(0, responder);
    if( cubesReprByDepth == NULL || bgSpaceCubes == NULL )
        return true;
    unsigned depthsAvail = std::min(cubesReprByDepth->availCount(), bgSpaceCubes->availCount()) - 1;
    unsigned depth, depth1Max;
    if( depthSearch <= depthsAvail ) {
        depth = 0;
        depth1Max = depthSearch;
    }else if( depthSearch <= 2 * depthsAvail ) {
        depth = depthSearch - depthsAvail;
        depth1Max = depthsAvail;
    }else{
        depth = depthSearch / 2;
        depth1Max = depthSearch - depth;
        cubesReprByDepth = getReprCubes(depth1Max, responder);
        bgSpaceCubes = getBGSpaceReprCubes(depth1Max, responder);
        if( cubesReprByDepth == NULL || bgSpaceCubes == NULL )
            return true;
    }
    QuickSearchProgress searchProgress((*cubesReprByDepth)[depth].ccpCubesBegin(),
            (*cubesReprByDepth)[depth].ccpCubesEnd(), depthSearch, catchFirst, movesMax);
    runInThreadPool(searchMovesQuickTa, cubesReprByDepth, bgSpaceCubes,
            &csearch, depth, depth1Max, &responder, &searchProgress);
    moveCount = searchProgress.getBestMoves(moves);
    return searchProgress.isStopRequested();
}

static bool searchMovesQuickB(const cube &csearch, unsigned depth1Max, unsigned depthSearch, bool catchFirst,
        Responder &responder, unsigned movesMax, int &moveCount, std::string &moves)
{
    moveCount = -1;
    const CubesReprByDepth *cubesReprByDepth = getReprCubes(depth1Max, responder);
    const SpaceReprCubes *bgSpaceCubes = getBGSpaceReprCubes(depth1Max, responder);
    if( cubesReprByDepth == NULL || bgSpaceCubes == NULL )
        return true;
    unsigned depth = depthSearch - 2*depth1Max;
    QuickSearchProgress searchProgress((*cubesReprByDepth)[depth1Max].ccpCubesBegin(),
            (*cubesReprByDepth)[depth1Max].ccpCubesEnd(), depthSearch, catchFirst, movesMax);
    if( depth == 1 )
        runInThreadPool(searchMovesQuickTb1, cubesReprByDepth, bgSpaceCubes,
                    &csearch, depth1Max, &responder, &searchProgress);
    else
        runInThreadPool(searchMovesQuickTb, cubesReprByDepth, bgSpaceCubes,
                    &csearch, depth, depth1Max, &responder, &searchProgress);
    moveCount = searchProgress.getBestMoves(moves);
    return searchProgress.isStopRequested();
}

static bool searchMovesOptimalA(const cube &csearch, unsigned depthSearch, Responder &responder)
{
    const CubesReprByDepth *cubesReprByDepth = getReprCubes(0, responder);
    if( cubesReprByDepth == NULL )
        return true;
    unsigned depthsAvail = cubesReprByDepth->availCount() - 1;
    unsigned depth, depthMax;
    if( depthSearch <= depthsAvail ) {
        depth = 0;
        depthMax = depthSearch;
    }else if( depthSearch <= 2 * depthsAvail ) {
        depth = depthSearch - depthsAvail;
        depthMax = depthsAvail;
    }else{
        depth = depthSearch / 2;
        depthMax = depthSearch - depth;
        cubesReprByDepth = getReprCubes(depthMax, responder);
        if( cubesReprByDepth == NULL )
            return true;
    }
    SearchProgress searchProgress((*cubesReprByDepth)[depth].ccpCubesBegin(),
            (*cubesReprByDepth)[depth].ccpCubesEnd(), depthSearch);
    runInThreadPool(searchMovesTa, cubesReprByDepth,
                depth, depthMax, &csearch, &responder, &searchProgress);
    if( searchProgress.isFinish() ) {
        responder.movecount("%u", depthSearch);
        responder.message("finished at %s", searchProgress.progressStr().c_str());
        return true;
    }
    bool isStopRequested = ProgressBase::isStopRequested();
    if( isStopRequested )
        responder.message("canceled");
    else
        responder.message("depth %d end", depthSearch);
    return isStopRequested;
}

static bool searchMovesOptimalB(const cube &csearch, unsigned depth, Responder &responder)
{
    const CubesReprByDepth *cubesReprByDepth = getReprCubes(DEPTH_MAX, responder);
    if( cubesReprByDepth == NULL )
        return true;
    SearchProgress searchProgress(
            (*cubesReprByDepth)[DEPTH_MAX].ccpCubesBegin(),
            (*cubesReprByDepth)[DEPTH_MAX].ccpCubesEnd(), 2*DEPTH_MAX+depth);
    runInThreadPool(searchMovesTb, cubesReprByDepth, depth, &csearch,
                &responder, &searchProgress);
    if( searchProgress.isFinish() ) {
        responder.movecount("%u", 2*DEPTH_MAX+depth);
        responder.message("finished at %s", searchProgress.progressStr().c_str());
        return true;
    }
    bool isStopRequested = ProgressBase::isStopRequested();
    if( isStopRequested )
        responder.message("canceled");
    else
        responder.message("depth %d end", 2*DEPTH_MAX+depth);
    return isStopRequested;
}

static void searchMovesQuickCatchFirst(const cube &csearch, Responder &responder)
{
    unsigned depth1Max;
    {
        const CubesReprByDepth *cubesReprByDepth = getReprCubes(0, responder);
        const SpaceReprCubes *bgSpaceCubes = getBGSpaceReprCubes(0, responder);
        if( cubesReprByDepth == NULL || bgSpaceCubes == NULL )
            return;
        depth1Max = std::min(cubesReprByDepth->availCount(), bgSpaceCubes->availCount()) - 1;
        depth1Max = std::max(depth1Max, (unsigned)TWOPHASE_DEPTH1_CATCHFIRST_MAX);
    }
    for(unsigned depthSearch = 0;
            depthSearch <= 12 && depthSearch <= 3*TWOPHASE_DEPTH1_CATCHFIRST_MAX;
            ++depthSearch)
    {
        int moveCount;
        std::string bestMoves;
        bool isFinish;
        if( depthSearch <= 2 * depth1Max )
            isFinish = searchMovesQuickA(csearch, depthSearch, true, responder, 999, moveCount, bestMoves);
        else
            isFinish = searchMovesQuickB(csearch, TWOPHASE_DEPTH1_CATCHFIRST_MAX, depthSearch,
                    true, responder, 999, moveCount, bestMoves);
        if( moveCount >= 0 ) {
            responder.solution(bestMoves.c_str());
            return;
        }
        if( isFinish )
            return;
        responder.message("depth %u end", depthSearch);
    }
    responder.message("not found");
}

static void searchMovesQuickMulti(const cube &csearch, Responder &responder)
{
    std::string movesBest;
    unsigned bestMoveCount = 999;
    for(unsigned depthSearch = 0;
            depthSearch <= 12 && depthSearch <= 3*TWOPHASE_DEPTH1_MULTI_MAX && depthSearch < bestMoveCount;
            ++depthSearch)
    {
        int moveCount;
        std::string moves;
        bool isFinish;
        if( depthSearch <= 2 * TWOPHASE_DEPTH1_MULTI_MAX )
            isFinish = searchMovesQuickA(csearch, depthSearch, false, responder, bestMoveCount-1, moveCount, moves);
        else
            isFinish = searchMovesQuickB(csearch, TWOPHASE_DEPTH1_MULTI_MAX, depthSearch,
                    false, responder, bestMoveCount-1, moveCount, moves);
        if( moveCount >= 0 ) {
            movesBest = moves;
            bestMoveCount = moveCount;
            if( moveCount == 0 )
                break;
        }
        if( isFinish )
            break;
        responder.message("depth %u end", depthSearch);
    }
    if( !movesBest.empty() ) {
        responder.solution(movesBest.c_str());
        return;
    }
    responder.message("not found");
}

static void searchMovesOptimal(const cube &csearch, Responder &responder)
{
    for(unsigned depthSearch = 0; depthSearch <= 2*DEPTH_MAX; ++depthSearch) {
        if( searchMovesOptimalA(csearch, depthSearch, responder) )
            return;
    }
    for(unsigned depth = 1; depth <= DEPTH_MAX; ++depth) {
        if( searchMovesOptimalB(csearch, depth, responder) )
            return;
    }
    responder.message("not found");
}

static void searchMoves(const cube &csearch, char mode, Responder &responder)
{
    responder.message("%s", getSetup().c_str());
    switch( mode ) {
    case 'q':
        searchMovesQuickCatchFirst(csearch, responder);
        break;
    case 'm':
        searchMovesQuickMulti(csearch, responder);
        break;
    default:
        searchMovesOptimal(csearch, responder);
        break;
    }
}

class SocketChunkedResponder : public Responder {
    const int m_fdReq;

    void handleMessage(MessageType, const char*);
public:
    explicit SocketChunkedResponder(int fdReq)
        : m_fdReq(fdReq)
    {
    }
};

void SocketChunkedResponder::handleMessage(MessageType mt, const char *msg) {
    const char *prefix = "";
    switch(mt) {
    case MT_UNQUALIFIED:
        break;
    case MT_PROGRESS:
        prefix = "progress: ";
        break;
    case MT_MOVECOUNT:
        prefix = "moves: ";
        break;
    case MT_SOLUTION:
        prefix = "solution:";
        break;
    }
    char respBuf[2048];
    unsigned msgLen = strlen(prefix) + strlen(msg) + 1;
    sprintf(respBuf, "%x\r\n%s%s\n\r\n", msgLen, prefix, msg);
    write(m_fdReq, respBuf, strlen(respBuf));
}

static void processCubeReq(int fdReq, const char *reqParam)
{
    static std::mutex solverMutex;

    if( !strncmp(reqParam, "stop", 4) ) {
        ProgressBase::requestStop();
        char respHeader[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-length: 0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        write(fdReq, respHeader, strlen(respHeader));
        return;
    }
    if( solverMutex.try_lock() ) {
        ProgressBase::requestRestart();
        char respHeader[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-type: text/plain\r\n"
            "Transfer-encoding: chunked\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        write(fdReq, respHeader, strlen(respHeader));
        SocketChunkedResponder responder(fdReq);
        char mode = reqParam[0];
        if( reqParam[1] == '=' ) {
            cube c;
            if( cubeRead(responder, reqParam+2, c) ) {
                responder.message("thread count: %d", THREAD_COUNT);
                if( c == csolved )
                    responder.message("already solved");
                else
                    searchMoves(c, mode, responder);
            }
        }else{
            responder.message("invalid parameter");
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

static void runServer()
{
    int listenfd, acceptfd, opton = 1;
    struct sockaddr_in addr;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket");
        return;
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opton, sizeof(opton));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    if( bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0 ) {
        perror("bind");
        return;
    }
    if( listen(listenfd, 1) < 0 ) {
        perror("listen");
        return;
    }
    while( (acceptfd = accept(listenfd, NULL, NULL)) >= 0 ) {
        //setsockopt(acceptfd, IPPROTO_TCP, TCP_NODELAY, &opton, sizeof(opton));
        std::thread t = std::thread(processConnection, acceptfd);
        t.detach();
    }
    perror("accept");
    exit(1);
}

static cube generateCube()
{
    unsigned rndarr[3];

    FILE *fp = fopen("/dev/urandom", "r");
    fread(rndarr, sizeof(rndarr), 1, fp);
    fclose(fp);
    unsigned long rnd = (unsigned long)rndarr[1] << 32 | rndarr[0];
    cube c;
    c.ccp = cubecorners_perm::fromPermIdx(rnd % 40320);
    rnd /= 40320;
    c.cco = cubecorner_orients::fromOrientIdx(rnd % 2187);
    rnd /= 2187;
    unsigned ceOrient = rnd % 2048;
    rnd /= 2048;
    rnd = rnd << 32 | rndarr[2];
    c.ce = cubeedges::fromPermAndOrientIdx(rnd % 479001600, ceOrient);
    if( c.ccp.isPermParityOdd() != c.ce.isPermParityOdd() ) {
        unsigned p = c.ce.getPermAt(10);
        c.ce.setPermAt(10, c.ce.getPermAt(11));
        c.ce.setPermAt(11, p);
    }
    return c;
}

class ConsoleResponder : public Responder {
    const unsigned m_verboseLevel;
    std::string m_solution, m_movecount;
    std::chrono::time_point<std::chrono::system_clock> m_startTime;
    void handleMessage(MessageType, const char*);
public:
    explicit ConsoleResponder(unsigned verboseLevel);
    unsigned durationTimeMs() const;
    std::string durationTime() const;
    const char *getSolution() const { return m_solution.empty() ? NULL : m_solution.c_str(); }
    const char *getMoveCount() const { return m_movecount.c_str(); }
};

ConsoleResponder::ConsoleResponder(unsigned verboseLevel)
    : m_verboseLevel(verboseLevel), m_startTime(std::chrono::system_clock::now())
{
}

unsigned ConsoleResponder::durationTimeMs() const
{
    std::chrono::time_point<std::chrono::system_clock> curTime(std::chrono::system_clock::now());
    auto durns = curTime - m_startTime;
    std::chrono::milliseconds dur = std::chrono::duration_cast<std::chrono::milliseconds>(durns);
    return dur.count();
}

std::string ConsoleResponder::durationTime() const
{
    std::ostringstream os;
    unsigned ms = durationTimeMs();
    unsigned minutes = ms / 60000;
    ms %= 60000;
    unsigned seconds = ms / 1000;
    ms %= 1000;
    if( minutes )
        os << minutes << ":" << std::setw(2) << std::setfill('0');
    os << seconds << "." << std::setw(3) << std::setfill('0') << ms;
    return os.str();
}

void ConsoleResponder::handleMessage(MessageType mt, const char *msg) {
    const char *pad = "                                                 ";
    pad += std::min(strlen(msg), strlen(pad));
    switch(mt) {
    case MT_UNQUALIFIED:
        if( m_verboseLevel ) {
            std::cout << "\r" << durationTime() << " " << msg << pad << std::flush;
            if( !strncmp(msg, "finished at ", 12) )
                std::cout << std::endl;
        }
        break;
    case MT_PROGRESS:
        if( m_verboseLevel > 1 )
            std::cout << "\r" << durationTime() << " " << msg << pad << std::flush;
        break;
    case MT_MOVECOUNT:
        std::cout << "\r" << durationTime() << " moves: " << msg << pad << std::endl;
        m_movecount = msg;
        break;
    case MT_SOLUTION:
        std::cout << "\r" << durationTime() << msg << pad << std::endl;
        m_solution = msg;
        break;
    }
}

static void printStats()
{
    std::vector<EdgeReprCandidateTransform> otransform;
    unsigned maxSize = 0;

    for(unsigned depth = 0; depth <= DEPTH_MAX; ++depth) {
        ConsoleResponder responder(2);
        const CubesReprByDepth *cubesReprByDepth = getReprCubes(depth, responder);
        std::cout << "depth " << depth << std::endl;
        const CubesReprAtDepth &ccReprCubesC = (*cubesReprByDepth)[depth];
        for(CubesReprAtDepth::ccpcubes_iter ccpCubesIt = ccReprCubesC.ccpCubesBegin();
                ccpCubesIt != ccReprCubesC.ccpCubesEnd(); ++ccpCubesIt)
        {
            const CornerPermReprCubes &ccpCubes = ccpCubesIt->second;
            //cubecorners_perm ccp = ccpCubesIt->first;
            for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpCubes.ccoCubesBegin();
                    ccoCubesIt != ccpCubes.ccoCubesEnd(); ++ccoCubesIt)
            {
                const CornerOrientReprCubes &ccoCubes = *ccoCubesIt;
                if( ccoCubes.size() > maxSize ) {
                    maxSize = ccoCubes.size();
                    std::cout << "max size: " << maxSize << std::endl;
                }
                /*
                cubecorner_orients cco = ccoCubes.getOrients();
                for(CornerOrientReprCubes::edges_iter edgeIt = ccoCubes.edgeBegin();
                        edgeIt != ccoCubes.edgeEnd(); ++edgeIt)
                {
                    const cubeedges ce = *edgeIt;
                }
                */
            }
        }
    }
    exit(0);
}

static void solveCube(const char *cubeStr, char mode)
{
    ConsoleResponder responder(mode == 'q' ? 0 : mode == 'm' ? 1 : 2);
    cube c;
    if( !cubeRead(responder, cubeStr, c) )
        return;
    cubePrint(c);
    searchMoves(c, mode, responder);
    if( mode != 'q' )
        std::cout << std::endl;
    std::cout << std::endl;
    const char *s = responder.getSolution();
    unsigned moveCount = 0;
    while( s ) {
        if( *s == ' ' )
            ++s;
        int rd = rotateNameToDir(s);
        if( rd == RCOUNT ) {
            std::cout << "fatal: unrecognized move " << s << std::endl;
            return;
        }
        c = cube::compose(c, crotated[rd]);
        std::cout << rotateDirName(rd) << " " << c.toParamText() << std::endl;
        cubePrint(c);
        s = strchr(s, ' ');
        ++moveCount;
    }
    if( c != csolved ) {
        std::cout << "fatal: bad solution!" << std::endl;
        return;
    }
}

static void cubeTester(unsigned cubeCount, char mode)
{
    if( mode == 'O' ) {
        ConsoleResponder responder(2);
        getReprCubes(DEPTH_MAX, responder);
        std::cout << std::endl;
    }
    std::map<std::string, std::pair<unsigned, unsigned>> moveCounters;
    for(unsigned i = 0; i < cubeCount; ++i) {
        cube c = generateCube();
        std::cout << i << "  " << c.toParamText() << std::endl;
        std::cout << std::endl;
        cubePrint(c);
        ConsoleResponder responder(mode == 'q' ? 0 : mode == 'm' ? 1 : 2);
        searchMoves(c, mode, responder);
        std::cout << std::endl;
        const char *s = responder.getSolution();
        while( s ) {
            if( *s == ' ' )
                ++s;
            int rd = rotateNameToDir(s);
            if( rd == RCOUNT ) {
                std::cout << "fatal: unrecognized move " << s << std::endl;
                return;
            }
            c = cube::compose(c, crotated[rd]);
            s = strchr(s, ' ');
        }
        if( c != csolved ) {
            std::cout << "fatal: bad solution!" << std::endl;
            return;
        }
        std::pair<unsigned, unsigned> &ent = moveCounters[responder.getMoveCount()];
        ++ent.first;
        ent.second += responder.durationTimeMs();
    }
    unsigned durationTimeTot = 0;
    std::cout << "          moves  cubes  avg time (s)" << std::endl;
    std::cout << "---------------  -----  -------------" << std::endl;
    for(auto [moveCount, stats] : moveCounters) {
        std::cout << std::setw(15) << moveCount << std::setw(7) <<
            stats.first << "  " << std::setprecision(4) <<
            stats.second/1000.0/stats.first << std::endl;
        durationTimeTot += stats.second;
    }
    std::cout << "---------------  -----  -------------" << std::endl;
        std::cout << "total " << std::setw(7) << durationTimeTot/1000.0 << " s" << std::setw(7) <<
            cubeCount << "  " << std::setprecision(4) <<
            durationTimeTot/1000.0/cubeCount << std::endl;
    std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    if( argc >= 2 ) {
        const char *s = argv[1];
        USEREVERSE = strchr(s, 'r') != NULL;
        TWOPHASE_SEARCHREV = strchr(s, 'n') ? 1 : 2;
        DEPTH_MAX = atoi(s);
        if( DEPTH_MAX <= 0 || DEPTH_MAX > 99 ) {
            printf("DEPTH_MAX invalid: must be in range 1..99\n");
            return 1;
        }
    }
    std::cout << getSetup() << std::endl;
    permReprInit();
    //printStats();
    if( argc >= 4 ) {
        if(isdigit(argv[3][0]))
            cubeTester(atoi(argv[3]), argv[2][0]);
        else
            solveCube(argv[3], argv[2][0]);
    }else
        runServer();
    return 0;
}

