#ifndef PROGRESSBASE_H
#define PROGRESSBASE_H

class ProgressBase {
    static bool m_isStopRequested;
protected:
    static void mutexLock();
    static bool isStopRequestedNoLock() { return m_isStopRequested; }
    static void mutexUnlock();
public:
    static void requestStop();
    static void requestRestart();
    static bool isStopRequested();
};

#endif // PROGRESSBASE_H
