package cubesrv

val TWOPHASE_SEARCHREV = 2

/* The cSearchMid is an intermediate cube, cube1 ⊙  csearch.
 * Assumes the cSearchMid was found in CubeCosets at depthMax.
 * Searches for a cube from the same BG space among cubesReprByDepth[depthMax].
 */
private suspend fun searchPhase1Cube2(
        cubesReprByDepth : CubesReprByDepth,
        bgcubesReprByDepthAdd : BGCubesReprByDepthAdd,
        cSearchMid : cube, cubes : Collection<cube>,
        searchRev : Int, searchTd : Int, cube2Depth : Int, movesMaxp : Int,
        catchFirst : Boolean, responder : Responder, moves : Array<String>) : Int
{
    var bestMoveCount : Int = -1
    var movesMax = movesMaxp

    for(cube2 in cubes) {
        // cSearchMid = cube1 ⊙  (csearch rev) = cube2 ⊙  cSpace
        // csearch rev = (cube1 rev) ⊙  cube2 ⊙  cSpace
        //
        // cSearchMid = cube1 ⊙  csearch = cube2 ⊙  cSpace
        // cSpace = (cube2 rev) ⊙  cube1 ⊙  csearch
        // csearch = (cube1 rev) ⊙  cube2 ⊙  cSpace
        // csearch rev = (cSpace rev) ⊙  (cube2 rev) ⊙  cube1
        val cSpace : cube = cube.compose(cube2.reverse(), cSearchMid)
        val movesInSpace = arrayOf("")
        val depthInSpace : Int = searchInSpaceMoves(bgcubesReprByDepthAdd, cSpace, searchRev!=0, searchTd,
                movesMax-cube2Depth, responder, movesInSpace)
        if( depthInSpace >= 0 ) {
            //responder.message("found in-space cube at depth %d", depthInSpace)
            val cube2T : cube = cube2.transform(transformReverse(searchTd))
            val cube2Moves : String = cubesReprByDepth.getMoves(cube2T, searchRev==0)
            if( searchRev != 0 )
                moves[0] = cube2Moves + movesInSpace[0]
            else
                moves[0] = movesInSpace[0] + cube2Moves
            bestMoveCount = cube2Depth + depthInSpace
            if( catchFirst || depthInSpace == 0 )
                return bestMoveCount
            movesMax = bestMoveCount-1
        }
    }
    return bestMoveCount
}

private class QuickSearchProgress(val m_itemCount : Int, val m_depthSearch : Int,
            val m_catchFirst : Boolean, var m_movesMax : Int) : ProgressBase()
{
    var m_nextItemIdx : Int = 0
    var m_bestMoveCount : Int = -1
    var m_bestMoves : String = ""

    fun isCatchFirst() : Boolean = m_catchFirst

    suspend fun inc(responder : Responder, itemIdxBuf : Array<Int>) : Int {
        val movesMax : Int
        var itemIdx : Int = -1
        mutexLock()
        if( m_movesMax >= 0 && m_nextItemIdx < m_itemCount && !isStopRequested() ) {
            itemIdx = m_nextItemIdx++
            movesMax = m_movesMax
        }else
            movesMax = -1
        mutexUnlock()
        if( movesMax >= 0 ) {
            itemIdxBuf[0] = itemIdx
            if( m_depthSearch >= 9 ) {
                val procCountNext : Int = 100 * (itemIdx+1) / m_itemCount
                val procCountCur : Int = 100 * itemIdx / m_itemCount
                if( procCountNext != procCountCur && (m_depthSearch>=10 || procCountCur % 10 == 0) )
                    responder.progress("depth $m_depthSearch search ${100 * itemIdx / m_itemCount}%")
            }
        }
        return movesMax
    }

    suspend fun setBestMoves(moves : String, moveCount : Int, responder : Responder) : Int {
        val movesMax : Int
        mutexLock()
        if( m_bestMoveCount < 0 || moveCount < m_bestMoveCount ) {
            m_bestMoves = moves
            m_bestMoveCount = moveCount
            m_movesMax = if(m_catchFirst) -1 else moveCount-1
            responder.movecount("$moveCount at depth: %2d".format(m_depthSearch))
        }
        movesMax = m_movesMax
        mutexUnlock()
        return movesMax
    }

    fun getBestMoves(moves : Array<String>) : Int {
        moves[0] = m_bestMoves
        return m_bestMoveCount
    }
}

