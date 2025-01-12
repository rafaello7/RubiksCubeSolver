package cubesrv

/* The whole set of representative cubes (CubesReprByDepth) is grouped by
 * depth - number of moves needed to get the cube starting from solved one.
 * Within each depth the cubes are grouped by corner permutation. Within one
 * corner permutation the cubes are grouped by corner orientations.
 */

/* A set of representative cubes for specific corners permutation and
 * orientations.
 */
class CornerOrientReprCubes(var m_orients : cubecorner_orients) {
    var m_items : LongArray = LongArray(0)
    var m_orientOccur : IntArray? = null

    fun getOrients() : cubecorner_orients = m_orients

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

    fun initOccur() {
        val orientOcc = IntArray(64)
        for(edges in m_items) {
            val ce = cubeedges(edges)
            val orientIdx : Int = ce.getOrientIdx()
            orientOcc[orientIdx ushr 5] = orientOcc[orientIdx ushr 5] or (1 shl (orientIdx and 0x1f))
        }
        m_orientOccur = orientOcc
    }

    fun containsCubeEdges(ce : cubeedges) : Boolean {
        if( m_orientOccur != null ) {
            val orientIdx : Int = ce.getOrientIdx()
            val orientOcc : Int = m_orientOccur?.get(orientIdx ushr 5) ?: 0x1ff
            if( (orientOcc and (1 shl (orientIdx and 0x1f))) == 0 )
                return false
        }
        return m_items.binarySearch(ce.get()) >= 0
    }

    fun empty() : Boolean = m_items.isEmpty()
    fun size() : Int = m_items.count()
    fun edgeList() : LongArray = m_items
    companion object {
        fun findSolutionEdgeMulti(ccoReprCubes : CornerOrientReprCubes,
                ccoReprSearchCubes : CornerOrientReprCubes,
                otransform : MutableList<EdgeReprCandidateTransform>,
                reversed : Boolean) : cubeedges
        {
            for(edges in ccoReprCubes.edgeList() ) {
                val ce = cubeedges(edges)
                val ceSearchRepr : cubeedges = CubecornerReprPerms.getComposedReprCubeedges(
                        ce, reversed, otransform)
                if( ccoReprSearchCubes.containsCubeEdges(ceSearchRepr) )
                    return ce
            }
            return cubeedges()
        }

        fun findSolutionEdgeSingle(ccoReprCubes : CornerOrientReprCubes,
                ccoReprSearchCubes : CornerOrientReprCubes,
                erct : EdgeReprCandidateTransform,
                reversed : Boolean) : cubeedges
        {
            val cetrans : cubeedges = ctransformed[erct.transformedIdx].ce
            val cetransRev : cubeedges = ctransformed[transformReverse(erct.transformedIdx)].ce
            if( ccoReprCubes.size() <= ccoReprSearchCubes.size() ) {
                for(edges in ccoReprCubes.edgeList()) {
                    val ce = cubeedges(edges)
                    val cesymm : cubeedges = if(erct.symmetric) ce.symmetric() else ce
                    val ceSearchRepr : cubeedges =
                        if( reversed ) {
                            if( erct.reversed )
                                cubeedges.compose3revmid(cetrans, cesymm, erct.ceTrans)
                            else
                                cubeedges.compose3(erct.ceTrans, cesymm, cetransRev)
                        }else{
                            if( erct.reversed )
                                cubeedges.compose3revmid(erct.ceTrans, cesymm, cetransRev)
                            else
                                cubeedges.compose3(cetrans, cesymm, erct.ceTrans)
                        }
                    if( ccoReprSearchCubes.containsCubeEdges(ceSearchRepr) )
                        return ce
                }
            }else{
                val erctCeTransRev : cubeedges = erct.ceTrans.reverse()
                for(edges in ccoReprSearchCubes.edgeList()) {
                    val ce = cubeedges(edges)
                    val ceSearch : cubeedges =
                        if( reversed ) {
                            if( erct.reversed ) {
                                cubeedges.compose3revmid(erct.ceTrans, ce, cetrans)
                            }else{
                                cubeedges.compose3(erctCeTransRev, ce, cetrans)
                            }
                        }else{
                            if( erct.reversed ) {
                                cubeedges.compose3revmid(cetransRev, ce, erct.ceTrans)
                            }else{
                                cubeedges.compose3(cetransRev, ce, erctCeTransRev)
                            }
                        }
                    val cesymmSearch : cubeedges = if(erct.symmetric) ceSearch.symmetric() else ceSearch
                    if( ccoReprCubes.containsCubeEdges(cesymmSearch) )
                        return cesymmSearch
                }
            }
            return cubeedges()
        }

        fun findSolutionEdge(
                ccoReprCubes : CornerOrientReprCubes,
                ccoReprSearchCubes : CornerOrientReprCubes,
                otransform : MutableList<EdgeReprCandidateTransform>,
                reversed : Boolean) : cubeedges
        {
            return if( otransform.count() == 1 )
                findSolutionEdgeSingle(
                        ccoReprCubes, ccoReprSearchCubes, otransform.first(), reversed)
            else{
                findSolutionEdgeMulti(
                        ccoReprCubes, ccoReprSearchCubes, otransform, reversed)
            }
        }
    }
}

