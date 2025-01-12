package cubesrv

val RCOUNTBG = 10
val TCOUNTBG = 8

val BGSpaceRotations = arrayOf(
    rotate_dir.ORANGE180, rotate_dir.RED180, rotate_dir.YELLOW180, rotate_dir.WHITE180,
    rotate_dir.GREENCW, rotate_dir.GREEN180, rotate_dir.GREENCCW,
    rotate_dir.BLUECW, rotate_dir.BLUE180, rotate_dir.BLUECCW
)

val BGSpaceTransforms = arrayOf(
    transform_dir.TD_0, transform_dir.TD_BG_CW, transform_dir.TD_BG_180, transform_dir.TD_BG_CCW,
    transform_dir.TD_YW_180, transform_dir.TD_OR_180, transform_dir.TD_E4_7, transform_dir.TD_E5_6
)

class BGCubecornerReprPerms(val m_useReverse : Boolean) {
    private class ReprCandidateTransform(val reversed : Boolean,
        val symmetric : Boolean, val transformIdx : Int)

    private class CubecornerPermToRepr {
        var reprIdx = -1        // index in m_reprPerms
        val transform = mutableListOf<ReprCandidateTransform>()
    }

    private val m_reprPerms = mutableListOf<cubecorners_perm>()
    private val m_permToRepr = Array<CubecornerPermToRepr>(40320, { CubecornerPermToRepr() })

    init {
        //m_reprPerms.reserve(useReverse ? 1672 : 2768)
        for(pidx in 0 ..< 40320) {
            val perm = cubecorners_perm.fromPermIdx(pidx)
            var permRepr : cubecorners_perm = cubecorners_perm()
            var transform = mutableListOf<ReprCandidateTransform>()
            for(reversed in 0 ..< if(m_useReverse) 2 else 1 ) {
                val permr = if(reversed != 0) perm.reverse() else perm
                for(symmetric in 0..1) {
                    val permchk = if(symmetric != 0) permr.symmetric() else permr
                    for(tdidx in 0 ..< TCOUNTBG) {
                        val td : Int = BGSpaceTransforms[tdidx].ordinal
                        val cand : cubecorners_perm = permchk.transform(td)
                        if( tdidx == 0 && reversed == 0  && symmetric == 0 || cand < permRepr ) {
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
                permToReprRepr.reprIdx = m_reprPerms.size
                m_reprPerms.add(permRepr)
            }
            val permToRepr : CubecornerPermToRepr = m_permToRepr[perm.getPermIdx()]
            permToRepr.reprIdx = permToReprRepr.reprIdx
            permToRepr.transform.clear()
            permToRepr.transform.addAll(transform)
        }
        println("bg repr size=${m_reprPerms.count()}")
    }

    fun isUseReverse() : Boolean = m_useReverse
    fun reprPermCount() : Int  = m_reprPerms.count()

    /* Returns corners permutation of representative cube for a cube
     * given by the corners permutation.
     */
    fun getReprPerm(ccp : cubecorners_perm) : cubecorners_perm {
        val permReprIdx = m_permToRepr[ccp.getPermIdx()].reprIdx
        return m_reprPerms[permReprIdx]
    }

    /* Returns a number in range 0..1671 or 0..2767, depend on use of moves
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

    fun getReprCubeedges(ccp : cubecorners_perm, ce : cubeedges) : cubeedges {
        val permToRepr : CubecornerPermToRepr = m_permToRepr[ccp.getPermIdx()]
        val cesymm = ce.symmetric()
        val cerev = ce.reverse()
        val cerevsymm = cerev.symmetric()
        var erepr = cubeedges()
        var isInit : Boolean = false
        for(rct in permToRepr.transform) {
            val ecand : cubeedges =
                if( rct.reversed ) {
                    if( rct.symmetric ) {
                        cerevsymm.transform(rct.transformIdx)
                    }else{
                        cerev.transform(rct.transformIdx)
                    }
                }else{
                    if( rct.symmetric ) {
                        cesymm.transform(rct.transformIdx)
                    }else{
                        ce.transform(rct.transformIdx)
                    }
                }
            if( !isInit || ecand < erepr ) {
                erepr = ecand
                isInit = true
            }
        }
        return erepr
    }

    fun cubeRepresentative(c : cube) : cube {
        val ccpRepr : cubecorners_perm = getReprPerm(c.ccp)
        val ceRepr : cubeedges = getReprCubeedges(c.ccp, c.ce)
        return cube(ccp = ccpRepr, cco = csolved.cco, ce = ceRepr)
    }

    fun getCubeedgesForRepresentative(ccpSearch : cubecorners_perm,
            ceSearchRepr : cubeedges) : cubeedges
    {
        val permToRepr : CubecornerPermToRepr = m_permToRepr[ccpSearch.getPermIdx()]
        val rct : ReprCandidateTransform = permToRepr.transform.first()
        // sequence was: ccpSearch reverse, then symmetric, then transform
        val transformIdxRev : Int = transformReverse(rct.transformIdx)
        val ceSearchRevSymm : cubeedges = ceSearchRepr.transform(transformIdxRev)
        val ceSearchRev : cubeedges = if(rct.symmetric) ceSearchRevSymm.symmetric() else ceSearchRevSymm
        val ceSearch : cubeedges = if(rct.reversed) ceSearchRev.reverse() else ceSearchRev
        return ceSearch
    }
}

