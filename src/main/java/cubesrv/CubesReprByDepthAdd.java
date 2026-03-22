package cubesrv;

import static cubesrv.ThreadPoolHelper.THREAD_COUNT;
import static cubesrv.ThreadPoolHelper.runInThreadPool;

public class CubesReprByDepthAdd {
    private final CubesReprByDepth m_cubesReprByDepth;

    public CubesReprByDepthAdd(boolean useReverse) {
        m_cubesReprByDepth = new CubesReprByDepth(useReverse);
    }

    public boolean isUseReverse() {
        return m_cubesReprByDepth.isUseReverse();
    }

    private static void addCubesT(int threadNo, CubesReprByDepth cubesReprByDepth,
                                  int depth, Responder responder, AddCubesProgress addCubesProgress) {
        int cubeCount = 0;
        int[] permReprIdx = {0};
        while (!addCubesProgress.inc(responder, cubeCount, permReprIdx)) {
            cubeCount = cubesReprByDepth.addCubesForReprPerm(permReprIdx[0], depth);
        }
    }

    private static void initOccurT(int threadNo, CubesReprAtDepth cubesReprAtDepth) {
        for (int i = threadNo; i < cubesReprAtDepth.size(); i += THREAD_COUNT)
            cubesReprAtDepth.initOccur(i);
    }

    private static boolean addCubes(CubesReprByDepth cubesReprByDepth,
                                    int requestedDepth, Responder responder) {
        while (cubesReprByDepth.availCount() <= requestedDepth) {
            int depth = cubesReprByDepth.availCount();
            AddCubesProgress addCubesProgress =
                    new AddCubesProgress(depth, cubesReprByDepth.getAt(depth).size());
            runInThreadPool(threadNo -> addCubesT(threadNo, cubesReprByDepth, depth, responder, addCubesProgress));
            if (ProgressBase.isStopRequested()) {
                responder.message("canceled");
                return true;
            }
            if (depth >= 8) {
                responder.progress("depth " + depth + " init occur");
                runInThreadPool(threadNo -> initOccurT(threadNo, cubesReprByDepth.getAt(depth)));
            }
            responder.message("depth " + depth + " cubes=" + cubesReprByDepth.getAt(depth).cubeCount());
            cubesReprByDepth.incAvailCount();
        }
        return false;
    }

    public CubesReprByDepth getReprCubes(int depth, Responder responder) {
        boolean isCanceled = addCubes(m_cubesReprByDepth, depth, responder);
        return isCanceled ? null : m_cubesReprByDepth;
    }
}
