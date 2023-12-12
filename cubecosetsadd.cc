#include "cubecosetsadd.h"
#include "cubesadd.h"
#include "threadpoolhelper.h"
#include <atomic>

static void addBGcosetsT(unsigned threadNo,
        const CubesReprByDepth *cubesReprByDepth, CubeCosets *bgCosets,
        unsigned depth, std::atomic_ulong *reprCubeCount)
{
    unsigned reprCubeCountT = 0;

    const CubesReprAtDepth &ccReprCubesC = (*cubesReprByDepth)[depth];
    for(CubesReprAtDepth::ccpcubes_iter ccpCubesIt = ccReprCubesC.ccpCubesBegin();
            ccpCubesIt != ccReprCubesC.ccpCubesEnd(); ++ccpCubesIt)
    {
        const CornerPermReprCubes &ccpCubes = *ccpCubesIt;
        cubecorners_perm ccp = ccReprCubesC.getPermAt(ccpCubesIt);
        for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpCubes.ccoCubesBegin();
                ccoCubesIt != ccpCubes.ccoCubesEnd(); ++ccoCubesIt)
        {
            const CornerOrientReprCubes &ccoCubes = *ccoCubesIt;
            cubecorner_orients cco = ccoCubes.getOrients();

            for(unsigned reversed = 0;
                    reversed < (cubesReprByDepth->isUseReverse() ? 2 : 1); ++reversed)
            {
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
                                if( (*bgCosets)[depth].addCube(reprCOrientIdx, ceReprBG, c) )
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

static void addBGcosets(const CubesReprByDepth &cubesReprByDepth,
        CubeCosets &bgCosets, unsigned requestedDepth, Responder &responder)
{
    while( bgCosets.availCount() <= requestedDepth ) {
        unsigned depth = bgCosets.availCount();
        std::atomic_ulong reprCubeCount(0);
        runInThreadPool(addBGcosetsT, &cubesReprByDepth, &bgCosets, depth,
                    &reprCubeCount);
        responder.message("depth %d space repr cubes=%lu", depth, reprCubeCount.load());
        bgCosets.incAvailCount();
    }
}

CubeCosetsAdd::CubeCosetsAdd()
    : m_spaceReprCubes(std::max(TWOPHASE_DEPTH1_CATCHFIRST_MAX, TWOPHASE_DEPTH1_MULTI_MAX)+1)
{
}

const CubeCosets *CubeCosetsAdd::getBGcosets(CubesReprByDepthAdd &cubesReprByDepthAdd,
        unsigned depth, Responder &responder)
{
    if( m_spaceReprCubes.availCount() <= depth ) {
        const CubesReprByDepth *cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depth, responder);
        if( cubesReprByDepth == NULL )
            return NULL;
        addBGcosets(*cubesReprByDepth, m_spaceReprCubes, depth, responder);
    }
    return &m_spaceReprCubes;
}

