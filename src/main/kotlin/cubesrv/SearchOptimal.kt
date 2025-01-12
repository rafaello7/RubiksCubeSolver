package cubesrv

private class SearchIndexes {
    var permReprIdx : Int = 0
    var reversed : Int = 0
    var symmetric : Int = 0
    var td : Int = 0
}

private class SearchProgress(val m_depth : Int, val m_useReverse : Boolean) : ProgressBase() {
    val m_itemCount : Int = (if(m_useReverse) 2 * 654 else 984) * 2 * transform_dir.TCOUNT.ordinal
    var m_nextItemIdx : Int = 0
    var m_runningThreadCount : Int = THREAD_COUNT
    var m_isFinish : Boolean = false

    fun isFinish() = m_isFinish

    suspend fun inc(responder : Responder, indexesBuf : SearchIndexes?) : Boolean {
        val res : Boolean
        var itemIdx : Int = -1
        mutexLock()
        if( indexesBuf == null )
            m_isFinish = true
        res = !m_isFinish && m_nextItemIdx < m_itemCount && !isStopRequested()
        if( res )
            itemIdx = m_nextItemIdx++
        else
            --m_runningThreadCount
        mutexUnlock()
        if( res && indexesBuf != null ) {
            indexesBuf.td = itemIdx % transform_dir.TCOUNT.ordinal
            var itemIdxDiv : Int = itemIdx / transform_dir.TCOUNT.ordinal
            indexesBuf.symmetric = itemIdxDiv and 1
            itemIdxDiv = itemIdxDiv ushr 1
            if( m_useReverse ) {
                indexesBuf.reversed = itemIdxDiv and 1
                itemIdxDiv = itemIdxDiv ushr 1
            }else
                indexesBuf.reversed = 0
            indexesBuf.permReprIdx = itemIdxDiv
            if( m_depth >= 17 ) {
                val procCountNext : Int = 100 * (itemIdx+1) / m_itemCount
                val procCountCur : Int = 100 * itemIdx / m_itemCount
                if( procCountNext != procCountCur && (m_depth>=18 || procCountCur % 10 == 0) )
                    responder.progress("depth $m_depth search ${100 * itemIdx / m_itemCount}%")
            }
        }else{
            if( m_depth >= 17 ) {
                responder.progress("depth $m_depth search $m_runningThreadCount threads still running")
            }
        }
        return res
    }

    fun progressStr() : String {
        return "${100 * m_nextItemIdx / m_itemCount}%"
    }
}

private fun getMovesForMatch(cubesReprByDepth : CubesReprByDepth,
        cSearch : cube, c : cube, searchRev : Int, searchTd : Int,
        reversed : Int, symmetric : Int, td : Int) : String
{
    // cube found:
    //  if reversed: cSearch = transform(symmetric((csearch rev))) ⊙  c
    //      (csearch rev) = symmetric(transformReverse(cSearch)) ⊙  symmetric(transformReverse(c rev))
    //  if not: cSearch = c ⊙  transform(symmetric(csearch))
    //      (csearch rev) = (symmetric(transformReverse(cSearch)) rev) ⊙  (symmetric(transformReverse(c rev)) rev)
    val cSearchT : cube = cSearch.transform(transformReverse(td))
    var cSearchTsymm : cube = if(symmetric!=0) cSearchT.symmetric() else cSearchT
    val cT : cube = c.transform(transformReverse(td))
    var cTsymm : cube = if(symmetric!=0) cT.symmetric() else cT
    if( searchTd != 0 ) {
        cSearchTsymm = cSearchTsymm.transform(transformReverse(searchTd))
        cTsymm = cTsymm.transform(transformReverse(searchTd))
    }
    var moves = ""
    if( searchRev != 0 ) {
        moves = cubesReprByDepth.getMoves(cTsymm, reversed == 0)
        moves += cubesReprByDepth.getMoves(cSearchTsymm, reversed != 0)
    }else{
        moves = cubesReprByDepth.getMoves(cSearchTsymm, reversed == 0)
        moves += cubesReprByDepth.getMoves(cTsymm, reversed != 0)
    }
    return moves
}

