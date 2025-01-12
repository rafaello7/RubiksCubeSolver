package cubesrv

/* Representative cube is a cube chosen from set of similar cubes.
 * Two cubes are similar when one cube can be converted into another
 * one by a transformation (colors switch) after cube rotation, mirroring
 * (a symmetric cube), optionally combined with reversing of moves
 * sequence used to get the cube.
 *
 * To get a representative cube from the set of similar cubes, a subset
 * of cubes with specific corners permutation is chosen first. Further
 * choice is based on the cube corner orientations. When a few cubes
 * have the same corners permutation and orientations, the choice is
 * based on the cube edges.
 *
 * There are 984 or 654 distinct corner permutations, depend on use of moves
 * reversing.
 */

class EdgeReprCandidateTransform(val transformedIdx : Int, val reversed : Boolean,
    val symmetric : Boolean, var ceTrans : cubeedges = csolved.ce)

class CubecornerReprPerms(val m_useReverse : Boolean) {
    private class ReprCandidateTransform(val reversed : Boolean,
        val symmetric : Boolean, val transformIdx : Int)

    private class CubecornerPermToRepr {
        var reprIdx = -1        // index in m_reprPerms
        val transform = mutableListOf<ReprCandidateTransform>()
    }

    private val m_reprPerms = mutableListOf<cubecorners_perm>()
    private val m_permToRepr = Array<CubecornerPermToRepr>(40320, { CubecornerPermToRepr() })

    init {
        //m_reprPerms.reserve(m_useReverse ? 654 : 984)
        for(pidx in 0 ..< 40320) {
            val perm = cubecorners_perm.fromPermIdx(pidx)
            var permRepr : cubecorners_perm = cubecorners_perm()
            var transform = mutableListOf<ReprCandidateTransform>()
            for(reversed in 0 ..< if(m_useReverse) 2 else 1 ) {
                val permr = if(reversed != 0) perm.reverse() else perm
                for(symmetric in 0..1) {
                    val permchk = if(symmetric != 0) permr.symmetric() else permr
                    for(td in 0 ..< transform_dir.TCOUNT.ordinal) {
                        val cand : cubecorners_perm = permchk.transform(td)
                        if( td+reversed+symmetric == 0 || cand < permRepr ) {
                            permRepr = cand
                            transform = mutableListOf(ReprCandidateTransform(reversed = reversed != 0,
                                    symmetric = symmetric != 0, transformIdx = td ))
                        }else if( cand.equals(permRepr) ) {
                            transform.add(ReprCandidateTransform(reversed = reversed != 0,
                                    symmetric = symmetric != 0, transformIdx = td ))
                        }
                    }
                }
            }
            val permToReprRepr : CubecornerPermToRepr = m_permToRepr[permRepr.getPermIdx()]
            if( permToReprRepr.reprIdx < 0 ) {
                permToReprRepr.reprIdx = m_reprPerms.count()
                m_reprPerms.add(permRepr)
            }
            val permToRepr : CubecornerPermToRepr = m_permToRepr[perm.getPermIdx()]
            permToRepr.reprIdx = permToReprRepr.reprIdx
            permToRepr.transform.clear()
            permToRepr.transform.addAll(transform)
        }
        println("repr size=${m_reprPerms.count()}")
    }

    fun isUseReverse() : Boolean = m_useReverse
    fun reprPermCount() : Int  = m_reprPerms.count()

    /* Returns corners permutation of representative cube for a cube
     * given by the corners permutation.
     */
    fun getReprPerm(ccp : cubecorners_perm) : cubecorners_perm {
        val reprPermIdx = m_permToRepr[ccp.getPermIdx()].reprIdx
        return m_reprPerms[reprPermIdx]
    }

    /* Returns a number in range 0..653 or 0..983, depend on use of moves
     * reversing.
     */
    fun getReprPermIdx(ccp : cubecorners_perm) : Int {
        return m_permToRepr[ccp.getPermIdx()].reprIdx
    }

