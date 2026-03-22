package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.CubesRepr.*;
import static cubesrv.ThreadPoolHelper.*;

public class CubesAdd {

    private static class AddCubesProgress extends ProgressBase {
        private final int m_depth;
        private final int m_reprPermCount;
        int m_cubeCount = 0;
        int m_nextPermReprIdx = 0;
        int m_runningThreadCount = THREAD_COUNT;

        AddCubesProgress(int depth, int reprPermCount) {
            m_depth = depth;
            m_reprPermCount = reprPermCount;
        }

        boolean inc(Responder responder, int cubeCount, int[] permReprIdxBuf) {
            int cubeCountTot;
            int permReprIdx = 0;
            int runningThreadCount = -1;
            boolean isFinish;
            mutexLock();
            m_cubeCount += cubeCount;
            cubeCountTot = m_cubeCount;
            isFinish = m_nextPermReprIdx >= m_reprPermCount || isStopRequested();
            if (isFinish)
                runningThreadCount = --m_runningThreadCount;
            else
                permReprIdx = m_nextPermReprIdx++;
            mutexUnlock();
            permReprIdxBuf[0] = permReprIdx;
            if (m_depth >= 9) {
                if (isFinish) {
                    responder.progress("depth " + m_depth + " cubes " + runningThreadCount + " threads still running");
                } else {
                    int procCountNext = 100 * (permReprIdx+1) / m_reprPermCount;
                    int procCountCur  = 100 * permReprIdx / m_reprPermCount;
                    if (procCountNext != procCountCur && (m_depth >= 10 || procCountCur % 10 == 0))
                        responder.progress("depth " + m_depth + " cubes " + cubeCountTot
                                + ", " + (100 * permReprIdx / m_reprPermCount) + "%");
                }
            }
            return isFinish;
        }
    }

    public static class CubesReprByDepthAdd {
        private final CubesReprByDepth m_cubesReprByDepth;

        public CubesReprByDepthAdd(boolean useReverse) {
            m_cubesReprByDepth = new CubesReprByDepth(useReverse);
        }

        public boolean isUseReverse() { return m_cubesReprByDepth.isUseReverse(); }

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
}

