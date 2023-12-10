#include "progressbase.h"
#include <mutex>

static std::mutex gProgressMutex;
bool ProgressBase::m_isStopRequested;

void ProgressBase::mutexLock() {
    gProgressMutex.lock();
}

void ProgressBase::mutexUnlock() {
    gProgressMutex.unlock();
}

void ProgressBase::requestStop()
{
    mutexLock();
    m_isStopRequested = true;
    mutexUnlock();
}

void ProgressBase::requestRestart()
{
    mutexLock();
    m_isStopRequested = false;
    mutexUnlock();
}

bool ProgressBase::isStopRequested()
{
    mutexLock();
    bool res = m_isStopRequested;
    mutexUnlock();
    return res;
}

