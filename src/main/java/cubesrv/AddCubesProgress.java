package cubesrv;

import static cubesrv.ThreadPoolHelper.THREAD_COUNT;

class AddCubesProgress extends ProgressBase {
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
                int procCountNext = 100 * (permReprIdx + 1) / m_reprPermCount;
                int procCountCur = 100 * permReprIdx / m_reprPermCount;
                if (procCountNext != procCountCur && (m_depth >= 10 || procCountCur % 10 == 0))
                    responder.progress("depth " + m_depth + " cubes " + cubeCountTot
                            + ", " + (100 * permReprIdx / m_reprPermCount) + "%");
            }
        }
        return isFinish;
    }
}
