package cubesrv

/* The cSpace cube shall be in BG space. The function searches for a shortest
 * solution consisting from the BG moves, no longer than movesMax. If such
 * solution don't exist, -1 is returned. The -1 value can be returned also when
 * search operation is canceled. This can be distinguished by call of
 * ProgressBase::isStopRequested().
 *
 * When the searchRev parameter is set to true, the moves sequence is reversed.
 * The searchTd parameter should be one of transform enum values: TD_0,
 * TD_C0_7_CW, TD_C0_7_CCW. If non-zero, means that cube is transformed from
 * yellow-white or orange-red space into the blue-green space by the rotation.
 * The found moves are translated into moves in appropriate space.
 */

private class SearchIndexesBG {
    var permReprIdx : Int = 0
    var reversed : Boolean = false
    var symmetric : Boolean = false
    var td : Int = 0

    fun inc(useReverse : Boolean) : Boolean {
        if( ++td == TCOUNTBG ) {
            if( symmetric ) {
                if( useReverse ) {
                    if( reversed && ++permReprIdx == 1672 )
                        return false
                    reversed = !reversed
                }else if( ++permReprIdx == 2768 )
                    return false
            }
            symmetric = !symmetric
            td = 0
        }
        return true
    }
}

private fun getInSpaceMovesForMatch(cubesReprByDepth : BGCubesReprByDepth,
        cSearch : cube, c : cube, searchRev : Boolean, searchTd : Int,
        reversed : Int, symmetric : Int, tdidx : Int) : String
{
    // cube found:
    //  if reversed: cSearch = transform(symmetric((csearch rev))) ⊙  c
    //      (csearch rev) = symmetric(transformReverse(cSearch)) ⊙  symmetric(transformReverse(c rev))
    //  if not: cSearch = c ⊙  transform(symmetric(csearch))
    //      (csearch rev) = (symmetric(transformReverse(cSearch)) rev) ⊙  (symmetric(transformReverse(c rev)) rev)
    val td : Int = BGSpaceTransforms[tdidx].ordinal
    val cSearchT : cube = cSearch.transform(transformReverse(td))
    val cSearchTsymm : cube = if(symmetric!=0) cSearchT.symmetric() else cSearchT
    val cT : cube = c.transform(transformReverse(td))
    val cTsymm : cube = if(symmetric!=0) cT.symmetric() else cT
    var moves : String
    if( searchRev ) {
        moves = cubesReprByDepth.getMoves(cTsymm, searchTd, reversed == 0)
        moves += cubesReprByDepth.getMoves(cSearchTsymm, searchTd, reversed != 0)
    }else{
        moves = cubesReprByDepth.getMoves(cSearchTsymm, searchTd, reversed == 0)
        moves += cubesReprByDepth.getMoves(cTsymm, searchTd, reversed != 0)
    }
    return moves
}

private fun generateInSpaceSearchTarr(csearch : cube, useReverse : Boolean,
        cSearchTarr : Array<Array<Array<cube>>>)
{
    for(reversed in 0 ..< (if(useReverse) 2 else 1)) {
        val csearchrev : cube = if(reversed!=0) csearch.reverse() else csearch
        for(symmetric in 0..1) {
            val csearchrevsymm : cube = if(symmetric!=0) csearchrev.symmetric() else csearchrev
            for(tdidx in 0 ..< TCOUNTBG)
                cSearchTarr[reversed][symmetric][tdidx] =
                    csearchrevsymm.transform(BGSpaceTransforms[tdidx].ordinal)
        }
    }
}

private fun searchInSpaceMovesForIdxs(cubesReprByDepth : BGCubesReprByDepth,
        depth : Int, depthMax : Int, cSearchTarr : Array<Array<Array<cube>>>,
        indexes : SearchIndexesBG, moves : Array<String>,
        searchRev : Boolean, searchTd : Int) : Boolean
{
    val cSearchT : cube = cSearchTarr[if(indexes.reversed)1 else 0][if(indexes.symmetric)1 else 0][indexes.td]
    val c = arrayOf(cube())
    val cSearch = arrayOf(cube())
    if( cubesReprByDepth.searchMovesForReprPerm(indexes.permReprIdx,
            depth, depthMax, cSearchT, indexes.reversed, c, cSearch) )
    {
        moves[0] = getInSpaceMovesForMatch(cubesReprByDepth, cSearch[0], c[0], searchRev,
                searchTd, if(indexes.reversed)1 else 0, if(indexes.symmetric) 1 else 0, indexes.td)
        return true
    }
    return false
}

private fun searchInSpaceMovesA(cubesReprByDepthBG : BGCubesReprByDepth,
        cSpaceArr : Array<Array<Array<cube>>>, searchRev : Boolean, searchTd : Int,
        depth : Int, depthMax : Int, moves : Array<String>) : Boolean
{
    val indexes = SearchIndexesBG()
    val useReverse : Boolean = cubesReprByDepthBG.isUseReverse()
    do {
        if( searchInSpaceMovesForIdxs(cubesReprByDepthBG, depth, depthMax, cSpaceArr, indexes,
                    moves, searchRev, searchTd) )
            return true
    } while( indexes.inc(useReverse) )
    return false
}