private fun generateSearchTarr(csearch : cube, useReverse : Boolean,
        cSearchTarr : Array<Array<Array<cube>>>)
{
    for(reversed in 0 ..< (if(useReverse) 2 else 1)) {
        val csearchrev : cube = if(reversed!=0) csearch.reverse() else csearch
        for(symmetric in 0..1) {
            val csearchrevsymm : cube = if(symmetric!=0) csearchrev.symmetric() else csearchrev
            for(td in 0 ..< transform_dir.TCOUNT.ordinal)
                cSearchTarr[reversed][symmetric][td] = csearchrevsymm.transform(td)
        }
    }
}

private fun searchMovesForIdxs(cubesReprByDepth : CubesReprByDepth,
        depth : Int, depthMax : Int, cSearchTarr : Array<Array<Array<cube>>>,
        indexes : SearchIndexes, moves : Array<String>,
        searchRev : Int, searchTd : Int) : Boolean
{
    val cSearchT : cube = cSearchTarr[indexes.reversed][indexes.symmetric][indexes.td]
    val c = arrayOf(cube())
    val cSearch = arrayOf(cube())
    if( cubesReprByDepth.searchMovesForReprPerm(indexes.permReprIdx,
            depth, depthMax, cSearchT, indexes.reversed != 0, c, cSearch) )
    {
        moves[0] = getMovesForMatch(cubesReprByDepth, cSearch[0], c[0], searchRev,
                searchTd, indexes.reversed, indexes.symmetric, indexes.td)
        return true
    }
    return false
}

private suspend fun searchMovesTa(threadNo : Int, cubesReprByDepth : CubesReprByDepth,
        depth : Int, depthMax : Int, cSearchTarr : Array<Array<Array<cube>>>,
        responder : Responder, searchProgress : SearchProgress)
{
    val indexes = SearchIndexes()

    while( searchProgress.inc(responder, indexes) ) {
        var moves = arrayOf("")
        if( searchMovesForIdxs(cubesReprByDepth, depth, depthMax, cSearchTarr, indexes, moves, 0, 0) ) {
            responder.solution(moves[0])
            searchProgress.inc(responder, null)
            return
        }
    }
}

private suspend fun searchMovesTb(threadNo : Int,
        cubesReprByDepth : CubesReprByDepth,
		depth : Int, depthMax : Int, csearch : cube,
        responder : Responder, searchProgress : SearchProgress)
{
    val ccReprCubesC : CubesReprAtDepth = cubesReprByDepth.getAt(depth)
    val ccp1FilledIters = mutableListOf<Pair<cubecorners_perm, CornerPermReprCubes>>()
    for(idx in 0 ..< ccReprCubesC.ccpCubesList().size) {
        val ccpCubes1 = ccReprCubesC.ccpCubesList()[idx]
        if( !ccpCubes1.empty() ) {
            val ccp = ccReprCubesC.getPermAt(idx)
            ccp1FilledIters.add(Pair(ccp, ccpCubes1))
        }
    }

    val indexes2 = SearchIndexes()
	while( searchProgress.inc(responder, indexes2) ) {
        val ccpReprCubes2 : CornerPermReprCubes = 
            cubesReprByDepth.getAt(depthMax).ccpCubesList()[indexes2.permReprIdx]
        if( ccpReprCubes2.empty() )
            continue
        for(pair in ccp1FilledIters) {
            val ccp1 : cubecorners_perm = pair.first
            val ccpCubes1 : CornerPermReprCubes = pair.second
            for(ccoCubes1 in ccpCubes1.ccoCubesList()) {
                val cco1 : cubecorner_orients = ccoCubes1.getOrients()
                for(edges1 in ccoCubes1.edgeList()) {
                    val ce1 = cubeedges(edges1)
                    val c1 = cube(ccp = ccp1, cco = cco1, ce = ce1)
                    val cubesChecked = mutableSetOf<cube>()
                    for(reversed1 in 0 ..< (if(cubesReprByDepth.isUseReverse()) 2 else 1)) {
                        val c1r : cube = if(reversed1 != 0) c1.reverse() else c1
                        for(symmetric1 in 0..1) {
                            val c1rs : cube = if(symmetric1!=0) c1r.symmetric() else c1r
                            for(td1 in 0 ..< transform_dir.TCOUNT.ordinal) {
                                val c1T : cube = c1rs.transform(td1)
                                val isDup : Boolean = !cubesChecked.add(c1T)
                                if( isDup )
                                    continue
                                val cSearch1 : cube = cube.compose(c1T, csearch)
                                val cSearchTarr = Array<Array<Array<cube>>>(2) {
                                    Array<Array<cube>>(2) {
                                        Array<cube>(transform_dir.TCOUNT.ordinal) { cube(); }}}
                                generateSearchTarr(cSearch1,
                                        cubesReprByDepth.isUseReverse(), cSearchTarr)
                                val moves2 = arrayOf("")
                                if( searchMovesForIdxs(cubesReprByDepth, depthMax,
                                            depthMax, cSearchTarr, indexes2, moves2, 0, 0) )
                                {
                                    var moves = moves2[0]
                                    moves += cubesReprByDepth.getMoves(c1T)
                                    responder.solution(moves)
                                    searchProgress.inc(responder, null)
                                    return
                                }
                            }
                        }
                    }
                }
            }
        }
	}
}

