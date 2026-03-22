package cubesrv;

import static cubesrv.ThreadPoolHelper.THREAD_COUNT;

class OptimalSearchProgress extends ProgressBase {
    private final int m_depth;
    private final boolean m_useReverse;
    private final int m_itemCount;
    int m_nextItemIdx = 0;
    int m_runningThreadCount = THREAD_COUNT;
    boolean m_isFinish = false;

    OptimalSearchProgress(int depth, boolean useReverse) {
        m_depth = depth;
        m_useReverse = useReverse;
        m_itemCount = (useReverse ? 2 * 654 : 984) * 2 * TransformDir.TCOUNT.ordinal();
    }

    boolean isFinish() {
        return m_isFinish;
    }

    boolean inc(Responder responder, OptimalSearchIndexes indexesBuf) {
        boolean res;
        int itemIdx = -1;
        mutexLock();
        if (indexesBuf == null) m_isFinish = true;
        res = !m_isFinish && m_nextItemIdx < m_itemCount && !isStopRequested();
        if (res)
            itemIdx = m_nextItemIdx++;
        else
            --m_runningThreadCount;
        mutexUnlock();
        if (res && indexesBuf != null) {
            indexesBuf.td = itemIdx % TransformDir.TCOUNT.ordinal();
            int itemIdxDiv = itemIdx / TransformDir.TCOUNT.ordinal();
            indexesBuf.symmetric = itemIdxDiv & 1;
            itemIdxDiv = itemIdxDiv >>> 1;
            if (m_useReverse) {
                indexesBuf.reversed = itemIdxDiv & 1;
                itemIdxDiv = itemIdxDiv >>> 1;
            } else {
                indexesBuf.reversed = 0;
            }
            indexesBuf.permReprIdx = itemIdxDiv;
            if (m_depth >= 17) {
                int procCountNext = 100 * (itemIdx + 1) / m_itemCount;
                int procCountCur = 100 * itemIdx / m_itemCount;
                if (procCountNext != procCountCur && (m_depth >= 18 || procCountCur % 10 == 0))
                    responder.progress("depth " + m_depth + " search " + (100 * itemIdx / m_itemCount) + "%");
            }
        } else {
            if (m_depth >= 17)
                responder.progress("depth " + m_depth + " search " + m_runningThreadCount + " threads still running");
        }
        return res;
    }

    String progressStr() {
        return (100 * m_nextItemIdx / m_itemCount) + "%";
    }
}
