package cubesrv

import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex

private class AddBGcubesProgress(val m_depth : Int, val m_reprPermCount : Int) : ProgressBase() {
    var m_cubeCount : Int = 0
    var m_nextPermReprIdx : Int = 0
    var m_runningThreadCount : Int = THREAD_COUNT

    suspend fun inc(responder : Responder, cubeCount : Int, reprPermIdxBuf : Array<Int>) : Boolean {
        var reprPermIdx : Int = 0
        val isFinish : Boolean
        mutexLock()
        m_cubeCount += cubeCount
        isFinish = m_nextPermReprIdx >= m_reprPermCount || isStopRequested()
        if( isFinish )
            --m_runningThreadCount
        else
            reprPermIdx = m_nextPermReprIdx++
        mutexUnlock()
        reprPermIdxBuf[0] = reprPermIdx
        return isFinish
    }
}

private suspend fun addInSpaceCubesT(
        cubesReprByDepth : BGCubesReprByDepth, depth : Int,
        responder : Responder, addCubesProgress : AddBGcubesProgress)
{
    var cubeCount : Int = 0
    val reprPermIdx = arrayOf(0)
    while( !addCubesProgress.inc(responder, cubeCount, reprPermIdx) ) {
        cubeCount = cubesReprByDepth.addCubesForReprPerm(reprPermIdx[0], depth)
    }
}

private suspend fun addInSpaceCubes(cubesReprByDepth : BGCubesReprByDepth, requestedDepth : Int,
        responder : Responder) : Boolean
{
    while( cubesReprByDepth.availCount() <= requestedDepth ) {
        val depth : Int = cubesReprByDepth.availCount()
        val addCubesProgress = AddBGcubesProgress(depth, cubesReprByDepth.getAt(depth).size())
        coroutineScope () {
            for(threadNo in 0 ..< THREAD_COUNT) {
                launch {
                    addInSpaceCubesT(cubesReprByDepth, depth, responder, addCubesProgress)
                }
            }
        }
        if( ProgressBase.isStopRequested() ) {
            responder.message("canceled")
            return true
        }
        responder.message("depth $depth in-space cubes=${cubesReprByDepth.getAt(depth).cubeCount()}")
        cubesReprByDepth.incAvailCount()
    }
    return false
}

class BGCubesReprByDepthAdd(useReverse : Boolean) {
    private val m_mtx = Mutex()
    private val m_cubesReprByDepth = BGCubesReprByDepth(useReverse)
    fun isUseReverse() : Boolean = m_cubesReprByDepth.isUseReverse()

    /* Returns the cube set filled up to at least the specified depth (inclusive).
     * The function triggers filling procedure when the depth is requested first time.
     * When the filling procedure is canceled, NULL is returned.
     */
    suspend fun getReprCubes(depth : Int, responder : Responder) : BGCubesReprByDepth? {
        m_mtx.lock()
        val isCanceled = addInSpaceCubes(m_cubesReprByDepth, depth, responder)
        m_mtx.unlock()
        return if(isCanceled) null else m_cubesReprByDepth
    }
}