private fun searchMovesOptimalA(cubesReprByDepthAdd : CubesReprByDepthAdd,
        csearch : cube, depthSearch : Int, responder : Responder) : Boolean
{
    var cubesReprByDepth : CubesReprByDepth? = cubesReprByDepthAdd.getReprCubes(0, responder)
    if( cubesReprByDepth == null )
        return true
    val depthsAvail : Int = cubesReprByDepth.availCount() - 1
    var depth : Int = -1
    var depthMax : Int = -1
    if( depthSearch <= depthsAvail ) {
        depth = 0
        depthMax = depthSearch
    }else if( depthSearch <= 2 * depthsAvail ) {
        depth = depthSearch - depthsAvail
        depthMax = depthsAvail
    }else{
        depth = depthSearch / 2
        depthMax = depthSearch - depth
        cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depthMax, responder)
        if( cubesReprByDepth == null )
            return true
    }
    val cSearchTarr = Array<Array<Array<cube>>>(2) { Array<Array<cube>>(2) {
        Array<cube>(transform_dir.TCOUNT.ordinal) { cube(); }}}
    generateSearchTarr(csearch, cubesReprByDepth.isUseReverse(), cSearchTarr)
    val searchProgress = SearchProgress(depthSearch, cubesReprByDepth.isUseReverse())
    runInThreadPool() { threadNo ->
        searchMovesTa(threadNo, cubesReprByDepth,
                depth, depthMax, cSearchTarr, responder, searchProgress)
    }
    if( searchProgress.isFinish() ) {
        responder.movecount("$depthSearch")
        responder.message("finished at ${searchProgress.progressStr()}")
        return true
    }
    val isStopRequested : Boolean = ProgressBase.isStopRequested()
    if( isStopRequested )
        responder.message("canceled")
    else
        responder.message("depth $depthSearch end")
    return isStopRequested
}

private fun searchMovesOptimalB(cubesReprByDepthAdd : CubesReprByDepthAdd,
        csearch : cube, depth : Int, depthMax : Int, responder : Responder) : Boolean
{
    val cubesReprByDepth : CubesReprByDepth? = cubesReprByDepthAdd.getReprCubes(depthMax, responder)
    if( cubesReprByDepth == null )
        return true
    val searchProgress = SearchProgress(2*depthMax+depth, cubesReprByDepth.isUseReverse())
    runInThreadPool() { threadNo ->
        searchMovesTb(threadNo, cubesReprByDepth, depth, depthMax, csearch, responder, searchProgress)
    }
    if( searchProgress.isFinish() ) {
        responder.movecount("${2*depthMax+depth}")
        responder.message("finished at ${searchProgress.progressStr()}")
        return true
    }
    val isStopRequested : Boolean = ProgressBase.isStopRequested()
    if( isStopRequested )
        responder.message("canceled")
    else
        responder.message("depth ${2*depthMax+depth} end")
    return isStopRequested
}

fun searchMovesOptimal(cubesReprByDepthAdd : CubesReprByDepthAdd,
        csearch : cube, depthMax : Int, responder : Responder)
{
    for(depthSearch in 0 .. 2*depthMax) {
        if( searchMovesOptimalA(cubesReprByDepthAdd, csearch, depthSearch, responder) )
            return
    }
    for(depth in 1 .. depthMax) {
        if( searchMovesOptimalB(cubesReprByDepthAdd, csearch, depth, depthMax, responder) )
            return
    }
    responder.message("not found")
}
