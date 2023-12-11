#include "cubesadd.h"
#include "progressbase.h"
#include "cubesrepr.h"
#include "threadpoolhelper.h"
#include <iostream>

class AddCubesProgress : public ProgressBase {
    unsigned long m_cubeCount;
    const unsigned m_depth;
    const unsigned m_reprPermCount;
    unsigned m_nextPermReprIdx;
    unsigned m_runningThreadCount;
public:
    AddCubesProgress(unsigned depth, unsigned reprPermCount)
        : m_cubeCount(0), m_depth(depth), m_reprPermCount(reprPermCount),
          m_nextPermReprIdx(0), m_runningThreadCount(THREAD_COUNT)
    {
    }

    bool inc(Responder &responder, unsigned long cubeCount, unsigned *permReprIdxBuf);
};

bool AddCubesProgress::inc(Responder &responder, unsigned long cubeCount, unsigned *permReprIdxBuf)
{
    unsigned long cubeCountTot;
    unsigned permReprIdx = 0;
    unsigned runningThreadCount;
    bool isFinish;
    mutexLock();
    cubeCountTot = m_cubeCount += cubeCount;
    isFinish = m_nextPermReprIdx >= m_reprPermCount || isStopRequestedNoLock();
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
            unsigned procCountNext = 100 * (permReprIdx+1) / m_reprPermCount;
            unsigned procCountCur = 100 * permReprIdx / m_reprPermCount;
            if( procCountNext != procCountCur && (m_depth >= 10 || procCountCur % 10 == 0) )
                responder.progress("depth %u cubes %llu, %u%%",
                        m_depth, cubeCountTot, 100 * permReprIdx / m_reprPermCount);
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
        cubeCount = cubesReprByDepth->addCubesForReprPerm(permReprIdx, depth);
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
    while( cubesReprByDepth.availCount() <= requestedDepth ) {
        unsigned depth = cubesReprByDepth.availCount();
        AddCubesProgress addCubesProgress(depth, cubesReprByDepth[depth].size());
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

CubesReprByDepthAdd::CubesReprByDepthAdd(bool useReverse)
    : m_cubesReprByDepth(useReverse)
{
}

const CubesReprByDepth *CubesReprByDepthAdd::getReprCubes(unsigned depth, Responder &responder)
{
    bool isCanceled = addCubes(m_cubesReprByDepth, depth, responder);
    return isCanceled ? NULL : &m_cubesReprByDepth;
}

