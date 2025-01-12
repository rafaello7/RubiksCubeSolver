package cubesrv

val TWOPHASE_DEPTH2_MAX = 8

class BGCornerPermReprCubes {
    var m_items : LongArray = LongArray(0)

    fun addCubes(cearr : Collection<cubeedges>) : Int {
        val edgeList = mutableListOf<Long>()
        val idxList = mutableListOf<Int>()
        for(ce in cearr) {
            val edge = ce.get()
            if( !(edge in edgeList) ) {
                var idx = m_items.binarySearch(edge)
                if( idx < 0 ) {
                    edgeList.add(edge)
                    idxList.add(-1-idx)
                }
            }
        }
        idxList.sort()
        edgeList.sort()
        val itemsNew = LongArray(m_items.size + edgeList.size)
        var srcPos = 0
        var idxsPos = 0
        var destPos = 0
        while( destPos < itemsNew.size ) {
            if( idxsPos < idxList.size && idxList[idxsPos] == srcPos ) {
                itemsNew[destPos] = edgeList[idxsPos]
                ++idxsPos
            }else{
                itemsNew[destPos] = m_items[srcPos]
                ++srcPos
            }
            ++destPos
        }
        m_items = itemsNew
        return edgeList.size
    }

    fun containsCubeEdges(ce : cubeedges) : Boolean {
        return m_items.binarySearch(ce.get()) >= 0
    }

    fun empty() : Boolean = m_items.isEmpty()
    fun size() : Int = m_items.count()
    fun edgeList() : LongArray = m_items
}

/* A set of representative cubes from BG space reachable at specific depth.
 */
class BGCubesReprAtDepth(val m_reprPerms : BGCubecornerReprPerms) {
    val m_cornerPermReprCubes = Array<BGCornerPermReprCubes>(m_reprPerms.reprPermCount()) {
        BGCornerPermReprCubes() }

    fun size() = m_cornerPermReprCubes.size
    fun cubeCount() : Int {
        var res = 0
        for(reprCube in m_cornerPermReprCubes)
            res += reprCube.size()
        return res
    }

    fun add(idx : Int) : BGCornerPermReprCubes = m_cornerPermReprCubes[idx]
    fun getAt(idx : Int) : BGCornerPermReprCubes = m_cornerPermReprCubes[idx]

    fun ccpCubesList() = m_cornerPermReprCubes

    fun getPermAt(reprPermIdx : Int) : cubecorners_perm {
        return m_reprPerms.getPermForIdx(reprPermIdx)
    }

    fun containsCube(c : cube) : Boolean {
        val ccpReprSearchIdx : Int = m_reprPerms.getReprPermIdx(c.ccp)
        val ccpReprSearchCubes : BGCornerPermReprCubes =
            m_cornerPermReprCubes[ccpReprSearchIdx]
        val ceSearchRepr : cubeedges = m_reprPerms.getReprCubeedges(c.ccp, c.ce)
        return ccpReprSearchCubes.containsCubeEdges(ceSearchRepr)
    }
}

/* A set of representative cubes from BG space (i.e. from subset reachable from csolved
 * only by rotations: ORANGE180, RED180, YELLOW180, WHITE180, GREENCW, GREEN180, GREENCCW,
 * BLUECW, BLUE180, BLUECCW).
 */
class BGCubesReprByDepth(useReverse : Boolean) {
    val m_reprPerms = BGCubecornerReprPerms(useReverse)
    val m_cubesAtDepths = mutableListOf<BGCubesReprAtDepth>()
    var m_availCount : Int = 1

    init {
        // prevent reallocation
        //m_cubesAtDepths.reserve(TWOPHASE_DEPTH2_MAX+1)
        // insert the solved cube at depth 0
        m_cubesAtDepths.add(BGCubesReprAtDepth(m_reprPerms))
        val cornerPermReprIdx : Int = m_reprPerms.getReprPermIdx(csolved.ccp)
        val ccpCubes : BGCornerPermReprCubes = m_cubesAtDepths[0].add(cornerPermReprIdx)
        val ceRepr : cubeedges = m_reprPerms.getReprCubeedges(csolved.ccp, csolved.ce)
        val ceReprArr = listOf(ceRepr)
        ccpCubes.addCubes(ceReprArr)
    }

    fun isUseReverse() : Boolean = m_reprPerms.isUseReverse()
    fun availCount() : Int = m_availCount
    fun incAvailCount() { ++m_availCount; }
    fun getAt(idx : Int) : BGCubesReprAtDepth {
        if( idx > TWOPHASE_DEPTH2_MAX ) {
            // about to reallocate m_cubesAtDepths; can be fatal in other threads
            println("BGCubesReprByDepth.getAt fatal: idx=$idx exceeds TWOPHASE_DEPTH2_MAX=$TWOPHASE_DEPTH2_MAX")
        }
        while( idx >= m_cubesAtDepths.size )
            m_cubesAtDepths.add(BGCubesReprAtDepth(m_reprPerms))
        return m_cubesAtDepths[idx]
    }

