package cubesrv;

import static cubesrv.ThreadPoolHelper.*;
import java.util.concurrent.atomic.AtomicInteger;

public class CubeCosetsAdd {
    public static final int TWOPHASE_DEPTH1_CATCHFIRST_MAX = 5;
    public static final int TWOPHASE_DEPTH1_MULTI_MAX = 6;

    private static void addBGcosetsT(int threadNo, CubesReprByDepth cubesReprByDepth,
            CubeCosets bgCosets, int depth, AtomicInteger reprCubeCount) {
        int reprCubeCountT = 0;
        CubesReprAtDepth ccReprCubesC = cubesReprByDepth.getAt(depth);
        CornerPermReprCubes[] list = ccReprCubesC.ccpCubesList();
        for (int idx = 0; idx < list.length; idx++) {
            CornerPermReprCubes ccpCubes = list[idx];
            CubecornersPerm ccp = ccReprCubesC.getPermAt(idx);
            for (CornerOrientReprCubes ccoCubes : ccpCubes.ccoCubesList()) {
                CubecornerOrients cco = ccoCubes.getOrients();
                for (int reversed = 0; reversed < (cubesReprByDepth.isUseReverse() ? 2 : 1); reversed++) {
                    CubecornersPerm ccprev = (reversed != 0) ? ccp.reverse() : ccp;
                    CubecornerOrients ccorev = (reversed != 0) ? cco.reverse(ccp) : cco;
                    for (int symmetric = 0; symmetric <= 1; symmetric++) {
                        CubecornersPerm ccprevsymm = (symmetric != 0) ? ccprev.symmetric() : ccprev;
                        CubecornerOrients ccorevsymm = (symmetric != 0) ? ccorev.symmetric() : ccorev;
                        for (int td = 0; td < TransformDir.TCOUNT.ordinal(); td++) {
                            CubecornersPerm ccpT = ccprevsymm.transform(td);
                            CubecornerOrients ccoT = ccorevsymm.transform(ccprevsymm, td);
                            CubecornerOrients ccoReprBG = ccoT.representativeBG(ccpT);
                            int reprCOrientIdx = ccoReprBG.getOrientIdx();
                            if (reprCOrientIdx % THREAD_COUNT == threadNo) {
                                for (long edges : ccoCubes.edgeList()) {
                                    CubeEdges ce = new CubeEdges(edges);
                                    CubeEdges cerev = (reversed != 0) ? ce.reverse() : ce;
                                    CubeEdges cerevsymm = (symmetric != 0) ? cerev.symmetric() : cerev;
                                    CubeEdges ceT = cerevsymm.transform(td);
                                    CubeEdges ceReprBG = ceT.representativeBG();
                                    Cube c = new Cube(ccpT, ccoT, ceT);
                                    if (bgCosets.getAt(depth).addCube(reprCOrientIdx, ceReprBG, c))
                                        ++reprCubeCountT;
                                }
                            }
                        }
                    }
                }
            }
        }
        reprCubeCount.addAndGet(reprCubeCountT);
    }

    private static void addBGcosets(CubesReprByDepth cubesReprByDepth,
            CubeCosets bgCosets, int requestedDepth, Responder responder) {
        while (bgCosets.availCount() <= requestedDepth) {
            int depth = bgCosets.availCount();
            AtomicInteger reprCubeCount = new AtomicInteger(0);
            runInThreadPool(threadNo -> addBGcosetsT(threadNo, cubesReprByDepth, bgCosets, depth, reprCubeCount));
            responder.message("depth " + depth + " space repr cubes=" + reprCubeCount.get());
            bgCosets.incAvailCount();
        }
    }

    private final CubeCosets m_cubeCosets =
        new CubeCosets(Math.max(TWOPHASE_DEPTH1_CATCHFIRST_MAX, TWOPHASE_DEPTH1_MULTI_MAX) + 1);

    public CubeCosets getBGcosets(CubesReprByDepthAdd cubesReprByDepthAdd,
            int depth, Responder responder) {
        if (m_cubeCosets.availCount() <= depth) {
            CubesReprByDepth cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depth, responder);
            if (cubesReprByDepth == null) return null;
            addBGcosets(cubesReprByDepth, m_cubeCosets, depth, responder);
        }
        return m_cubeCosets;
    }
}