    fun getPermForIdx(reprPermIdx : Int) : cubecorners_perm {
        return m_reprPerms[reprPermIdx]
    }

    /* Returns true when the corners permutation determines uniquely
     * the transformation needed to convert a cube having the permutation to
     * representative one.
     */
    fun isSingleTransform(ccp : cubecorners_perm) : Boolean {
        return m_permToRepr[ccp.getPermIdx()].transform.size == 1
    }

    /* Returns corner orientations of representative cube for a cube given
     * by corners permutation and orientations. The transform is an output
     * parameter: provides a list of transformations converting the given
     * corner permutation and orientations to the representative corner
     * orientations. All the transformations give also identical
     * representative corners permutation.
     * The transform can be passed later to cubeedgesRepresentative
     * to get the edges of representative cube.
     */
    fun getReprOrients(
            ccp : cubecorners_perm, cco : cubecorner_orients,
            transform : MutableList<EdgeReprCandidateTransform>) : cubecorner_orients
    {
        val permToRepr : CubecornerPermToRepr = m_permToRepr[ccp.getPermIdx()]
        val ccpsymm : cubecorners_perm = ccp.symmetric()
        val ccprev : cubecorners_perm = ccp.reverse()
        val ccprevsymm : cubecorners_perm = ccprev.symmetric()
        var orepr : cubecorner_orients = cubecorner_orients()
        val ccosymm : cubecorner_orients = cco.symmetric()
        val ccorev : cubecorner_orients = cco.reverse(ccp)
        val ccorevsymm : cubecorner_orients = ccorev.symmetric()
        var isInit : Boolean = false
        transform.clear()
        for(rct in permToRepr.transform) {
            val ocand : cubecorner_orients =
                if( rct.reversed ) {
                    if( rct.symmetric ) {
                        ccorevsymm.transform(ccprevsymm, rct.transformIdx)
                    }else{
                        ccorev.transform(ccprev, rct.transformIdx)
                    }
                }else{
                    if( rct.symmetric ) {
                        ccosymm.transform(ccpsymm, rct.transformIdx)
                    }else{
                        cco.transform(ccp, rct.transformIdx)
                    }
                }
            if( !isInit || ocand < orepr ) {
                orepr = ocand
                transform.clear()
                transform.add(EdgeReprCandidateTransform(transformedIdx = rct.transformIdx,
                        reversed = rct.reversed, symmetric = rct.symmetric, ceTrans = csolved.ce))
                isInit = true
            }else if( isInit && ocand == orepr )
                transform.add(EdgeReprCandidateTransform(transformedIdx = rct.transformIdx,
                        reversed = rct.reversed, symmetric = rct.symmetric, ceTrans = csolved.ce))
        }
        return orepr
    }

