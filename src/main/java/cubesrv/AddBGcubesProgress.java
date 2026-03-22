package cubesrv;

import static cubesrv.ThreadPoolHelper.THREAD_COUNT;

class AddBGcubesProgress extends ProgressBase {
    private final int m_reprPermCount;
    int m_cubeCount = 0;
    int m_nextPermReprIdx = 0;
    int m_runningThreadCount = THREAD_COUNT;

    AddBGcubesProgress(int reprPermCount) {
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