private suspend fun searchMovesQuickForCcp(ccp : cubecorners_perm, ccpReprCubes : CornerPermReprCubes,
        cubesReprByDepth : CubesReprByDepth,
        bgcubesReprByDepthAdd : BGCubesReprByDepthAdd,
        bgCosets : CubeCosets,
        csearchWithMovesAppend : List<Pair<cube, String>>,
        depth : Int, depth1Max : Int, movesMaxp : Int,
        responder : Responder, searchProgress : QuickSearchProgress) : Boolean
{
    var movesMax = movesMaxp
    val csearchTarr = Array<Array<MutableList<cube>>>(2) {
        Array<MutableList<cube>>(3) { mutableListOf<cube>() } }

    for((c, moves) in csearchWithMovesAppend) {
        csearchTarr[0][0].add(c)
        csearchTarr[0][1].add(c.transform(1))
        csearchTarr[0][2].add(c.transform(2))
        val crev : cube = c.reverse()
        csearchTarr[1][0].add(crev)
        csearchTarr[1][1].add(crev.transform(1))
        csearchTarr[1][2].add(crev.transform(2))
    }
    for(reversed in 0 ..< (if(cubesReprByDepth.isUseReverse()) 2 else 1)) {
        val ccprev : cubecorners_perm = if(reversed!=0) ccp.reverse() else ccp
        for(symmetric in 0..1) {
            val ccprevsymm : cubecorners_perm = if(symmetric!=0) ccprev.symmetric() else ccprev
            for(td in 0..< transform_dir.TCOUNT.ordinal) {
                val ccpT : cubecorners_perm = ccprevsymm.transform(td)
                for(ccoReprCubes in ccpReprCubes.ccoCubesList()) {
                    val cco : cubecorner_orients = ccoReprCubes.getOrients()
                    val ccorev : cubecorner_orients = if(reversed!=0) cco.reverse(ccp) else cco
                    val ccorevsymm : cubecorner_orients = if(symmetric!=0) ccorev.symmetric() else ccorev
                    val ccoT : cubecorner_orients = ccorevsymm.transform(ccprevsymm, td)
                    val ceTarr = mutableListOf<cubeedges>()
                    for(srchItem in 0..< csearchTarr[0][0].size) {
                        for(searchRev in 0..< TWOPHASE_SEARCHREV) {
                            for(searchTd in 0..< 3) {
                                val csearchT : cube = csearchTarr[searchRev][searchTd][srchItem]
                                val ccpSearch : cubecorners_perm = cubecorners_perm.compose(ccpT, csearchT.ccp)
                                val ccoSearch : cubecorner_orients = cubecorner_orients.compose(ccoT,
                                        csearchT.ccp, csearchT.cco)
                                val ccoSearchReprBG : cubecorner_orients = ccoSearch.representativeBG(ccpSearch)
                                val searchReprCOrientIdx : Int = ccoSearchReprBG.getOrientIdx()
                                if( bgCosets.getAt(depth1Max).containsCCOrients(searchReprCOrientIdx) ) {
                                    if( ceTarr.isEmpty() ) {
                                        for(edges in ccoReprCubes.edgeList()) {
                                            val ce = cubeedges(edges)
                                            val cerev : cubeedges = if(reversed!=0) ce.reverse() else ce
                                            val cerevsymm : cubeedges = if(symmetric!=0) cerev.symmetric() else cerev
                                            val ceT : cubeedges = cerevsymm.transform(td)
                                            ceTarr.add(ceT)
                                        }
                                    }
                                    for(ceT in ceTarr) {
                                        val ceSearch : cubeedges = cubeedges.compose(ceT, csearchT.ce)
                                        val ceSearchSpaceRepr : cubeedges = ceSearch.representativeBG()
                                        val cubesForCE : Collection<cube>? =
                                            bgCosets.getAt(depth1Max).getCubesForCE(
                                                    searchReprCOrientIdx, ceSearchSpaceRepr)
                                        if( cubesForCE != null ) {
                                            val cSearch1 = cube(ccp = ccpSearch, cco = ccoSearch, ce = ceSearch)
                                            val inspaceWithCube2Moves = arrayOf("")
                                            val moveCount : Int = searchPhase1Cube2(cubesReprByDepth,
                                                    bgcubesReprByDepthAdd,
                                                    cSearch1, cubesForCE, searchRev, searchTd,
                                                    depth1Max, movesMax-depth,
                                                    searchProgress.isCatchFirst(),
                                                    responder, inspaceWithCube2Moves)
                                            if( moveCount >= 0 ) {
                                                val cube1 = cube(ccp = ccpT, cco = ccoT, ce = ceT)
                                                val cube1T : cube = cube1.transform(transformReverse(searchTd))
                                                val cube1Moves : String =
                                                    cubesReprByDepth.getMoves(cube1T, searchRev!=0)
                                                val cubeMovesAppend : String =
                                                    csearchWithMovesAppend[srchItem].second
                                                var moves : String =
                                                    if( searchRev != 0 )
                                                        cube1Moves + inspaceWithCube2Moves[0]
                                                    else
                                                        inspaceWithCube2Moves[0] + cube1Moves
                                                moves += cubeMovesAppend
                                                movesMax = searchProgress.setBestMoves(moves,
                                                        depth+moveCount, responder)
                                                if( movesMax < 0 )
                                                    return true
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if( ProgressBase.isStopRequested() )
                        return true
                }
            }
        }
    }
    return false
}

private suspend fun searchMovesQuickTa(threadNo : Int,
        cubesReprByDepth : CubesReprByDepth,
        bgcubesReprByDepthAdd : BGCubesReprByDepthAdd,
        bgCosets : CubeCosets,
        csearch : cube, depth : Int, depth1Max : Int,
        responder : Responder, searchProgress : QuickSearchProgress)
{
    val itemIdx : Array<Int> = arrayOf(-1)

    while( true ) {
        val movesMax = searchProgress.inc(responder, itemIdx)
        if( movesMax < 0 )
            break
        val ccpReprCubes : CornerPermReprCubes = cubesReprByDepth.getAt(depth).getAt(itemIdx[0])
        val ccp : cubecorners_perm = cubesReprByDepth.getReprPermForIdx(itemIdx[0])
        if( !ccpReprCubes.empty() ) {
            val cubesWithMoves = listOf(Pair(csearch, String()))
            if( searchMovesQuickForCcp(ccp, ccpReprCubes, cubesReprByDepth,
                        bgcubesReprByDepthAdd, bgCosets,
                        cubesWithMoves, depth, depth1Max, movesMax,
                        responder, searchProgress) )
                return
        }
    }
}

private suspend fun searchMovesQuickTb1(threadNo : Int, cubesReprByDepth : CubesReprByDepth,
        bgcubesReprByDepthAdd : BGCubesReprByDepthAdd,
        bgCosets : CubeCosets, csearch : cube, depth1Max : Int,
        responder : Responder, searchProgress : QuickSearchProgress)
{
    val item2Idx = arrayOf(-1)
    while( true ) {
        val movesMax = searchProgress.inc(responder, item2Idx)
        if( movesMax < 0 )
            break
        val ccp2ReprCubes : CornerPermReprCubes = cubesReprByDepth.getAt(depth1Max).getAt(item2Idx[0])
        val ccp2 : cubecorners_perm = cubesReprByDepth.getReprPermForIdx(item2Idx[0])
        if( ccp2ReprCubes.empty() )
            continue
        val cubesWithMoves = mutableListOf<Pair<cube, String>>()
        for(rd in 0..< rotate_dir.RCOUNT.ordinal) {
            val c1Search : cube = cube.compose(crotated[rd], csearch)
            val cube1Moves : String = cubesReprByDepth.getMoves(crotated[rd])
            cubesWithMoves.add(Pair(c1Search, cube1Moves))
        }
        if( searchMovesQuickForCcp(ccp2, ccp2ReprCubes, cubesReprByDepth,
                    bgcubesReprByDepthAdd, bgCosets,
                    cubesWithMoves, depth1Max+1, depth1Max, movesMax,
                    responder, searchProgress) )
            return
    }
}

private suspend fun searchMovesQuickTb(threadNo : Int, cubesReprByDepth : CubesReprByDepth,
        bgcubesReprByDepthAdd : BGCubesReprByDepthAdd,
        bgCosets : CubeCosets, csearch : cube, depth : Int, depth1Max : Int,
        responder : Responder, searchProgress : QuickSearchProgress)
{
    val ccReprCubesC : CubesReprAtDepth = cubesReprByDepth.getAt(depth)
    while( true ) {
        val item2Idx = arrayOf(0)
        val movesMax = searchProgress.inc(responder, item2Idx)
        if( movesMax < 0 )
            break
        val ccp2ReprCubes : CornerPermReprCubes = cubesReprByDepth.getAt(depth1Max).getAt(item2Idx[0])
        val ccp2 : cubecorners_perm = cubesReprByDepth.getReprPermForIdx(item2Idx[0])
        if( ccp2ReprCubes.empty() )
            continue
        for(idx1 in 0 ..< ccReprCubesC.ccpCubesList().size) {
            val ccp1ReprCubes : CornerPermReprCubes = ccReprCubesC.ccpCubesList()[idx1]
            val ccp1 : cubecorners_perm = ccReprCubesC.getPermAt(idx1)
            if( ccp1ReprCubes.empty() )
                continue
            for(cco1ReprCubes in ccp1ReprCubes.ccoCubesList()) {
                val cco1 : cubecorner_orients = cco1ReprCubes.getOrients()
                for(edges1 in cco1ReprCubes.edgeList()) {
                    val ce1 = cubeedges(edges1)
                    val cubesChecked = mutableListOf<cube>()
                    val cubesWithMoves = mutableListOf<Pair<cube, String>>()
                    for(reversed1 in 0.. (if(cubesReprByDepth.isUseReverse()) 1 else 0)) {
                        val ccp1rev : cubecorners_perm = if(reversed1!=0) ccp1.reverse() else ccp1
                        val cco1rev : cubecorner_orients = if(reversed1!=0) cco1.reverse(ccp1) else cco1
                        val ce1rev : cubeedges = if(reversed1!=0) ce1.reverse() else ce1
                        for(symmetric1 in 0..< 2) {
                            val ccp1revsymm : cubecorners_perm = if(symmetric1!=0) ccp1rev.symmetric() else ccp1rev
                            val cco1revsymm : cubecorner_orients = if(symmetric1!=0) cco1rev.symmetric() else cco1rev
                            val ce1revsymm : cubeedges = if(symmetric1!=0) ce1rev.symmetric() else ce1rev
                            for(td1 in 0..< transform_dir.TCOUNT.ordinal) {
                                val ccp1T : cubecorners_perm = ccp1revsymm.transform(td1)
                                val cco1T : cubecorner_orients = cco1revsymm.transform(ccp1revsymm, td1)
                                val ce1T : cubeedges = ce1revsymm.transform(td1)

                                val c1T = cube(ccp = ccp1T, cco = cco1T, ce = ce1T)
                                val isDup : Boolean = cubesChecked.contains(c1T);
                                if( isDup )
                                    continue
                                cubesChecked.add(c1T)

                                val ccp1Search : cubecorners_perm = cubecorners_perm.compose(ccp1T, csearch.ccp)
                                val cco1Search : cubecorner_orients = cubecorner_orients.compose(cco1T,
                                        csearch.ccp, csearch.cco)
                                val ce1Search : cubeedges = cubeedges.compose(ce1T, csearch.ce)
                                val c1Search = cube(ccp = ccp1Search, cco = cco1Search, ce = ce1Search)
                                val cube1Moves : String = cubesReprByDepth.getMoves(c1T)
                                cubesWithMoves.add(Pair(c1Search, cube1Moves))
                            }
                        }
                    }
                    if( searchMovesQuickForCcp(ccp2, ccp2ReprCubes, cubesReprByDepth,
                                bgcubesReprByDepthAdd, bgCosets,
                                cubesWithMoves, depth+depth1Max, depth1Max, movesMax,
                                responder, searchProgress) )
                        return
                }
            }
        }
    }
}

private fun searchMovesQuickA(cubesReprByDepthAdd : CubesReprByDepthAdd,
        bgcubesReprByDepthAdd : BGCubesReprByDepthAdd,
        bgCosetsAdd : CubeCosetsAdd,
        csearch : cube, depthSearch : Int, catchFirst : Boolean,
        responder : Responder, movesMax : Int,
        moveCount : Array<Int>, moves : Array<String>) : Boolean
{
    moveCount[0] = -1
    var cubesReprByDepth : CubesReprByDepth? = cubesReprByDepthAdd.getReprCubes(0, responder)
    var bgCosets : CubeCosets? = bgCosetsAdd.getBGcosets(cubesReprByDepthAdd, 0, responder)
    if( cubesReprByDepth == null || bgCosets == null )
        return true
    val depthsAvail : Int = minOf(cubesReprByDepth.availCount(), bgCosets.availCount()) - 1
    val depth : Int
    val depth1Max : Int
    if( depthSearch <= depthsAvail ) {
        depth = 0
        depth1Max = depthSearch
    }else if( depthSearch <= 2 * depthsAvail ) {
        depth = depthSearch - depthsAvail
        depth1Max = depthsAvail
    }else{
        depth = depthSearch / 2
        depth1Max = depthSearch - depth
        cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depth1Max, responder)
        bgCosets = bgCosetsAdd.getBGcosets(cubesReprByDepthAdd, depth1Max, responder)
        if( cubesReprByDepth == null || bgCosets == null )
            return true
    }
    val searchProgress = QuickSearchProgress(cubesReprByDepth.getAt(depth).size(),
            depthSearch, catchFirst, movesMax)
    runInThreadPool() { threadNo ->
        searchMovesQuickTa(threadNo, cubesReprByDepth, bgcubesReprByDepthAdd, bgCosets,
                csearch, depth, depth1Max, responder, searchProgress)
    }
    moveCount[0] = searchProgress.getBestMoves(moves)
    return ProgressBase.isStopRequested()
}

private fun searchMovesQuickB(cubesReprByDepthAdd : CubesReprByDepthAdd,
        bgcubesReprByDepthAdd : BGCubesReprByDepthAdd,
        bgCosetsAdd : CubeCosetsAdd,
        csearch : cube, depth1Max : Int, depthSearch : Int, catchFirst : Boolean,
        responder : Responder, movesMax : Int, moveCount : Array<Int>, moves : Array<String>) : Boolean
{
    moveCount[0] = -1
    val cubesReprByDepth : CubesReprByDepth? = cubesReprByDepthAdd.getReprCubes(depth1Max, responder)
    val bgCosets : CubeCosets? = bgCosetsAdd.getBGcosets(cubesReprByDepthAdd, depth1Max, responder)
    if( cubesReprByDepth == null || bgCosets == null )
        return true
    val depth : Int = depthSearch - 2*depth1Max
    val searchProgress = QuickSearchProgress(cubesReprByDepth.getAt(depth1Max).size(),
            depthSearch, catchFirst, movesMax)
    if( depth == 1 )
        runInThreadPool() { threadNo ->
            searchMovesQuickTb1(threadNo, cubesReprByDepth, bgcubesReprByDepthAdd,
                bgCosets, csearch, depth1Max, responder, searchProgress)
        }
    else
        runInThreadPool() { threadNo ->
            searchMovesQuickTb(threadNo, cubesReprByDepth, bgcubesReprByDepthAdd,
                bgCosets, csearch, depth, depth1Max, responder, searchProgress)
        }
    moveCount[0] = searchProgress.getBestMoves(moves)
    return ProgressBase.isStopRequested()
}

fun searchMovesQuickCatchFirst(
        cubesReprByDepthAdd : CubesReprByDepthAdd,
        bgcubesReprByDepthAdd : BGCubesReprByDepthAdd,
        bgCosetsAdd : CubeCosetsAdd,
        csearch : cube, responder : Responder)
{
    val cubesReprByDepth : CubesReprByDepth? = cubesReprByDepthAdd.getReprCubes(0, responder)
    val bgCosets : CubeCosets? = bgCosetsAdd.getBGcosets(
            cubesReprByDepthAdd, 0, responder)
    if( cubesReprByDepth == null || bgCosets == null )
        return
    var depth1Max = minOf(cubesReprByDepth.availCount(), bgCosets.availCount()) - 1
    depth1Max = maxOf(depth1Max, TWOPHASE_DEPTH1_CATCHFIRST_MAX)
    for(depthSearch in 0..12) {
        if( depthSearch > 3*TWOPHASE_DEPTH1_CATCHFIRST_MAX)
            break
        val moveCount = arrayOf(0)
        val bestMoves = arrayOf("")
        val isFinish : Boolean =
            if( depthSearch <= 2 * depth1Max )
                searchMovesQuickA(cubesReprByDepthAdd, bgcubesReprByDepthAdd,
                        bgCosetsAdd, csearch, depthSearch, true, responder,
                        999, moveCount, bestMoves)
            else
                searchMovesQuickB(cubesReprByDepthAdd, bgcubesReprByDepthAdd,
                        bgCosetsAdd, csearch,
                        TWOPHASE_DEPTH1_CATCHFIRST_MAX, depthSearch,
                        true, responder, 999, moveCount, bestMoves)
        if( moveCount[0] >= 0 ) {
            responder.solution(bestMoves[0])
            return
        }
        if( isFinish )
            return
        responder.message("depth $depthSearch end")
    }
    responder.message("not found")
}

fun searchMovesQuickMulti(
        cubesReprByDepthAdd : CubesReprByDepthAdd,
        bgcubesReprByDepthAdd : BGCubesReprByDepthAdd,
        bgCosetsAdd : CubeCosetsAdd,
        csearch : cube, responder : Responder)
{
    var movesBest : String = ""
    var bestMoveCount : Int = 999
    for(depthSearch in 0..12) {
        if( depthSearch > 3*TWOPHASE_DEPTH1_MULTI_MAX || depthSearch >= bestMoveCount)
            break
        val moveCount = arrayOf(0)
        val moves = arrayOf("")
        val isFinish : Boolean =
            if( depthSearch <= 2 * TWOPHASE_DEPTH1_MULTI_MAX )
                searchMovesQuickA(cubesReprByDepthAdd, bgcubesReprByDepthAdd,
                        bgCosetsAdd, csearch, depthSearch, false, responder,
                        bestMoveCount-1, moveCount, moves)
            else
                searchMovesQuickB(cubesReprByDepthAdd, bgcubesReprByDepthAdd,
                        bgCosetsAdd, csearch, TWOPHASE_DEPTH1_MULTI_MAX, depthSearch,
                        false, responder, bestMoveCount-1, moveCount, moves)
        if( moveCount[0] >= 0 ) {
            movesBest = moves[0]
            bestMoveCount = moveCount[0]
            if( bestMoveCount == 0 )
                break
        }
        if( isFinish )
            break
        responder.message("depth $depthSearch end")
    }
    if( !movesBest.isEmpty() ) {
        responder.solution(movesBest)
        return
    }
    responder.message("not found")
}