    fun getMoves(c : cube, searchTd : Int, movesRevp : Boolean = false) : String {
        var movesRev = movesRevp
        val reverseMoveIdxs = arrayOf(0, 1, 2, 3, 6, 5, 4, 9, 8, 7)
        val transformedMoves = arrayOf(
            arrayOf(
                rotate_dir.BLUE180, rotate_dir.GREEN180, rotate_dir.ORANGE180, rotate_dir.RED180,
                rotate_dir.WHITECW, rotate_dir.WHITE180, rotate_dir.WHITECCW, rotate_dir.YELLOWCW,
                rotate_dir.YELLOW180, rotate_dir.YELLOWCCW
            ),
            arrayOf(
                rotate_dir.YELLOW180, rotate_dir.WHITE180, rotate_dir.BLUE180, rotate_dir.GREEN180,
                rotate_dir.REDCW, rotate_dir.RED180, rotate_dir.REDCCW, rotate_dir.ORANGECW,
                rotate_dir.ORANGE180, rotate_dir.ORANGECCW
            )
        )
        val rotateIdxs = mutableListOf<Int>()
        var insertPos = 0
        val crepr : cube = m_reprPerms.cubeRepresentative(c)
        var ccpReprIdx : Int = m_reprPerms.getReprPermIdx(crepr.ccp)
        var depth : Int = 0
        while( true ) {
            val ccpReprCubes : BGCornerPermReprCubes = m_cubesAtDepths[depth].getAt(ccpReprIdx)
            if( ccpReprCubes.containsCubeEdges(crepr.ce) )
                break
            ++depth
            if( depth >= m_availCount ) {
                println("bg getMoves: depth reached maximum, cube NOT FOUND")
                kotlin.system.exitProcess(1)
            }
        }
        var cc : cube = c
        while( depth-- > 0 ) {
            var cmidx : Int = 0
            val ccRev : cube = cc.reverse()
            var cc1 = cube()
            while( cmidx < RCOUNTBG ) {
                val cm : Int = BGSpaceRotations[cmidx].ordinal
                cc1 = cube.compose(cc, crotated[cm])
                var cc1repr : cube = m_reprPerms.cubeRepresentative(cc1)
                ccpReprIdx = m_reprPerms.getReprPermIdx(cc1repr.ccp)
                val ccpReprCubes : BGCornerPermReprCubes = m_cubesAtDepths[depth].getAt(ccpReprIdx)
                if( ccpReprCubes.containsCubeEdges(cc1repr.ce) )
                    break
                cc1 = cube.compose(ccRev, crotated[cm])
                cc1repr = m_reprPerms.cubeRepresentative(cc1)
                ccpReprIdx = m_reprPerms.getReprPermIdx(cc1repr.ccp)
                val ccpReprCubesRev : BGCornerPermReprCubes =
                    m_cubesAtDepths[depth].getAt(ccpReprIdx)
                if( ccpReprCubesRev.containsCubeEdges(cc1repr.ce) ) {
                    movesRev = !movesRev
                    break
                }
                ++cmidx
            }
            if( cmidx == RCOUNTBG ) {
                println("bg getMoves: cube at depth $depth NOT FOUND")
                kotlin.system.exitProcess(1)
            }
            rotateIdxs.subList(0, insertPos).add(if(movesRev) cmidx else reverseMoveIdxs[cmidx])
            if( movesRev )
                ++insertPos
            cc = cc1
        }
        var res : String = ""
        for(rotateIdx in rotateIdxs) {
            val rotateDir : Int = if(searchTd!=0)
                transformedMoves[searchTd-1][rotateIdx].ordinal else
                BGSpaceRotations[rotateIdx].ordinal
            res += " "
            res += rotateDirName(rotateDir)
        }
        return res
    }