    /* Gets the representative orients of cco.
     * Assumes the ccp,cco are cubecorners of a cube compose of two cubes, c1 ⊙  c2
     *
     * Parameters:
     *    ccp, cco      - cubecorners of the c1.cc composed with c2.cc
     *    reverse       - whether the ccp,cco is reversed
     *    ce2           - c2.ce
     *    transform     - output list to pass later to cubeedgesComposedRepresentative()
     *                    along with c1.ce, to get representative cubeedges of
     *                    c1.ce ⊙  ce2
     */
    fun getComposedReprOrients(
            ccp : cubecorners_perm, cco : cubecorner_orients, reverse : Boolean,
            ce2 : cubeedges, transform : MutableList<EdgeReprCandidateTransform>) : cubecorner_orients
    {
        val permToRepr = m_permToRepr[ccp.getPermIdx()]
        val ccpsymm : cubecorners_perm = ccp.symmetric()
        val ccprev : cubecorners_perm = ccp.reverse()
        val ccprevsymm : cubecorners_perm = ccprev.symmetric()
        var orepr : cubecorner_orients = cubecorner_orients()
        val ccosymm : cubecorner_orients = cco.symmetric()
        val ccorev : cubecorner_orients = cco.reverse(ccp)
        val ccorevsymm : cubecorner_orients = ccorev.symmetric()
        var isInit : Boolean = false
        transform.clear()
        for(rct in permToRepr.transform) {
            val ocand : cubecorner_orients =
                if( rct.reversed ) {
                    if( rct.symmetric ) {
                        ccorevsymm.transform(ccprevsymm, rct.transformIdx)
                    }else{
                        ccorev.transform(ccprev, rct.transformIdx)
                    }
                }else{
                    if( rct.symmetric ) {
                        ccosymm.transform(ccpsymm, rct.transformIdx)
                    }else{
                        cco.transform(ccp, rct.transformIdx)
                    }
                }
            if( !isInit || ocand < orepr ) {
                orepr = ocand
                transform.clear()
                transform.add(EdgeReprCandidateTransform(transformedIdx = rct.transformIdx,
                        reversed = rct.reversed, symmetric = rct.symmetric ))
                isInit = true
            }else if( isInit && ocand == orepr )
                transform.add(EdgeReprCandidateTransform(transformedIdx = rct.transformIdx,
                        reversed = rct.reversed, symmetric = rct.symmetric))
        }
        for(erct in transform) {
            val ce2symm = if(erct.symmetric) ce2.symmetric() else ce2
            if( reverse ) {
                if( erct.reversed ) {
                    // transform((ce1 rev) ⊙  (ce2 rev)) = cetrans ⊙  (ce1 rev) ⊙  (ce2 rev) ⊙  cetransRev
                    // ceTrans = (ce2 rev) ⊙  cetransRev
                    erct.ceTrans = cubeedges.compose(ce2symm.reverse(),
                            ctransformed[transformReverse(erct.transformedIdx)].ce)
                }else{
                    // transform(ce2 ⊙  ce1) = cetrans ⊙  ce2 ⊙  ce1 ⊙  cetransRev
                    // ceTrans = cetrans ⊙  ce2
                    erct.ceTrans = cubeedges.compose(
                            ctransformed[erct.transformedIdx].ce, ce2symm)
                }
            }else{
                if( erct.reversed ) {
                    // transform((ce2 rev) ⊙  (ce1 rev)) = cetrans ⊙  (ce2 rev) ⊙  (ce1 rev) ⊙  cetransRev
                    // ceTrans = cetrans ⊙  (ce2 rev)
                    erct.ceTrans = cubeedges.compose(
                            ctransformed[erct.transformedIdx].ce, ce2symm.reverse())
                }else{
                    // transform(ce1 ⊙  ce2) = cetrans ⊙  ce1 ⊙  ce2 ⊙  cetransRev
                    // ceTrans = ce2 ⊙  cetransRev
                    erct.ceTrans = cubeedges.compose(ce2symm,
                            ctransformed[transformReverse(erct.transformedIdx)].ce)
                }
            }
        }
        return orepr
    }