/* A set of representative cubes for specific corners permutation.
 */
class CornerPermReprCubes {
    companion object {
        val m_coreprCubesEmpty = CornerOrientReprCubes(cubecorner_orients())
    }
    val m_coreprCubes = mutableListOf<CornerOrientReprCubes>()

    fun empty() : Boolean = m_coreprCubes.isEmpty()
    fun size() : Int = m_coreprCubes.count()

    fun cubeCount() : Int {
        var res = 0
        for(it in m_coreprCubes)
            res += it.size()
        return res
    }

    fun initOccur() {
        for(cerepr in m_coreprCubes)
            cerepr.initOccur()
    }

    fun cornerOrientCubesAt(cco : cubecorner_orients) : CornerOrientReprCubes {
        val idx = m_coreprCubes.binarySearchBy(cco) { it.getOrients() }
        return if( idx >= 0 ) m_coreprCubes[idx] else m_coreprCubesEmpty
    }

    fun cornerOrientCubesAdd(cco : cubecorner_orients) : CornerOrientReprCubes {
        var idx = m_coreprCubes.binarySearchBy(cco) { it.getOrients() }
        if( idx < 0 ) {
            idx = -1 - idx
            m_coreprCubes.subList(0, idx).add( CornerOrientReprCubes(cco) )
        }
        return m_coreprCubes[idx]
    }

    fun ccoCubesList() : Collection<CornerOrientReprCubes> = m_coreprCubes
}

/* A set of representative cubes reachable at specific depth.
 */
class CubesReprAtDepth(val m_reprPerms : CubecornerReprPerms) {
    val m_cornerPermReprCubes =
        List<CornerPermReprCubes>(m_reprPerms.reprPermCount()) { CornerPermReprCubes() }

    fun size() : Int = m_cornerPermReprCubes.size

    fun cubeCount() : Int {
        var res : Int = 0
        for(reprCube in m_cornerPermReprCubes)
            res += reprCube.cubeCount()
        return res
    }

    fun add(idx : Int) : CornerPermReprCubes {
        return m_cornerPermReprCubes[idx]
    }

    fun initOccur(idx : Int) {
        m_cornerPermReprCubes[idx].initOccur()
    }

    fun getAt(idx : Int) : CornerPermReprCubes {
        return m_cornerPermReprCubes[idx]
    }

    fun getFor(ccp : cubecorners_perm) : CornerPermReprCubes {
        val reprPermIdx : Int = m_reprPerms.getReprPermIdx(ccp)
        return m_cornerPermReprCubes[reprPermIdx]
    }

    fun ccpCubesList() : List<CornerPermReprCubes> = m_cornerPermReprCubes

    fun getPermAt(reprPermIdx : Int) : cubecorners_perm {
        return m_reprPerms.getPermForIdx(reprPermIdx)
    }
}

class CubesReprByDepth(useReverse : Boolean) {
    val m_reprPerms = CubecornerReprPerms(useReverse)
    // insert the solved cube at depth 0
    val m_cubesAtDepths = mutableListOf<CubesReprAtDepth>(CubesReprAtDepth(m_reprPerms))
    var m_availCount : Int = 1

