#include "cubesrepr.h"
#include "cubesreprbg.h"
#include "cubecosets.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
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

static bool cubeFromColorsOnSquares(Responder &responder, const char *squareColors, cube &c)
{
    static const unsigned char R120[3] = { 1, 2, 0 };
    static const unsigned char R240[3] = { 2, 0, 1 };
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
    unsigned m_nextPermReprIdx;
    unsigned m_runningThreadCount;
public:
    AddCubesProgress(unsigned depth)
        : m_cubeCount(0), m_depth(depth),
          m_nextPermReprIdx(0), m_runningThreadCount(THREAD_COUNT)
    {
    }

    bool inc(Responder &responder, unsigned long cubeCount, unsigned *permReprIdxBuf);
    bool inbginc(Responder &responder, unsigned long cubeCount, unsigned *permReprIdxBuf);
};

bool AddCubesProgress::inc(Responder &responder, unsigned long cubeCount, unsigned *permReprIdxBuf)
{
    const unsigned permReprCount = USEREVERSE ? 654 : 984;
    unsigned long cubeCountTot;
    unsigned permReprIdx = 0;
    unsigned runningThreadCount;
    bool isFinish;
    mutexLock();
    cubeCountTot = m_cubeCount += cubeCount;
    isFinish = m_nextPermReprIdx >= permReprCount || isStopRequestedNoLock();
    if( isFinish )
        runningThreadCount = --m_runningThreadCount;
    else
        permReprIdx = m_nextPermReprIdx++;
    mutexUnlock();
    *permReprIdxBuf = permReprIdx;
    if( m_depth >= 9 ) {
        if( isFinish ) {
            responder.progress("depth %d cubes %d threads still running",
                    m_depth, runningThreadCount);
        }else{
            unsigned procCountNext = 100 * (permReprIdx+1) / permReprCount;
            unsigned procCountCur = 100 * permReprIdx / permReprCount;
            if( procCountNext != procCountCur && (m_depth >= 10 || procCountCur % 10 == 0) )
                responder.progress("depth %u cubes %llu, %u%%",
                        m_depth, cubeCountTot, 100 * permReprIdx / permReprCount);
        }
    }
    return isFinish;
}

bool AddCubesProgress::inbginc(Responder &responder, unsigned long cubeCount, unsigned *permReprIdxBuf)
{
    unsigned permReprIdx = 0;
    bool isFinish;
    mutexLock();
    m_cubeCount += cubeCount;
    isFinish = m_nextPermReprIdx >= (USEREVERSE ? 1672 : 2768) || isStopRequestedNoLock();
    if( isFinish )
        --m_runningThreadCount;
    else
        permReprIdx = m_nextPermReprIdx++;
    mutexUnlock();
    *permReprIdxBuf = permReprIdx;
    return isFinish;
}

static void addCubesT(unsigned threadNo,
        CubesReprByDepth *cubesReprByDepth, int depth,
        Responder *responder, AddCubesProgress *addCubesProgress)
{
    unsigned long cubeCount = 0;
    unsigned permReprIdx;

    while( !addCubesProgress->inc(*responder, cubeCount, &permReprIdx) ) {
        cubeCount = addCubesForReprPerm(cubesReprByDepth, permReprIdx, depth);
    }
}

static void initOccurT(unsigned threadNo, CubesReprAtDepth *cubesReprAtDepth)
{
    for(unsigned i = threadNo; i < cubesReprAtDepth->size(); i += THREAD_COUNT)
        cubesReprAtDepth->initOccur(i);
}

static bool addCubes(CubesReprByDepth &cubesReprByDepth, unsigned requestedDepth,
        Responder &responder)
{
    if( requestedDepth >= cubesReprByDepth.availMaxCount() ) {
        std::cout << "fatal: addCubes: requested depth=" << requestedDepth << ", max=" <<
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
        std::vector<cubeedges> ceReprArr = { ceRepr };
        ccoCubes.addCubes(ceReprArr);
        cubesReprByDepth.incAvailCount();
    }
    while( cubesReprByDepth.availCount() <= requestedDepth ) {
        unsigned depth = cubesReprByDepth.availCount();
        AddCubesProgress addCubesProgress(depth);
        runInThreadPool(addCubesT, &cubesReprByDepth,
                    depth, &responder, &addCubesProgress);
        if( ProgressBase::isStopRequested() ) {
            responder.message("canceled");
            return true;
        }
        if( depth >= 8 ) {
            responder.progress("depth %d init occur", depth);
            runInThreadPool(initOccurT, &cubesReprByDepth[depth]);
        }
        responder.message("depth %d cubes=%lu", depth,
                    cubesReprByDepth[depth].cubeCount());
        cubesReprByDepth.incAvailCount();
    }
    return false;
}

