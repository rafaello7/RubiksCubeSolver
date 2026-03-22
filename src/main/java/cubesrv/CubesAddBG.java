package cubesrv;

import static cubesrv.CubesReprBG.*;
import static cubesrv.ThreadPoolHelper.*;
import java.util.concurrent.locks.ReentrantLock;

public class CubesAddBG {
    private static void addInSpaceCubesT(BGCubesReprByDepth cubesReprByDepth, int depth,
            Responder responder, AddBGcubesProgress addCubesProgress) {
        int cubeCount = 0;
        int[] reprPermIdx = {0};
        while (!addCubesProgress.inc(responder, cubeCount, reprPermIdx)) {
            cubeCount = cubesReprByDepth.addCubesForReprPerm(reprPermIdx[0], depth);
        }
    }

    private static boolean addInSpaceCubes(BGCubesReprByDepth cubesReprByDepth,
            int requestedDepth, Responder responder) {
        while (cubesReprByDepth.availCount() <= requestedDepth) {
            int depth = cubesReprByDepth.availCount();
            AddBGcubesProgress addCubesProgress =
                new AddBGcubesProgress(cubesReprByDepth.getAt(depth).size());
            runInThreadPool(threadNo -> addInSpaceCubesT(cubesReprByDepth, depth, responder, addCubesProgress));
            if (ProgressBase.isStopRequested()) {
                responder.message("canceled");
                return true;
            }
            responder.message("depth " + depth + " in-space cubes=" + cubesReprByDepth.getAt(depth).cubeCount());
            cubesReprByDepth.incAvailCount();
        }
        return false;
    }

    public static class BGCubesReprByDepthAdd {
        private final ReentrantLock m_lock = new ReentrantLock();
        private final BGCubesReprByDepth m_cubesReprByDepth;

        public BGCubesReprByDepthAdd(boolean useReverse) {
            m_cubesReprByDepth = new BGCubesReprByDepth(useReverse);
        }

        public boolean isUseReverse() { return m_cubesReprByDepth.isUseReverse(); }

        public BGCubesReprByDepth getReprCubes(int depth, Responder responder) {
            m_lock.lock();
            try {
                boolean isCanceled = addInSpaceCubes(m_cubesReprByDepth, depth, responder);
                return isCanceled ? null : m_cubesReprByDepth;
            } finally {
                m_lock.unlock();
            }
        }
    }
}