    init {
        val cornerPermReprIdx : Int = m_reprPerms.getReprPermIdx(csolved.ccp)
        val ccpCubes : CornerPermReprCubes = m_cubesAtDepths[0].add(cornerPermReprIdx)
        val otransform = mutableListOf<EdgeReprCandidateTransform>()
        val ccoRepr : cubecorner_orients = m_reprPerms.getReprOrients(csolved.ccp,
                csolved.cco, otransform)
        val ccoCubes : CornerOrientReprCubes = ccpCubes.cornerOrientCubesAdd(ccoRepr)
        val ceRepr : cubeedges = m_reprPerms.getReprCubeedges(csolved.ce, otransform)
        ccoCubes.addCubes(listOf(ceRepr))
    }

    fun isUseReverse() : Boolean = m_reprPerms.isUseReverse()
    fun availCount() : Int = m_availCount
    fun incAvailCount() { ++m_availCount; }
    fun getAt(idx : Int) : CubesReprAtDepth {
        while( idx >= m_cubesAtDepths.size )
            m_cubesAtDepths.add(CubesReprAtDepth(m_reprPerms))
        return m_cubesAtDepths[idx]
    }

    fun getReprPermForIdx(reprPermIdx : Int) : cubecorners_perm {
        return m_reprPerms.getPermForIdx(reprPermIdx)
    }

    // the cube passed as parameter shall exist in the set
    // returns the list of moves separated by spaces
    fun getMoves(c : cube, movesRevp : Boolean = false) : String {
        var movesRev = movesRevp
        val rotateDirs = mutableListOf<Int>()
        var insertPos = 0
        val crepr : cube = m_reprPerms.cubeRepresentative(c)
        val ccpReprIdx : Int = m_reprPerms.getReprPermIdx(crepr.ccp)
        var depth : Int = 0
        while( true ) {
            val ccpReprCubes : CornerPermReprCubes = m_cubesAtDepths[depth].getAt(ccpReprIdx)
            val ccoReprCubes : CornerOrientReprCubes = ccpReprCubes.cornerOrientCubesAt(crepr.cco)
            if( ccoReprCubes.containsCubeEdges(crepr.ce) )
                break
            ++depth
            if( depth >= m_availCount ) {
                println("getMoves: depth reached maximum, cube NOT FOUND")
                kotlin.system.exitProcess(1)
            }
        }
        var cc : cube = c
        while( depth-- > 0 ) {
            var cm : Int = 0
            val ccRev : cube = cc.reverse()
            var cc1 : cube = cc
            while( cm < rotate_dir.RCOUNT.ordinal ) {
                cc1 = cube.compose(cc, crotated[cm])
                var cc1repr : cube = m_reprPerms.cubeRepresentative(cc1)
                var ccpReprIdx : Int = m_reprPerms.getReprPermIdx(cc1repr.ccp)
                val ccpReprCubes : CornerPermReprCubes = m_cubesAtDepths[depth].getAt(ccpReprIdx)
                val ccoReprCubes : CornerOrientReprCubes = ccpReprCubes.cornerOrientCubesAt(cc1repr.cco)
                if( ccoReprCubes.containsCubeEdges(cc1repr.ce) )
                    break
                cc1 = cube.compose(ccRev, crotated[cm])
                cc1repr = m_reprPerms.cubeRepresentative(cc1)
                ccpReprIdx = m_reprPerms.getReprPermIdx(cc1repr.ccp)
                val ccpReprCubesRev : CornerPermReprCubes = m_cubesAtDepths[depth].getAt(ccpReprIdx)
                val ccoReprCubesRev : CornerOrientReprCubes = ccpReprCubesRev.cornerOrientCubesAt(cc1repr.cco)
                if( ccoReprCubesRev.containsCubeEdges(cc1repr.ce) ) {
                    movesRev = !movesRev
                    break
                }
                ++cm
            }
            if( cm == rotate_dir.RCOUNT.ordinal ) {
                println("getMoves: cube at depth $depth NOT FOUND")
                kotlin.system.exitProcess(1)
            }
            rotateDirs.subList(0, insertPos).add(if(movesRev) cm else rotateDirReverse(cm))
            if( movesRev )
                ++insertPos
            cc = cc1
        }
        var res : String = ""
        for(rotateDir in rotateDirs) {
            res += " "
            res += rotateDirName(rotateDir)
        }
        return res
    }

