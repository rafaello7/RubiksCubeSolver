#include "cubesadd.h"
#include "progressbase.h"
#include "cubesrepr.h"
#include "threadpoolhelper.h"
#include <iostream>

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
};

bool AddCubesProgress::inc(Responder &responder, unsigned long cubeCount, unsigned *permReprIdxBuf)
{
    const unsigned permReprCount = CubesReprAtDepth::size();
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
    if( requestedDepth > DEPTH_MAX_AVAIL ) {
        std::cout << "fatal: addCubes: requested depth=" << requestedDepth << ", max=" <<
            DEPTH_MAX_AVAIL << std::endl;
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

const CubesReprByDepth *getReprCubes(unsigned depth, Responder &responder)
{
    static CubesReprByDepth cubesReprByDepth(DEPTH_MAX_AVAIL+1);
    bool isCanceled = addCubes(cubesReprByDepth, depth, responder);
    return isCanceled ? NULL : &cubesReprByDepth;
}