static void addInSpaceCubesT(unsigned threadNo,
        BGSpaceCubesReprByDepth *cubesReprByDepth, int depth,
        Responder *responder, AddCubesProgress *addCubesProgress)
{
    unsigned long cubeCount = 0;
    unsigned permReprIdx;

    while( !addCubesProgress->inbginc(*responder, cubeCount, &permReprIdx) ) {
        cubeCount = addInSpaceCubesForReprPerm(cubesReprByDepth, permReprIdx, depth);
    }
}

static bool addInSpaceCubes(BGSpaceCubesReprByDepth &cubesReprByDepth, unsigned requestedDepth,
        Responder &responder)
{
    if( requestedDepth >= cubesReprByDepth.availMaxCount() ) {
        std::cout << "fatal: addInSpaceCubes: requested depth=" << requestedDepth <<
            ", max=" << cubesReprByDepth.availMaxCount()-1 << std::endl;
        exit(1);
    }
    if( cubesReprByDepth.availCount() == 0 ) {
        // insert the solved cube
        unsigned cornerPermReprIdx = inbgspaceCubecornerPermRepresentativeIdx(csolved.ccp);
        BGSpaceCornerPermReprCubes &ccpCubes = cubesReprByDepth[0].add(cornerPermReprIdx);
        cubeedges ceRepr = inbgspaceCubeedgesRepresentative(csolved.ccp, csolved.ce);
        std::vector<cubeedges> ceReprArr = { ceRepr };
        ccpCubes.addCubes(ceReprArr);
        cubesReprByDepth.incAvailCount();
    }
    while( cubesReprByDepth.availCount() <= requestedDepth ) {
        unsigned depth = cubesReprByDepth.availCount();
        AddCubesProgress addCubesProgress(depth);
        runInThreadPool(addInSpaceCubesT, &cubesReprByDepth,
                    depth, &responder, &addCubesProgress);
        if( ProgressBase::isStopRequested() ) {
            responder.message("canceled");
            return true;
        }
        responder.message("depth %d in-space cubes=%lu", depth,
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

struct SearchIndexes {
    unsigned permReprIdx;
    bool reversed;
    bool symmetric;
    unsigned td;

    bool inc() {
        if( ++td == TCOUNT ) {
            if( symmetric ) {
                if( USEREVERSE ) {
                    if( reversed && ++permReprIdx == 654 )
                        return false;
                    reversed = !reversed;
                }else if( ++permReprIdx == 984 )
                    return false;
            }
            symmetric = !symmetric;
            td = 0;
        }
        return true;
    }

    bool inspaceinc() {
        if( ++td == TCOUNTBG ) {
            if( symmetric ) {
                if( USEREVERSE ) {
                    if( reversed && ++permReprIdx == 1672 )
                        return false;
                    reversed = !reversed;
                }else if( ++permReprIdx == 2768 )
                    return false;
            }
            symmetric = !symmetric;
            td = 0;
        }
        return true;
    }
};

class SearchProgress : public ProgressBase {
    const unsigned m_depth;
    const unsigned m_itemCount;
    unsigned m_nextItemIdx;
    unsigned m_runningThreadCount;
    bool m_isFinish;

public:
    SearchProgress(unsigned depth)
        : m_depth(depth), m_itemCount((USEREVERSE? 2 * 654 : 984) * 2 * TCOUNT),
        m_nextItemIdx(0), m_runningThreadCount(THREAD_COUNT), m_isFinish(false)
    {
    }

    bool inc(Responder &responder, SearchIndexes*);
    bool isFinish() const { return m_isFinish; }
    std::string progressStr();
};

bool SearchProgress::inc(Responder &responder, SearchIndexes *indexesBuf)
{
    bool res;
    unsigned itemIdx;
    mutexLock();
    if( indexesBuf == NULL )
        m_isFinish = true;
    res = !m_isFinish && m_nextItemIdx < m_itemCount && !isStopRequestedNoLock();
    if( res )
        itemIdx = m_nextItemIdx++;
    else
        --m_runningThreadCount;
    mutexUnlock();
    if( res ) {
        indexesBuf->td = itemIdx % TCOUNT;
        unsigned itemIdxDiv = itemIdx / TCOUNT;
        indexesBuf->symmetric = itemIdxDiv & 1;
        itemIdxDiv >>= 1;
        if( USEREVERSE ) {
            indexesBuf->reversed = itemIdxDiv & 1;
            itemIdxDiv >>= 1;
        }else
            indexesBuf->reversed = 0;
        indexesBuf->permReprIdx = itemIdxDiv;
        if( m_depth >= 17 ) {
            unsigned procCountNext = 100 * (itemIdx+1) / m_itemCount;
            unsigned procCountCur = 100 * itemIdx / m_itemCount;
            if( procCountNext != procCountCur && (m_depth>=18 || procCountCur % 10 == 0) )
                responder.progress("depth %d search %d%%",
                        m_depth, 100 * itemIdx / m_itemCount);
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
    std::ostringstream res;
    res << 100 * m_nextItemIdx / m_itemCount << "%";
    return res.str();
}

static std::string getMovesForMatch(const CubesReprByDepth &cubesReprByDepth,
        const cube &cSearch, const cube &c, unsigned searchRev, unsigned searchTd,
        unsigned reversed, unsigned symmetric, unsigned td)
{
    // cube found:
    //  if reversed: cSearch = transform(symmetric((csearch rev))) ⊙  c
    //      (csearch rev) = symmetric(transformReverse(cSearch)) ⊙  symmetric(transformReverse(c rev))
    //  if not: cSearch = c ⊙  transform(symmetric(csearch))
    //      (csearch rev) = (symmetric(transformReverse(cSearch)) rev) ⊙  (symmetric(transformReverse(c rev)) rev)
    cube cSearchT = cSearch.transform(transformReverse(td));
    cube cSearchTsymm = symmetric ? cSearchT.symmetric() : cSearchT;
    cube cT = c.transform(transformReverse(td));
    cube cTsymm = symmetric ? cT.symmetric() : cT;
    if( searchTd ) {
        cSearchTsymm = cSearchTsymm.transform(transformReverse(searchTd));
        cTsymm = cTsymm.transform(transformReverse(searchTd));
    }
    std::string moves;
    if( searchRev ) {
        moves = printMoves(cubesReprByDepth, cTsymm, !reversed);
        moves += printMoves(cubesReprByDepth, cSearchTsymm, reversed);
    }else{
        moves = printMoves(cubesReprByDepth, cSearchTsymm, !reversed);
        moves += printMoves(cubesReprByDepth, cTsymm, reversed);
    }
    return moves;
}

static void generateSearchTarr(const cube &csearch, cube cSearchTarr[2][2][TCOUNT])
{
    for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
        cube csearchrev = reversed ? csearch.reverse() : csearch;
        for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
            cube csearchrevsymm = symmetric ? csearchrev.symmetric() : csearchrev;
            for(unsigned td = 0; td < TCOUNT; ++td)
                cSearchTarr[reversed][symmetric][td] =
                    csearchrevsymm.transform(td);
        }
    }
}

static bool searchMovesForIdxs(const CubesReprByDepth &cubesReprByDepth,
        unsigned depth, unsigned depthMax, const cube cSearchTarr[2][2][TCOUNT],
        const SearchIndexes &indexes, std::string &moves,
        unsigned searchRev, unsigned searchTd)
{
    std::vector<EdgeReprCandidateTransform> otransform;
    CubesReprAtDepth::ccpcubes_iter ccpIt =
        cubesReprByDepth[depth].ccpCubesBegin() + indexes.permReprIdx;
    const CornerPermReprCubes &ccpReprCubes = ccpIt->second;
    cubecorners_perm ccp = ccpIt->first;
    if( ccpReprCubes.empty() )
        return false;
    const cube &cSearchT = cSearchTarr[indexes.reversed][indexes.symmetric][indexes.td];
    cubecorners_perm ccpSearch = indexes.reversed ?
        cubecorners_perm::compose(cSearchT.ccp, ccp) :
        cubecorners_perm::compose(ccp, cSearchT.ccp);
    unsigned ccpReprSearchIdx = cubecornerPermRepresentativeIdx(ccpSearch);
    const CornerPermReprCubes &ccpReprSearchCubes =
        cubesReprByDepth[depthMax].getAt(ccpReprSearchIdx);
    if( ccpReprCubes.size() <= ccpReprSearchCubes.size() || !isSingleTransform(ccpSearch) ) {
        for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpReprCubes.ccoCubesBegin();
                ccoCubesIt != ccpReprCubes.ccoCubesEnd(); ++ccoCubesIt)
        {
            const CornerOrientReprCubes &ccoReprCubes = *ccoCubesIt;
            cubecorner_orients cco = ccoReprCubes.getOrients();
            cubecorner_orients ccoSearch = indexes.reversed ?
                cubecorner_orients::compose(cSearchT.cco, ccp, cco) :
                cubecorner_orients::compose(cco, cSearchT.ccp, cSearchT.cco);
            cubecorner_orients ccoSearchRepr = cubecornerOrientsComposedRepresentative(
                    ccpSearch, ccoSearch, indexes.reversed, cSearchT.ce, otransform);
            const CornerOrientReprCubes &ccoReprSearchCubes =
                ccpReprSearchCubes.cornerOrientCubesAt(ccoSearchRepr);
            if( ccoReprSearchCubes.empty() )
                continue;
            cubeedges ce = CornerOrientReprCubes::findSolutionEdge(
                    ccoReprCubes, ccoReprSearchCubes, otransform, indexes.reversed);
            if( !ce.isNil() ) {
                cubeedges ceSearch = indexes.reversed ?
                    cubeedges::compose(cSearchT.ce, ce) :
                    cubeedges::compose(ce, cSearchT.ce);
                cube cSearch = { .ccp = ccpSearch, .cco = ccoSearch, .ce = ceSearch };
                cube c = { .ccp = ccp, .cco = cco, .ce = ce };
                moves = getMovesForMatch(cubesReprByDepth, cSearch, c, searchRev,
                        searchTd, indexes.reversed, indexes.symmetric, indexes.td);
                return true;
            }
        }
    }else{
        for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpReprSearchCubes.ccoCubesBegin();
                ccoCubesIt != ccpReprSearchCubes.ccoCubesEnd(); ++ccoCubesIt)
        {
            const CornerOrientReprCubes &ccoReprSearchCubes = *ccoCubesIt;
            cubecorner_orients ccoSearchRepr = ccoReprSearchCubes.getOrients();
            cubecorner_orients cco = cubecornerOrientsForComposedRepresentative(
                    ccpSearch, ccoSearchRepr, indexes.reversed, cSearchT, otransform);
            const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(cco);
            if( ccoReprCubes.empty() )
                continue;
            cubeedges ce = CornerOrientReprCubes::findSolutionEdge(
                    ccoReprCubes, ccoReprSearchCubes, otransform, indexes.reversed);
            if( !ce.isNil() ) {
                cubecorner_orients ccoSearch = indexes.reversed ?
                    cubecorner_orients::compose(cSearchT.cco, ccp, cco) :
                    cubecorner_orients::compose(cco, cSearchT.ccp, cSearchT.cco);
                cubeedges ceSearch = indexes.reversed ?
                    cubeedges::compose(cSearchT.ce, ce) :
                    cubeedges::compose(ce, cSearchT.ce);
                cube cSearch = { .ccp = ccpSearch, .cco = ccoSearch, .ce = ceSearch };
                cube c = { .ccp = ccp, .cco = cco, .ce = ce };
                moves = getMovesForMatch(cubesReprByDepth, cSearch, c, searchRev,
                        searchTd, indexes.reversed, indexes.symmetric, indexes.td);
                return true;
            }
        }
    }
    return false;
}

static void searchMovesTa(unsigned threadNo,
        const CubesReprByDepth *cubesReprByDepth,
        unsigned depth, unsigned depthMax,
        const cube cSearchTarr[2][2][TCOUNT], Responder *responder,
        SearchProgress *searchProgress)
{
    SearchIndexes indexes;

    while( searchProgress->inc(*responder, &indexes) ) {
        std::string moves;
        if( searchMovesForIdxs(*cubesReprByDepth, depth, depthMax, cSearchTarr, indexes, moves, 0, 0) ) {
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
    const CubesReprAtDepth &ccReprCubesC = (*cubesReprByDepth)[depth];
    std::vector<CubesReprAtDepth::ccpcubes_iter> ccp1FilledIters;
    for(CubesReprAtDepth::ccpcubes_iter ccpCubes1It = ccReprCubesC.ccpCubesBegin();
            ccpCubes1It != ccReprCubesC.ccpCubesEnd(); ++ccpCubes1It)
    {
        if( !ccpCubes1It->second.empty() )
            ccp1FilledIters.push_back(ccpCubes1It);
    }

    SearchIndexes indexes2;
	while( searchProgress->inc(*responder, &indexes2) ) {
        CubesReprAtDepth::ccpcubes_iter cornerPerm2It =
            (*cubesReprByDepth)[DEPTH_MAX].ccpCubesBegin() + indexes2.permReprIdx;
        const CornerPermReprCubes &ccpReprCubes2 = cornerPerm2It->second;
        if( ccpReprCubes2.empty() )
            continue;
        for(CubesReprAtDepth::ccpcubes_iter ccpCubes1It : ccp1FilledIters) {
            const CornerPermReprCubes &ccpCubes1 = ccpCubes1It->second;
            cubecorners_perm ccp1 = ccpCubes1It->first;
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
                                bool isDup = std::find(cubesChecked.begin(),
                                        cubesChecked.end(), c1T) != cubesChecked.end();
                                if( isDup )
                                    continue;
                                cubesChecked.push_back(c1T);
                                cube cSearch1 = cube::compose(c1T, *csearch);
                                cube cSearchTarr[2][2][TCOUNT];
                                generateSearchTarr(cSearch1, cSearchTarr);
                                std::string moves2;
                                if( searchMovesForIdxs(*cubesReprByDepth, DEPTH_MAX,
                                            DEPTH_MAX, cSearchTarr, indexes2, moves2, 0, 0) )
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

static std::string getInSpaceMovesForMatch(const BGSpaceCubesReprByDepth &cubesReprByDepth,
        const cube &cSearch, const cube &c, unsigned searchRev, unsigned searchTd,
        unsigned reversed, unsigned symmetric, unsigned tdidx)
{
    // cube found:
    //  if reversed: cSearch = transform(symmetric((csearch rev))) ⊙  c
    //      (csearch rev) = symmetric(transformReverse(cSearch)) ⊙  symmetric(transformReverse(c rev))
    //  if not: cSearch = c ⊙  transform(symmetric(csearch))
    //      (csearch rev) = (symmetric(transformReverse(cSearch)) rev) ⊙  (symmetric(transformReverse(c rev)) rev)
    unsigned td = BGSpaceTransforms[tdidx];
    cube cSearchT = cSearch.transform(transformReverse(td));
    cube cSearchTsymm = symmetric ? cSearchT.symmetric() : cSearchT;
    cube cT = c.transform(transformReverse(td));
    cube cTsymm = symmetric ? cT.symmetric() : cT;
    std::string moves;
    if( searchRev ) {
        moves = printInSpaceMoves(cubesReprByDepth, cTsymm, searchTd, !reversed);
        moves += printInSpaceMoves(cubesReprByDepth, cSearchTsymm, searchTd, reversed);
    }else{
        moves = printInSpaceMoves(cubesReprByDepth, cSearchTsymm, searchTd, !reversed);
        moves += printInSpaceMoves(cubesReprByDepth, cTsymm, searchTd, reversed);
    }
    return moves;
}

static void generateInSpaceSearchTarr(const cube &csearch, cube cSearchTarr[2][2][TCOUNTBG])
{
    for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
        cube csearchrev = reversed ? csearch.reverse() : csearch;
        for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
            cube csearchrevsymm = symmetric ? csearchrev.symmetric() : csearchrev;
            for(unsigned tdidx = 0; tdidx < TCOUNTBG; ++tdidx)
                cSearchTarr[reversed][symmetric][tdidx] =
                    csearchrevsymm.transform(BGSpaceTransforms[tdidx]);
        }
    }
}

static bool searchInSpaceMovesForIdxs(const BGSpaceCubesReprByDepth &cubesReprByDepth,
        unsigned depth, unsigned depthMax, const cube cSearchTarr[2][2][TCOUNTBG],
        const SearchIndexes &indexes, std::string &moves,
        unsigned searchRev, unsigned searchTd)
{
    BGSpaceCubesReprAtDepth::ccpcubes_iter ccpIt =
        cubesReprByDepth[depth].ccpCubesBegin() + indexes.permReprIdx;
    const BGSpaceCornerPermReprCubes &ccpReprCubes = ccpIt->second;
    cubecorners_perm ccp = ccpIt->first;
    if( ccpReprCubes.empty() )
        return false;
    const cube &cSearchT = cSearchTarr[indexes.reversed][indexes.symmetric][indexes.td];
    cubecorners_perm ccpSearch = indexes.reversed ?
        cubecorners_perm::compose(cSearchT.ccp, ccp) :
        cubecorners_perm::compose(ccp, cSearchT.ccp);
    unsigned ccpReprSearchIdx = inbgspaceCubecornerPermRepresentativeIdx(ccpSearch);
    const BGSpaceCornerPermReprCubes &ccpReprSearchCubes =
        cubesReprByDepth[depthMax].getAt(ccpReprSearchIdx);
    if( ccpReprCubes.size() <= ccpReprSearchCubes.size() || !inbgspaceIsSingleTransform(ccpSearch) ) {
        for(BGSpaceCornerPermReprCubes::edges_iter edgeIt = ccpReprCubes.edgeBegin();
                edgeIt != ccpReprCubes.edgeEnd(); ++edgeIt)
        {
            cubeedges ce = *edgeIt;
            cubeedges ceSearch = indexes.reversed ?
                cubeedges::compose(cSearchT.ce, ce) :
                cubeedges::compose(ce, cSearchT.ce);
            cubeedges ceSearchRepr = inbgspaceCubeedgesRepresentative(ccpSearch, ceSearch);
            if( ccpReprSearchCubes.containsCubeEdges(ceSearchRepr) ) {
                cube cSearch = { .ccp = ccpSearch, .cco = csolved.cco, .ce = ceSearch };
                cube c = { .ccp = ccp, .cco = csolved.cco, .ce = ce };
                moves = getInSpaceMovesForMatch(cubesReprByDepth, cSearch, c, searchRev,
                        searchTd, indexes.reversed, indexes.symmetric, indexes.td);
                return true;
            }
        }
    }else{
        cube cSearchTrev = cSearchT.reverse();
        for(BGSpaceCornerPermReprCubes::edges_iter edgeIt = ccpReprSearchCubes.edgeBegin();
                edgeIt != ccpReprSearchCubes.edgeEnd(); ++edgeIt)
        {
            cubeedges ceSearchRepr = *edgeIt;
            cubeedges ceSearch = inbgspaceCubeedgesForRepresentative(ccpSearch, ceSearchRepr);
            cubeedges ce = indexes.reversed ?
                cubeedges::compose(cSearchTrev.ce, ceSearch) :
                cubeedges::compose(ceSearch, cSearchTrev.ce);
            if( ccpReprCubes.containsCubeEdges(ce) ) {
                cube cSearch = { .ccp = ccpSearch, .cco = csolved.cco, .ce = ceSearch };
                cube c = { .ccp = ccp, .cco = csolved.cco, .ce = ce };
                moves = getInSpaceMovesForMatch(cubesReprByDepth, cSearch, c, searchRev,
                        searchTd, indexes.reversed, indexes.symmetric, indexes.td);
                return true;
            }
        }
    }
    return false;
}

static const BGSpaceCubesReprByDepth *getCubesInSpace(unsigned depth, Responder &responder)
{
    static BGSpaceCubesReprByDepth cubesReprByDepthInSpace(TWOPHASE_DEPTH2_MAX+1);
    static std::mutex mtx;
    bool isCanceled;

    mtx.lock();
    isCanceled = addInSpaceCubes(cubesReprByDepthInSpace, depth, responder);
    mtx.unlock();
    return isCanceled ? NULL : &cubesReprByDepthInSpace;
}

static bool searchInSpaceMovesA(const BGSpaceCubesReprByDepth &cubesReprByDepthBG,
        const cube cSpaceArr[2][2][TCOUNTBG], unsigned searchRev, unsigned searchTd,
        unsigned depth, unsigned depthMax, std::string &moves)
{
    SearchIndexes indexes {};
    do {
        if( searchInSpaceMovesForIdxs(cubesReprByDepthBG, depth, depthMax, cSpaceArr, indexes,
                    moves, searchRev, searchTd) )
            return true;
    } while( indexes.inspaceinc() );
    return false;
}

static bool searchInSpaceMovesB(const BGSpaceCubesReprByDepth &cubesReprByDepthBG,
		const cube &cSpace, unsigned searchRev, unsigned searchTd, unsigned depth, unsigned depthMax,
        std::string &moves)
{
    const BGSpaceCubesReprAtDepth &ccReprCubesC = cubesReprByDepthBG[depth];
    for(BGSpaceCubesReprAtDepth::ccpcubes_iter ccpCubes1It = ccReprCubesC.ccpCubesBegin();
            ccpCubes1It != ccReprCubesC.ccpCubesEnd(); ++ccpCubes1It)
    {
        const BGSpaceCornerPermReprCubes &ccpCubes1 = ccpCubes1It->second;
        cubecorners_perm ccp1 = ccpCubes1It->first;
        if( ccpCubes1.empty() )
            continue;
        for(BGSpaceCornerPermReprCubes::edges_iter edge1It = ccpCubes1.edgeBegin();
                edge1It != ccpCubes1.edgeEnd(); ++edge1It)
        {
            const cubeedges ce1 = *edge1It;
            cube c1 = { .ccp = ccp1, .cco = csolved.cco, .ce = ce1 };
            std::vector<cube> cubesChecked;
            for(unsigned reversed1 = 0; reversed1 < (USEREVERSE ? 2 : 1); ++reversed1) {
                cube c1r = reversed1 ? c1.reverse() : c1;
                for(unsigned symmetric1 = 0; symmetric1 < 2; ++symmetric1) {
                    cube c1rs = symmetric1 ? c1r.symmetric() : c1r;
                    for(unsigned td1idx = 0; td1idx < TCOUNTBG; ++td1idx) {
                        const cube c1T = c1rs.transform(BGSpaceTransforms[td1idx]);
                        bool isDup = std::find(cubesChecked.begin(),
                                cubesChecked.end(), c1T) != cubesChecked.end();
                        if( isDup )
                            continue;
                        cubesChecked.push_back(c1T);
                        cube cSearch1 = cube::compose(c1T, cSpace);
                        std::string moves2;
                        cube cSpaceArr[2][2][TCOUNTBG];
                        generateInSpaceSearchTarr(cSearch1, cSpaceArr);
                        if( searchInSpaceMovesA(cubesReprByDepthBG, cSpaceArr, searchRev,
                                    searchTd, depthMax, depthMax, moves2) )
                        {
                            if( searchRev ) {
                                moves = printInSpaceMoves(
                                        cubesReprByDepthBG, c1T, searchTd, true) + moves2;
                            }else{
                                moves = moves2 + printInSpaceMoves(
                                        cubesReprByDepthBG, c1T, searchTd);
                            }
                            return true;
                        }
                    }
                }
            }
        }
	}
    return false;
}

static bool containsInSpaceCube(const BGSpaceCubesReprAtDepth &cubesRepr, const cube &c)
{
    unsigned ccpReprSearchIdx = inbgspaceCubecornerPermRepresentativeIdx(c.ccp);
    const BGSpaceCornerPermReprCubes &ccpReprSearchCubes = cubesRepr.getAt(ccpReprSearchIdx);
    cubeedges ceSearchRepr = inbgspaceCubeedgesRepresentative(c.ccp, c.ce);
    return ccpReprSearchCubes.containsCubeEdges(ceSearchRepr);
}

static int searchInSpaceMoves(const cube &cSpace, unsigned searchRev, unsigned searchTd,
        unsigned movesMax, Responder &responder, std::string &moves)
{
    const BGSpaceCubesReprByDepth *cubesReprByDepthBG = getCubesInSpace(0, responder);
    if( cubesReprByDepthBG == NULL )
        return -1;
    for(unsigned depthSearch = 0; depthSearch <= movesMax &&
            depthSearch < cubesReprByDepthBG->availCount(); ++depthSearch)
    {
        if( containsInSpaceCube((*cubesReprByDepthBG)[depthSearch], cSpace) ) {
            moves = printInSpaceMoves(*cubesReprByDepthBG, cSpace, searchTd, !searchRev);
            return depthSearch;
        }
    }
    if( cubesReprByDepthBG->availCount() <= movesMax ) {
        cube cSpaceTarr[2][2][TCOUNTBG];
        generateInSpaceSearchTarr(cSpace, cSpaceTarr);
        for(unsigned depthSearch = cubesReprByDepthBG->availCount();
                depthSearch <= movesMax && depthSearch <= 3*TWOPHASE_DEPTH2_MAX;
                ++depthSearch)
        {
            if( depthSearch < 2*cubesReprByDepthBG->availCount()-1 && depthSearch <= movesMax )
            {
                unsigned depthMax = cubesReprByDepthBG->availCount() - 1;
                if( searchInSpaceMovesA(*cubesReprByDepthBG, cSpaceTarr, searchRev, searchTd,
                            depthSearch-depthMax, depthMax, moves) )
                    return depthSearch;
            }else if( depthSearch <= 2*TWOPHASE_DEPTH2_MAX ) {
                unsigned depth = depthSearch / 2;
                unsigned depthMax = depthSearch - depth;
                cubesReprByDepthBG = getCubesInSpace(depthMax, responder);
                if( cubesReprByDepthBG == NULL )
                    return -1;
                if( searchInSpaceMovesA(*cubesReprByDepthBG, cSpaceTarr, searchRev, searchTd,
                            depth, depthMax, moves) )
                    return depthSearch;
                ++depthSearch;
            }else{
                cubesReprByDepthBG = getCubesInSpace(TWOPHASE_DEPTH2_MAX, responder);
                if( cubesReprByDepthBG == NULL )
                    return -1;
                unsigned depth = depthSearch - 2*TWOPHASE_DEPTH2_MAX;
                if( searchInSpaceMovesB(*cubesReprByDepthBG, cSpace, searchRev, searchTd,
                            depth, TWOPHASE_DEPTH2_MAX, moves) )
                    return 2*TWOPHASE_DEPTH2_MAX + depth;
            }
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
        int depthInSpace = searchInSpaceMoves(cSpace, searchRev, searchTd,
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
                responder.progress("depth %d search %d%%",
                        m_depthSearch, 100 * itemIdx / itemCount);
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
    static CubesReprByDepth cubesReprByDepth(DEPTH_MAX+1);
    bool isCanceled =  addCubes(cubesReprByDepth, depth, responder);
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
    cube cSearchTarr[2][2][TCOUNT];
    generateSearchTarr(csearch, cSearchTarr);
    SearchProgress searchProgress(depthSearch);
    runInThreadPool(searchMovesTa, cubesReprByDepth,
                depth, depthMax, cSearchTarr, &responder, &searchProgress);
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
    SearchProgress searchProgress(2*DEPTH_MAX+depth);
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
            if( cubeFromColorsOnSquares(responder, reqParam+2, c) ) {
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
            flockfile(stdout);
            std::cout << "\r" << durationTime() << " " << msg << pad << std::flush;
            if( !strncmp(msg, "finished at ", 12) || m_verboseLevel >= 3 )
                std::cout << std::endl;
            funlockfile(stdout);
        }
        break;
    case MT_PROGRESS:
        if( m_verboseLevel > 1 ) {
            flockfile(stdout);
            std::cout << "\r" << durationTime() << " " << msg << pad << std::flush;
            funlockfile(stdout);
        }
        break;
    case MT_MOVECOUNT:
        flockfile(stdout);
        std::cout << "\r" << durationTime() << " moves: " << msg << pad << std::endl;
        m_movecount = msg;
        funlockfile(stdout);
        break;
    case MT_SOLUTION:
        flockfile(stdout);
        std::cout << "\r" << durationTime() << msg << pad << std::endl;
        m_solution = msg;
        funlockfile(stdout);
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

static void solveCubes(const std::vector<cube> &cubes, char mode)
{
    if( mode == 'O' ) {
        ConsoleResponder responder(2);
        getReprCubes(DEPTH_MAX, responder);
        std::cout << std::endl;
    }
    std::map<std::string, std::pair<unsigned, unsigned>> moveCounters;
    for(unsigned i = 0; i < cubes.size(); ++i) {
        cube c = cubes[i];
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
            cubes.size() << "  " << std::setprecision(4) <<
            durationTimeTot/1000.0/cubes.size() << std::endl;
    std::cout << std::endl;
}

static std::string mapColorsOnSqaresFromExt(const char *ext) {
    // external format:
    //     U                Yellow
    //   L F R B   =   Blue   Red   Green Orange
    //     D                 White
    // sequence:
    //    yellow   green    red      white    blue     orange
    //  UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB
    const unsigned squaremap[] = {
         2,  5,  8,  1,  4,  7,  0,  3,  6, // upper -> yellow
        45, 46, 47, 48, 49, 50, 51, 52, 53, // back -> orange
        36, 37, 38, 39, 40, 41, 42, 43, 44, // left -> blue
        18, 19, 20, 21, 22, 23, 24, 25, 26, // front -> red
         9, 10, 11, 12, 13, 14, 15, 16, 17, // right -> green
        33, 30, 27, 34, 31, 28, 35, 32, 29  // down -> white
    };
    std::map<char, char> colormap = {
        {'U', 'Y'}, {'R', 'G'}, {'F', 'R'},
        {'D', 'W'}, {'L', 'B'}, {'B', 'O'}
    };
    std::string res;
    for(unsigned i = 0; i < 54; ++i) {
        res += colormap[ext[squaremap[i]]];
    }
    return res;
}

static bool cubeFromScrambleStr(Responder &responder, const char *scrambleStr, cube &c) {
    const char rotateMapFromExtFmt[][3] = {
        "B1", "B2", "B3", "F1", "F2", "F3", "U1", "U2", "U3",
        "D1", "D2", "D3", "R1", "R2", "R3", "L1", "L2", "L3"
    };
    c = csolved;
    unsigned i = 0;
    while(scrambleStr[i] && scrambleStr[i+1]) {
        if( isalnum(scrambleStr[i]) ) {
            unsigned rd = 0;
            while( rd < RCOUNT && (scrambleStr[i] != rotateMapFromExtFmt[rd][0] ||
                        scrambleStr[i+1] != rotateMapFromExtFmt[rd][1]) )
                ++rd;
            if( rd == RCOUNT ) {
                responder.message("Unkown move: %c%c", scrambleStr[i], scrambleStr[i+1]);
                return false;
            }
            c = cube::compose(c, crotated[rd]);
            i += 2;
        }else
            ++i;
    }
    return true;
}

static bool cubeFromString(Responder &responder, const char *cubeStr, cube &c) {
    bool res = false;
    unsigned len = strlen(cubeStr);
    if( strspn(cubeStr, "YOBRGW") == 54 )
        res = cubeFromColorsOnSquares(responder, cubeStr, c);
    else if( strspn(cubeStr, "URFDLB") == 54 ) {
        std::string fromExt = mapColorsOnSqaresFromExt(cubeStr);
        res = cubeFromColorsOnSquares(responder, fromExt.c_str(), c);
    }else if( strspn(cubeStr, "URFDLB123 \t\r\n") == len )
        res = cubeFromScrambleStr(responder, cubeStr, c);
    else
        responder.message("cube string format not recognized: %s", cubeStr);
    return res;
}

static void solveCubes(const char *fnameOrCubeStr, char mode)
{
    std::vector<cube> cubes;

    int len = strlen(fnameOrCubeStr);
    if( len > 4 && !strcmp(fnameOrCubeStr+len-4, ".txt") ) {
        FILE *fp = fopen(fnameOrCubeStr, "r");
        if( fp == NULL ) {
            perror(fnameOrCubeStr);
            return;
        }
        ConsoleResponder responder(2);
        cube c;
        char buf[2000];
        while( fgets(buf, sizeof(buf), fp) ) {
            if( !cubeFromString(responder, buf, c) ) {
                fclose(fp);
                return;
            }
            cubes.push_back(c);
        }
        fclose(fp);
    }else{
        ConsoleResponder responder(2);
        cube c;
        if( !cubeFromString(responder, fnameOrCubeStr, c) )
            return;
        cubes.push_back(c);
    }
    solveCubes(cubes, mode);
}

static void cubeTester(unsigned cubeCount, char mode)
{
    std::vector<cube> cubes;

    for(unsigned i = 0; i < cubeCount; ++i)
        cubes.push_back(generateCube());
    solveCubes(cubes, mode);
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
    permReprInit(USEREVERSE);
    bgspacePermReprInit(USEREVERSE);
    //printStats();
    if( argc >= 4 ) {
        if(isdigit(argv[2][0]))
            cubeTester(atoi(argv[2]), argv[3][0]);
        else
            solveCubes(argv[2], argv[3][0]);
    }else
        runServer();
    return 0;
}

