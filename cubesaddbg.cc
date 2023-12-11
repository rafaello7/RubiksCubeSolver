#include "cubesaddbg.h"
#include "progressbase.h"
#include "cubesreprbg.h"
#include "threadpoolhelper.h"
#include <iostream>
#include <mutex>


class AddBGcubesProgress : public ProgressBase {
    unsigned long m_cubeCount;
    const unsigned m_depth;
    const unsigned m_reprPermCount;
    unsigned m_nextPermReprIdx;
    unsigned m_runningThreadCount;
public:
    AddBGcubesProgress(unsigned depth, unsigned reprPermCount)
        : m_cubeCount(0), m_depth(depth), m_reprPermCount(reprPermCount),
          m_nextPermReprIdx(0), m_runningThreadCount(THREAD_COUNT)
    {
    }

    bool inc(Responder &responder, unsigned long cubeCount, unsigned *reprPermIdxBuf);
};

bool AddBGcubesProgress::inc(Responder &responder, unsigned long cubeCount, unsigned *reprPermIdxBuf)
{
    unsigned reprPermIdx = 0;
    bool isFinish;
    mutexLock();
    m_cubeCount += cubeCount;
    isFinish = m_nextPermReprIdx >= m_reprPermCount || isStopRequestedNoLock();
    if( isFinish )
        --m_runningThreadCount;
    else
        reprPermIdx = m_nextPermReprIdx++;
    mutexUnlock();
    *reprPermIdxBuf = reprPermIdx;
    return isFinish;
}

static void addInSpaceCubesT(unsigned threadNo,
        BGCubesReprByDepth *cubesReprByDepth, int depth,
        Responder *responder, AddBGcubesProgress *addCubesProgress)
{
    unsigned long cubeCount = 0;
    unsigned reprPermIdx;

    while( !addCubesProgress->inc(*responder, cubeCount, &reprPermIdx) ) {
        cubeCount = cubesReprByDepth->addCubesForReprPerm(reprPermIdx, depth);
    }
}

static bool addInSpaceCubes(BGCubesReprByDepth &cubesReprByDepth, unsigned requestedDepth,
        Responder &responder)
{
    while( cubesReprByDepth.availCount() <= requestedDepth ) {
        unsigned depth = cubesReprByDepth.availCount();
        AddBGcubesProgress addCubesProgress(depth, cubesReprByDepth[depth].size());
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

BGCubesReprByDepthAdd::BGCubesReprByDepthAdd(bool useReverse)
    : m_cubesReprByDepth(useReverse)
{
}

const BGCubesReprByDepth *BGCubesReprByDepthAdd::getReprCubes(unsigned depth, Responder &responder)
{
    m_mtx.lock();
    bool isCanceled = addInSpaceCubes(m_cubesReprByDepth, depth, responder);
    m_mtx.unlock();
    return isCanceled ? NULL : &m_cubesReprByDepth;
}

