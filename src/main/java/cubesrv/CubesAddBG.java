package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.CubesReprBG.*;
import static cubesrv.ThreadPoolHelper.*;
import java.util.concurrent.locks.ReentrantLock;

public class CubesAddBG {

    private static class AddBGcubesProgress extends ProgressBase {
        private final int m_depth;
        private final int m_reprPermCount;
        int m_cubeCount = 0;
        int m_nextPermReprIdx = 0;
        int m_runningThreadCount = THREAD_COUNT;

        AddBGcubesProgress(int depth, int reprPermCount) {
            m_depth = depth;
            m_reprPermCount = reprPermCount;
        }

        boolean inc(Responder responder, int cubeCount, int[] reprPermIdxBuf) {
            int reprPermIdx = 0;
            boolean isFinish;
            mutexLock();
            m_cubeCount += cubeCount;
            isFinish = m_nextPermReprIdx >= m_reprPermCount || isStopRequested();
            if (isFinish)
                --m_runningThreadCount;
            else
                reprPermIdx = m_nextPermReprIdx++;
            mutexUnlock();
            reprPermIdxBuf[0] = reprPermIdx;
            return isFinish;
        }
    }

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
                new AddBGcubesProgress(depth, cubesReprByDepth.getAt(depth).size());
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