    fun addCubesForReprPerm(reprPermIdx : Int, depth : Int) : Int {
        var cubeCount : Int = 0

        val ccpReprCubesC : BGCubesReprAtDepth = m_cubesAtDepths[depth-1]
        val ccpReprCubesNewP : BGCornerPermReprCubes? = if(depth == 1) null else
            m_cubesAtDepths[depth-2].getAt(reprPermIdx)
        val ccpReprCubesNewC : BGCornerPermReprCubes = ccpReprCubesC.getAt(reprPermIdx)
        val ccpReprCubesNewN : BGCornerPermReprCubes = m_cubesAtDepths[depth].add(reprPermIdx)
        val ccpNewRepr : cubecorners_perm = m_reprPerms.getPermForIdx(reprPermIdx)
        val ccpChecked = mutableSetOf<cubecorners_perm>()
        for(trrev in 0 ..< (if(m_reprPerms.isUseReverse()) 2 else 1)) {
            val ccpNewReprRev : cubecorners_perm = if(trrev!=0) ccpNewRepr.reverse() else ccpNewRepr
            for(symmetric in 0..1) {
                val ccpNewS : cubecorners_perm = if(symmetric!=0) ccpNewReprRev.symmetric() else ccpNewReprRev
                for(tdidx in 0 ..< TCOUNTBG) {
                    val td : Int = BGSpaceTransforms[tdidx].ordinal
                    val ccpNew : cubecorners_perm = ccpNewS.transform(td)
                    if( !ccpChecked.contains(ccpNew) ) {
                        ccpChecked.add(ccpNew)
                        for(rdidx in 0 ..< RCOUNTBG) {
                            val rd : Int = BGSpaceRotations[rdidx].ordinal
                            val rdRev : Int = rotateDirReverse(rd)
                            for(reversed in 0 ..< (if(m_reprPerms.isUseReverse()) 2 else 1)) {
                                val ccp : cubecorners_perm = if(reversed!= 0)
                                    cubecorners_perm.compose(crotated[rdRev].ccp, ccpNew) else
                                    cubecorners_perm.compose(ccpNew, crotated[rdRev].ccp)
                                val ccpReprIdx : Int = m_reprPerms.getReprPermIdx(ccp)
                                if( m_reprPerms.getPermForIdx(ccpReprIdx) == ccp ) {
                                    val cpermReprCubesC : BGCornerPermReprCubes =
                                        ccpReprCubesC.getAt(ccpReprIdx)
                                    val ceNewArr = mutableListOf<cubeedges>()
                                    for(edges in cpermReprCubesC.edgeList()) {
                                        val ce = cubeedges(edges)
                                        val cenew : cubeedges = if(reversed!=0)
                                            cubeedges.compose(crotated[rd].ce, ce) else
                                            cubeedges.compose(ce, crotated[rd].ce)
                                        val cenewRepr : cubeedges =
                                            m_reprPerms.getReprCubeedges(ccpNew, cenew)
                                        if( ccpReprCubesNewP != null &&
                                                ccpReprCubesNewP.containsCubeEdges(cenewRepr) )
                                            continue
                                        if( ccpReprCubesNewC.containsCubeEdges(cenewRepr) )
                                            continue
                                        ceNewArr.add(cenewRepr)
                                    }
                                    if( !ceNewArr.isEmpty() )
                                        cubeCount += ccpReprCubesNewN.addCubes(ceNewArr)
                                }
                            }
                        }
                    }
                }
            }
        }
        return cubeCount
    }

    fun searchMovesForReprPerm(reprPermIdx : Int,
                depth : Int, depthMax : Int, cSearchT : cube,
                reversed : Boolean, c : Array<cube>, cSearch : Array<cube>) : Boolean
    {
        val ccpReprCubes : BGCornerPermReprCubes = m_cubesAtDepths[depth].getAt(reprPermIdx)
        val ccp : cubecorners_perm = m_reprPerms.getPermForIdx(reprPermIdx)
        if( !ccpReprCubes.empty() ) {
            val ccpSearch : cubecorners_perm = if(reversed)
                cubecorners_perm.compose(cSearchT.ccp, ccp) else
                cubecorners_perm.compose(ccp, cSearchT.ccp)
            val ccpReprSearchIdx : Int = m_reprPerms.getReprPermIdx(ccpSearch)
            val ccpReprSearchCubes : BGCornerPermReprCubes =
                m_cubesAtDepths[depthMax].getAt(ccpReprSearchIdx)
            if( ccpReprCubes.size() <= ccpReprSearchCubes.size() ||
                    !m_reprPerms.isSingleTransform(ccpSearch) )
            {
                for(edges in ccpReprCubes.edgeList()) {
                    val ce = cubeedges(edges)
                    val ceSearch : cubeedges = if(reversed)
                        cubeedges.compose(cSearchT.ce, ce) else
                        cubeedges.compose(ce, cSearchT.ce)
                    val ceSearchRepr : cubeedges = m_reprPerms.getReprCubeedges(ccpSearch, ceSearch)
                    if( ccpReprSearchCubes.containsCubeEdges(ceSearchRepr) ) {
                        cSearch[0] = cube(ccp = ccpSearch, cco = csolved.cco, ce = ceSearch)
                        c[0] = cube(ccp = ccp, cco = csolved.cco, ce = ce)
                        return true
                    }
                }
            }else{
                val cSearchTrev : cube = cSearchT.reverse()
                for(edges in ccpReprSearchCubes.edgeList()) {
                    val ceSearchRepr = cubeedges(edges)
                    val ceSearch : cubeedges =
                        m_reprPerms.getCubeedgesForRepresentative(ccpSearch, ceSearchRepr)
                    val ce : cubeedges = if(reversed)
                        cubeedges.compose(cSearchTrev.ce, ceSearch) else
                        cubeedges.compose(ceSearch, cSearchTrev.ce)
                    if( ccpReprCubes.containsCubeEdges(ce) ) {
                        cSearch[0] = cube(ccp = ccpSearch, cco = csolved.cco, ce = ceSearch)
                        c[0] = cube(ccp = ccp, cco = csolved.cco, ce = ce)
                        return true
                    }
                }
            }
        }
        return false
    }
}