    /* Auxiliary function to fill the cube set. Adds cubes for
     * the specified representative index at the specified depth.
     * Before call the set should be filled up to depth-1.
     */
    fun addCubesForReprPerm(reprPermIdx : Int, depth : Int) : Int {
        val otransformNew = mutableListOf<EdgeReprCandidateTransform>()
        var cubeCount : Int = 0

        val ccpReprCubesC : CubesReprAtDepth = m_cubesAtDepths[depth-1]
        val ccpReprCubesNewP : CornerPermReprCubes? = if(depth == 1) null else
            m_cubesAtDepths[depth-2].getAt(reprPermIdx)
        val ccpReprCubesNewC : CornerPermReprCubes = ccpReprCubesC.getAt(reprPermIdx)
        val ccpReprCubesNewN : CornerPermReprCubes = m_cubesAtDepths[depth].add(reprPermIdx)
        val ccpNewRepr : cubecorners_perm = m_reprPerms.getPermForIdx(reprPermIdx)
        val ccpChecked = mutableSetOf<cubecorners_perm>()
        for(trrev in 0 ..< (if(m_reprPerms.isUseReverse()) 2 else 1)) {
            val ccpNewReprRev : cubecorners_perm = if(trrev != 0) ccpNewRepr.reverse() else ccpNewRepr
            for(symmetric in 0..1) {
                val ccpNewS : cubecorners_perm = if(symmetric != 0) ccpNewReprRev.symmetric() else ccpNewReprRev
                for(td in 0 ..< transform_dir.TCOUNT.ordinal) {
                    val ccpNew : cubecorners_perm = ccpNewS.transform(td)
                    if( !ccpChecked.contains(ccpNew) ) {
                        ccpChecked.add(ccpNew)
                        for(rd in 0 ..< rotate_dir.RCOUNT.ordinal) {
                            val rdRev : Int = rotateDirReverse(rd)
                            for(reversed in 0 ..< if(m_reprPerms.isUseReverse()) 2 else 1) {
                                val ccp : cubecorners_perm = if(reversed!=0)
                                    cubecorners_perm.compose(crotated[rdRev].ccp, ccpNew) else
                                    cubecorners_perm.compose(ccpNew, crotated[rdRev].ccp)
                                val ccpReprIdx : Int = m_reprPerms.getReprPermIdx(ccp)
                                if( m_reprPerms.getPermForIdx(ccpReprIdx).equals(ccp) ) {
                                    val cpermReprCubesC : CornerPermReprCubes = ccpReprCubesC.getAt(ccpReprIdx)
                                    for(corientReprCubesC in cpermReprCubesC.ccoCubesList()) {
                                        val cco : cubecorner_orients = corientReprCubesC.getOrients()
                                        val ccoNew : cubecorner_orients = if(reversed!=0)
                                            cubecorner_orients.compose(crotated[rd].cco, ccp, cco) else
                                            cubecorner_orients.compose(cco, crotated[rd].ccp, crotated[rd].cco)
                                        val ccoReprNew : cubecorner_orients = m_reprPerms.getComposedReprOrients(
                                                ccpNew, ccoNew, reversed!=0, crotated[rd].ce, otransformNew)
                                        val corientReprCubesNewP : CornerOrientReprCubes? =
                                            ccpReprCubesNewP?.cornerOrientCubesAt(ccoReprNew)
                                        val corientReprCubesNewC : CornerOrientReprCubes =
                                            ccpReprCubesNewC.cornerOrientCubesAt(ccoReprNew)
                                        val ceNewArr = mutableListOf<cubeedges>()
                                        for(edges in corientReprCubesC.edgeList()) {
                                            val ce = cubeedges(edges)
                                            val cenewRepr : cubeedges =
                                                CubecornerReprPerms.getComposedReprCubeedges(ce,
                                                reversed!=0, otransformNew)
                                            if( corientReprCubesNewP != null &&
                                                    corientReprCubesNewP.containsCubeEdges(cenewRepr) )
                                                continue
                                            if( corientReprCubesNewC.containsCubeEdges(cenewRepr) )
                                                continue
                                            ceNewArr.add(cenewRepr)
                                        }
                                        if( !ceNewArr.isEmpty() ) {
                                            val corientReprCubesNewN : CornerOrientReprCubes =
                                                ccpReprCubesNewN.cornerOrientCubesAdd(ccoReprNew)
                                            cubeCount += corientReprCubesNewN.addCubes(ceNewArr)
                                        }
                                    }
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
        val otransform = mutableListOf<EdgeReprCandidateTransform>()
        val ccpReprCubes : CornerPermReprCubes = m_cubesAtDepths[depth].getAt(reprPermIdx)
        val ccp : cubecorners_perm = m_reprPerms.getPermForIdx(reprPermIdx)
        if( !ccpReprCubes.empty() ) {
            val ccpSearch : cubecorners_perm = if(reversed)
                cubecorners_perm.compose(cSearchT.ccp, ccp) else
                cubecorners_perm.compose(ccp, cSearchT.ccp)
            val ccpReprSearchCubes : CornerPermReprCubes = m_cubesAtDepths[depthMax].getFor(ccpSearch)
            if( ccpReprCubes.size() <= ccpReprSearchCubes.size() ||
                    !m_reprPerms.isSingleTransform(ccpSearch) )
            {
                for(ccoReprCubes in ccpReprCubes.ccoCubesList()) {
                    val cco : cubecorner_orients = ccoReprCubes.getOrients()
                    val ccoSearch : cubecorner_orients = if(reversed)
                        cubecorner_orients.compose(cSearchT.cco, ccp, cco) else
                        cubecorner_orients.compose(cco, cSearchT.ccp, cSearchT.cco)
                    val ccoSearchRepr : cubecorner_orients = m_reprPerms.getComposedReprOrients(
                            ccpSearch, ccoSearch, reversed, cSearchT.ce, otransform)
                    val ccoReprSearchCubes : CornerOrientReprCubes =
                        ccpReprSearchCubes.cornerOrientCubesAt(ccoSearchRepr)
                    if( ccoReprSearchCubes.empty() )
                        continue
                    val ce : cubeedges = CornerOrientReprCubes.findSolutionEdge(
                            ccoReprCubes, ccoReprSearchCubes, otransform, reversed)
                    if( !ce.isNil() ) {
                        val ceSearch : cubeedges = if(reversed)
                            cubeedges.compose(cSearchT.ce, ce) else
                            cubeedges.compose(ce, cSearchT.ce)
                        cSearch[0] = cube(ccp = ccpSearch, cco = ccoSearch, ce = ceSearch)
                        c[0] = cube(ccp = ccp, cco = cco, ce = ce)
                        return true
                    }
                }
            }else{
                for(ccoReprSearchCubes in ccpReprSearchCubes.ccoCubesList()) {
                    val ccoSearchRepr : cubecorner_orients = ccoReprSearchCubes.getOrients()
                    val cco : cubecorner_orients = m_reprPerms.getOrientsForComposedRepr(
                            ccpSearch, ccoSearchRepr, reversed, cSearchT, otransform)
                    val ccoReprCubes : CornerOrientReprCubes = ccpReprCubes.cornerOrientCubesAt(cco)
                    if( ccoReprCubes.empty() )
                        continue
                    val ce : cubeedges = CornerOrientReprCubes.findSolutionEdge(
                            ccoReprCubes, ccoReprSearchCubes, otransform, reversed)
                    if( !ce.isNil() ) {
                        val ccoSearch : cubecorner_orients = if(reversed)
                            cubecorner_orients.compose(cSearchT.cco, ccp, cco) else
                            cubecorner_orients.compose(cco, cSearchT.ccp, cSearchT.cco)
                        val ceSearch : cubeedges = if(reversed)
                            cubeedges.compose(cSearchT.ce, ce) else
                            cubeedges.compose(ce, cSearchT.ce)
                        cSearch[0] = cube(ccp = ccpSearch, cco = ccoSearch, ce = ceSearch)
                        c[0] = cube(ccp = ccp, cco = cco, ce = ce)
                        return true
                    }
                }
            }
        }
        return false
    }
}
