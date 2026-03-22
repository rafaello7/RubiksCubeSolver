package cubesrv;

import java.util.concurrent.locks.ReentrantLock;

public class ProgressBase {
    private static final ReentrantLock gProgressLock = new ReentrantLock();
    private static volatile boolean m_isStopRequested = false;

    public static void mutexLock() {
        gProgressLock.lock();
    }

    public static void mutexUnlock() {
        gProgressLock.unlock();
    }

    public static boolean isStopRequested() {
        return m_isStopRequested;
    }

    public static void requestStop() {
        mutexLock();
        m_isStopRequested = true;
        mutexUnlock();
    }

    public static void requestRestart() {
        mutexLock();
        m_isStopRequested = false;
        mutexUnlock();
    }
}

