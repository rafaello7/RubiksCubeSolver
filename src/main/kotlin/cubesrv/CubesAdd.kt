package cubesrv

private class AddCubesProgress(val m_depth : Int, val m_reprPermCount : Int) : ProgressBase() {
    var m_cubeCount : Int = 0
    var m_nextPermReprIdx : Int = 0
    var m_runningThreadCount : Int = THREAD_COUNT

    suspend fun inc(responder : Responder, cubeCount : Int, permReprIdxBuf : Array<Int>) : Boolean {
        val cubeCountTot : Int
        var permReprIdx : Int = 0
        var runningThreadCount : Int = -1
        val isFinish : Boolean
        mutexLock()
        m_cubeCount += cubeCount
        cubeCountTot = m_cubeCount
        isFinish = m_nextPermReprIdx >= m_reprPermCount || isStopRequested()
        if( isFinish )
            runningThreadCount = --m_runningThreadCount
        else
            permReprIdx = m_nextPermReprIdx++
        mutexUnlock()
        permReprIdxBuf[0] = permReprIdx
        if( m_depth >= 9 ) {
            if( isFinish ) {
                responder.progress("depth $m_depth cubes $runningThreadCount threads still running")
            }else{
                val procCountNext : Int = 100 * (permReprIdx+1) / m_reprPermCount
                val procCountCur : Int = 100 * permReprIdx / m_reprPermCount
                if( procCountNext != procCountCur && (m_depth >= 10 || procCountCur % 10 == 0) )
                    responder.progress(
                        "depth $m_depth cubes $cubeCountTot, ${100 * permReprIdx / m_reprPermCount}%")
            }
        }
        return isFinish
    }
}

class CubesReprByDepthAdd(useReverse : Boolean) {
    private val m_cubesReprByDepth = CubesReprByDepth(useReverse)

    fun isUseReverse() : Boolean = m_cubesReprByDepth.isUseReverse()
    private companion object {
        suspend fun addCubesT(threadNo : Int,
                cubesReprByDepth : CubesReprByDepth, depth : Int,
                responder : Responder, addCubesProgress : AddCubesProgress)
        {
            var cubeCount : Int = 0
            val permReprIdx = arrayOf(0)

            while( !addCubesProgress.inc(responder, cubeCount, permReprIdx) ) {
                cubeCount = cubesReprByDepth.addCubesForReprPerm(permReprIdx[0], depth)
            }
        }

        fun initOccurT(threadNo : Int, cubesReprAtDepth : CubesReprAtDepth) {
            for(i in threadNo ..< cubesReprAtDepth.size() step THREAD_COUNT)
                cubesReprAtDepth.initOccur(i)
        }

        fun addCubes(cubesReprByDepth : CubesReprByDepth, requestedDepth : Int,
                responder : Responder) : Boolean
        {
            while( cubesReprByDepth.availCount() <= requestedDepth ) {
                val depth : Int = cubesReprByDepth.availCount()
                val addCubesProgress = AddCubesProgress(depth, cubesReprByDepth.getAt(depth).size())
                runInThreadPool { threadNo ->
                    addCubesT(threadNo, cubesReprByDepth, depth, responder, addCubesProgress)
                }
                if( ProgressBase.isStopRequested() ) {
                    responder.message("canceled")
                    return true
                }
                if( depth >= 8 ) {
                    responder.progress("depth $depth init occur")
                    runInThreadPool { threadNo ->
                        initOccurT(threadNo, cubesReprByDepth.getAt(depth))
                    }
                }
                responder.message("depth $depth cubes=${cubesReprByDepth.getAt(depth).cubeCount()}")
                cubesReprByDepth.incAvailCount()
            }
            return false
        }
    }

    /* Returns the cube set filled up to at least the specified depth (inclusive).
     * The function triggers filling procedure when the depth is requested first time.
     * When the filling procedure is canceled, NULL is returned.
     */
    fun getReprCubes(depth : Int, responder : Responder) : CubesReprByDepth? {
        val isCanceled : Boolean = addCubes(m_cubesReprByDepth, depth, responder)
        return if(isCanceled) null else m_cubesReprByDepth
    }
}

