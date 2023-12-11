#include "cubecosetsadd.h"
#include "cubesadd.h"
#include "threadpoolhelper.h"
#include <atomic>

static void addBGSpaceReprCubesT(unsigned threadNo,
        const CubesReprByDepth *cubesReprByDepth, SpaceReprCubes *bgSpaceCubes,
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
                    reversed < (CubesReprByDepth::isUseReverse() ? 2 : 1); ++reversed)
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

void addBGSpaceReprCubes(const CubesReprByDepth &cubesReprByDepth,
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

const SpaceReprCubes *getBGSpaceReprCubes(unsigned depth, Responder &responder)
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