    fun getOrientsForComposedRepr(
            ccpSearch : cubecorners_perm, ccoSearchRepr : cubecorner_orients,
            reversed : Boolean, cSearchT : cube,
            transform : MutableList<EdgeReprCandidateTransform>) : cubecorner_orients
    {
        val permToRepr = m_permToRepr[ccpSearch.getPermIdx()]
        val ccpSearchRepr : cubecorners_perm = m_reprPerms[permToRepr.reprIdx]
        val cSearchTrev : cube = cSearchT.reverse()

        transform.clear()
        val rct : ReprCandidateTransform = permToRepr.transform.first()
        // sequence was: ccpSearch reverse, then symmetric, then transform
        val transformIdxRev : Int = transformReverse(rct.transformIdx)
        //cubecorners_perm ccpSearchRevSymm = ccpSearchRepr.transform(transformIdxRev)
        val ccoSearchRevSymm : cubecorner_orients = ccoSearchRepr.transform(ccpSearchRepr, transformIdxRev)
        //cubecorners_perm ccpSearchRev = rct.symmetric ? ccpSearchRevSymm.symmetric() : ccpSearchRevSymm
        val ccoSearchRev : cubecorner_orients = if(rct.symmetric) ccoSearchRevSymm.symmetric() else ccoSearchRevSymm
        //cubecorners_perm ccpSearch = rct.reversed ? ccpSearchRev.reverse() : ccpSearchRev
        val ccoSearch : cubecorner_orients = if(rct.reversed) ccoSearchRev.reverse(ccpSearch.reverse()) else ccoSearchRev
        // ccpSearch is: ccpSearch = reversed ? (cSearchT.ccp ⊙  ccp) : (ccp ⊙  cSearchT.ccp)
        val cco : cubecorner_orients = if(reversed)
            cubecorner_orients.compose(cSearchTrev.cco, ccpSearch, ccoSearch) else
            cubecorner_orients.compose(ccoSearch, cSearchTrev.ccp, cSearchTrev.cco)
        transform.add(EdgeReprCandidateTransform(transformedIdx = rct.transformIdx,
                reversed = rct.reversed, symmetric = rct.symmetric ))
        val erct : EdgeReprCandidateTransform = transform.first()
        val ce2symm : cubeedges = if(erct.symmetric) cSearchT.ce.symmetric() else cSearchT.ce
        if( reversed ) {
            if( erct.reversed ) {
                // transform((ce1 rev) ⊙  (ce2 rev)) = cetrans ⊙  (ce1 rev) ⊙  (ce2 rev) ⊙  cetransRev
                // ceTrans = (ce2 rev) ⊙  cetransRev
                erct.ceTrans = cubeedges.compose(ce2symm.reverse(),
                        ctransformed[transformReverse(erct.transformedIdx)].ce)
            }else{
                // transform(ce2 ⊙  ce1) = cetrans ⊙  ce2 ⊙  ce1 ⊙  cetransRev
                // ceTrans = ctrans ⊙  ce2
                erct.ceTrans = cubeedges.compose(
                        ctransformed[erct.transformedIdx].ce, ce2symm)
            }
        }else{
            if( erct.reversed ) {
                // transform((ce2 rev) ⊙  (ce1 rev)) = cetrans ⊙  (ce2 rev) ⊙  (ce1 rev) ⊙  cetransRev
                // ceTrans = cetrans ⊙  (ce2 rev)
                erct.ceTrans = cubeedges.compose(
                        ctransformed[erct.transformedIdx].ce, ce2symm.reverse())
            }else{
                // transform(ce1 ⊙  ce2) = cetrans ⊙  ce1 ⊙  ce2 ⊙  cetransRev
                // ceTrans = ce2 ⊙  cetransRev
                erct.ceTrans = cubeedges.compose(ce2symm,
                        ctransformed[transformReverse(erct.transformedIdx)].ce)
            }
        }
        return cco
    }

    fun getReprCubeedges(ce : cubeedges,
            transform : List<EdgeReprCandidateTransform>) : cubeedges
    {
        var cerepr = cubeedges()
        if( transform.count() == 1 ) {
            val erct = transform.first()
            var cechk : cubeedges = if(erct.reversed) ce.reverse() else ce
            if( erct.symmetric )
                cechk = cechk.symmetric()
            cerepr = cechk.transform(erct.transformedIdx)
        }else{
            val cesymm : cubeedges = ce.symmetric()
            val cerev : cubeedges = ce.reverse()
            val cerevsymm : cubeedges = cerev.symmetric()
            var isInit : Boolean = false
            for(erct in transform) {
                val cand : cubeedges =
                    if( erct.reversed ) {
                        if( erct.symmetric ) {
                            cerevsymm.transform(erct.transformedIdx)
                        }else{
                            cerev.transform(erct.transformedIdx)
                        }
                    }else{
                        if( erct.symmetric ) {
                            cesymm.transform(erct.transformedIdx)
                        }else{
                            ce.transform(erct.transformedIdx)
                        }
                    }
                if( !isInit || cand < cerepr )
                    cerepr = cand
                isInit = true
            }
        }
        return cerepr
    }

