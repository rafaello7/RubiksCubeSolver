package cubesrv

import kotlinx.coroutines.sync.Mutex

open class ProgressBase {
    companion object {
        val gProgressMutex = Mutex()
        var m_isStopRequested = false

        suspend fun mutexLock() : Unit {
            gProgressMutex.lock()
        }

        fun isStopRequested() : Boolean = m_isStopRequested

        fun mutexUnlock() : Unit {
            gProgressMutex.unlock()
        }

        suspend fun requestStop() : Unit
        {
            mutexLock()
            m_isStopRequested = true
            mutexUnlock()
        }

        suspend fun requestRestart() : Unit
        {
            mutexLock()
            m_isStopRequested = false
            mutexUnlock()
        }
    }
}