private fun searchInSpaceMovesB(cubesReprByDepthBG : BGCubesReprByDepth,
		cSpace : cube, searchRev : Boolean, searchTd : Int, depth : Int, depthMax : Int,
        moves : Array<String>) : Boolean
{
    val ccReprCubesC : BGCubesReprAtDepth = cubesReprByDepthBG.getAt(depth)
    for(idx1 in 0 .. ccReprCubesC.ccpCubesList().size) {
        val ccpCubes1 = ccReprCubesC.ccpCubesList()[idx1]
        val ccp1 : cubecorners_perm = ccReprCubesC.getPermAt(idx1)
        if( ccpCubes1.empty() )
            continue
        for(edges1 in ccpCubes1.edgeList()) {
            val ce1 = cubeedges(edges1)
            val c1 = cube(ccp = ccp1, cco = csolved.cco, ce = ce1)
            val cubesChecked = mutableSetOf<cube>() 
            for(reversed1 in 0 .. if(cubesReprByDepthBG.isUseReverse()) 1 else 0) {
                val c1r : cube = if(reversed1!=0) c1.reverse() else c1
                for(symmetric1 in 0..1) {
                    val c1rs : cube = if(symmetric1!=0) c1r.symmetric() else c1r
                    for(td1idx in 0 ..< TCOUNTBG) {
                        val c1T : cube = c1rs.transform(BGSpaceTransforms[td1idx].ordinal)
                        val isDup : Boolean = cubesChecked.contains(c1T)
                        if( isDup )
                            continue
                        cubesChecked.add(c1T)
                        val cSearch1 : cube = cube.compose(c1T, cSpace)
                        val moves2 = arrayOf("")
                        val cSpaceArr = Array<Array<Array<cube>>>(2) {
                            Array<Array<cube>>(2) { Array<cube>(TCOUNTBG) { cube() }}}
                        generateInSpaceSearchTarr(cSearch1,
                                cubesReprByDepthBG.isUseReverse(), cSpaceArr)
                        if( searchInSpaceMovesA(cubesReprByDepthBG, cSpaceArr, searchRev,
                                    searchTd, depthMax, depthMax, moves2) )
                        {
                            if( searchRev ) {
                                moves[0] = cubesReprByDepthBG.getMoves(
                                        c1T, searchTd, true) + moves2
                            }else{
                                moves[0] = moves2[0] + cubesReprByDepthBG.getMoves(c1T, searchTd)
                            }
                            return true
                        }
                    }
                }
            }
        }
	}
    return false
}

suspend fun searchInSpaceMoves(cubesReprByDepthAdd : BGCubesReprByDepthAdd,
        cSpace : cube, searchRev : Boolean, searchTd : Int,
        movesMax : Int, responder : Responder, moves : Array<String>) : Int
{
    var cubesReprByDepthBGn : BGCubesReprByDepth? = cubesReprByDepthAdd.getReprCubes(0, responder)
    if( cubesReprByDepthBGn == null )
        return -1
    var cubesReprByDepthBG : BGCubesReprByDepth = cubesReprByDepthBGn
    for(depthSearch in 0 .. movesMax) {
        if( depthSearch >= cubesReprByDepthBG.availCount() )
            break
        if( cubesReprByDepthBG.getAt(depthSearch).containsCube(cSpace) ) {
            moves[0] = cubesReprByDepthBG.getMoves(cSpace, searchTd, !searchRev)
            return depthSearch
        }
    }
    if( cubesReprByDepthBG.availCount() <= movesMax ) {
        val cSpaceTarr = Array<Array<Array<cube>>>(2) {
                            Array<Array<cube>>(2) { Array<cube>(TCOUNTBG) { cube() }}}
        generateInSpaceSearchTarr(cSpace, cubesReprByDepthBG.isUseReverse(), cSpaceTarr)
        for(depthSearch in cubesReprByDepthBG.availCount() .. movesMax) {
            if( depthSearch > 3*TWOPHASE_DEPTH2_MAX )
                break
            if( depthSearch < 2*cubesReprByDepthBG.availCount()-1 && depthSearch <= movesMax )
            {
                val depthMax : Int = cubesReprByDepthBG.availCount() - 1
                if( searchInSpaceMovesA(cubesReprByDepthBG, cSpaceTarr, searchRev, searchTd,
                            depthSearch-depthMax, depthMax, moves) )
                    return depthSearch
            }else if( depthSearch <= 2*TWOPHASE_DEPTH2_MAX ) {
                val depth : Int = depthSearch / 2
                val depthMax : Int = depthSearch - depth
                cubesReprByDepthBGn = cubesReprByDepthAdd.getReprCubes(depthMax, responder)
                if( cubesReprByDepthBGn == null )
                    return -1
                cubesReprByDepthBG = cubesReprByDepthBGn
                if( searchInSpaceMovesA(cubesReprByDepthBG, cSpaceTarr, searchRev, searchTd,
                            depth, depthMax, moves) )
                    return depthSearch
            }else{
                cubesReprByDepthBGn = cubesReprByDepthAdd.getReprCubes(TWOPHASE_DEPTH2_MAX, responder)
                if( cubesReprByDepthBGn == null )
                    return -1
                cubesReprByDepthBG = cubesReprByDepthBGn
                val depth : Int = depthSearch - 2*TWOPHASE_DEPTH2_MAX
                if( searchInSpaceMovesB(cubesReprByDepthBG, cSpace, searchRev, searchTd,
                            depth, TWOPHASE_DEPTH2_MAX, moves) )
                    return 2*TWOPHASE_DEPTH2_MAX + depth
            }
        }
    }
    return -1
}