    companion object {
        fun getComposedReprCubeedges(
                ce : cubeedges, reverse : Boolean,
                transform : List<EdgeReprCandidateTransform>) : cubeedges
        {
            var cerepr : cubeedges = cubeedges()
            if( transform.count() == 1 ) {
                val erct = transform.first()
                val cesymm : cubeedges = if(erct.symmetric) ce.symmetric() else ce
                if( reverse ) {
                    if( erct.reversed ) {
                        // erct.ceTrans = (ce2 rev) ⊙  cetransRev
                        // transform((ce1 rev) ⊙  (ce2 rev)) = cetrans ⊙  (ce1 rev) ⊙  (ce2 rev) ⊙  cetransRev
                        //      = cetrans ⊙  (ce1 rev) ⊙  erct.ceTrans
                        cerepr = cubeedges.compose3revmid(ctransformed[erct.transformedIdx].ce,
                                cesymm, erct.ceTrans)
                    }else{
                        // erct.ceTrans = ctrans ⊙  ce2
                        // transform(ce2 ⊙  ce1) = cetrans ⊙  ce2 ⊙  ce1 ⊙  cetransRev
                        //      = erct.ceTrans ⊙  ce1 ⊙  cetransRev
                        cerepr = cubeedges.compose3(erct.ceTrans, cesymm,
                                ctransformed[transformReverse(erct.transformedIdx)].ce)
                    }
                }else{
                    if( erct.reversed ) {
                        // erct.ceTrans = cetrans ⊙  (ce2 rev)
                        // transform((ce2 rev) ⊙  (ce1 rev)) = cetrans ⊙  (ce2 rev) ⊙  (ce1 rev) ⊙  cetransRev
                        //      = erct.ceTrans ⊙  (ce1 rev) ⊙  cetransRev
                        cerepr = cubeedges.compose3revmid(erct.ceTrans, cesymm,
                                ctransformed[transformReverse(erct.transformedIdx)].ce)
                    }else{
                        // erct.ceTrans = ce2 ⊙  cetransRev
                        // transform(ce1 ⊙  ce2) = cetrans ⊙  ce1 ⊙  ce2 ⊙  cetransRev
                        //      = cetrans ⊙  ce1 ⊙  erct.ceTrans
                        cerepr = cubeedges.compose3(ctransformed[erct.transformedIdx].ce,
                                cesymm, erct.ceTrans)
                    }
                }
            }else{
                var isInit : Boolean = false
                for(erct in transform) {
                    val cesymm : cubeedges = if(erct.symmetric) ce.symmetric() else ce
                    val cand : cubeedges =
                        if( reverse ) {
                            if( erct.reversed ) {
                                cubeedges.compose3revmid(ctransformed[erct.transformedIdx].ce,
                                        cesymm, erct.ceTrans)
                            }else{
                                cubeedges.compose3(erct.ceTrans, cesymm,
                                        ctransformed[transformReverse(erct.transformedIdx)].ce)
                            }
                        }else{
                            if( erct.reversed ) {
                                cubeedges.compose3revmid(erct.ceTrans, cesymm,
                                        ctransformed[transformReverse(erct.transformedIdx)].ce)
                            }else{
                                cubeedges.compose3(ctransformed[erct.transformedIdx].ce,
                                        cesymm, erct.ceTrans)
                            }
                        }

                    if( !isInit || cand < cerepr )
                        cerepr = cand
                    isInit = true
                }
            }
            return cerepr
        }
    }

    fun cubeRepresentative(c : cube) : cube {
        val transform = mutableListOf<EdgeReprCandidateTransform>()
        val ccpRepr : cubecorners_perm = getReprPerm(c.ccp)
        val ccoRepr : cubecorner_orients = getReprOrients(c.ccp, c.cco, transform)
        val ceRepr : cubeedges = getReprCubeedges(c.ce, transform)
        return cube(ccp = ccpRepr, cco = ccoRepr, ce = ceRepr)
    }
}
