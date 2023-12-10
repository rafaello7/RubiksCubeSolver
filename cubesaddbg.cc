#include "cubesaddbg.h"
#include "progressbase.h"
#include "cubesreprbg.h"
#include "threadpoolhelper.h"
#include <iostream>
#include <mutex>


class AddBGcubesProgress : public ProgressBase {
    unsigned long m_cubeCount;
    const unsigned m_depth;
    unsigned m_nextPermReprIdx;
    unsigned m_runningThreadCount;
public:
    AddBGcubesProgress(unsigned depth)
        : m_cubeCount(0), m_depth(depth),
          m_nextPermReprIdx(0), m_runningThreadCount(THREAD_COUNT)
    {
    }

    bool inc(Responder &responder, unsigned long cubeCount, unsigned *permReprIdxBuf);
};

bool AddBGcubesProgress::inc(Responder &responder, unsigned long cubeCount, unsigned *permReprIdxBuf)
{
    const unsigned permReprCount = BGSpaceCubesReprAtDepth::size();
    unsigned permReprIdx = 0;
    bool isFinish;
    mutexLock();
    m_cubeCount += cubeCount;
    isFinish = m_nextPermReprIdx >= permReprCount || isStopRequestedNoLock();
    if( isFinish )
        --m_runningThreadCount;
    else
        permReprIdx = m_nextPermReprIdx++;
    mutexUnlock();
    *permReprIdxBuf = permReprIdx;
    return isFinish;
}

static void addInSpaceCubesT(unsigned threadNo,
        BGSpaceCubesReprByDepth *cubesReprByDepth, int depth,
        Responder *responder, AddBGcubesProgress *addCubesProgress)
{
    unsigned long cubeCount = 0;
    unsigned permReprIdx;

    while( !addCubesProgress->inc(*responder, cubeCount, &permReprIdx) ) {
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
        AddBGcubesProgress addCubesProgress(depth);
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

const BGSpaceCubesReprByDepth *getCubesInSpace(unsigned depth, Responder &responder)
{
    static BGSpaceCubesReprByDepth cubesReprByDepthInSpace(TWOPHASE_DEPTH2_MAX+1);
    static std::mutex mtx;
    bool isCanceled;

    mtx.lock();
    isCanceled = addInSpaceCubes(cubesReprByDepthInSpace, depth, responder);
    mtx.unlock();
    return isCanceled ? NULL : &cubesReprByDepthInSpace;
}

