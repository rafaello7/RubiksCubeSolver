package cubesrv

import java.util.concurrent.atomic.AtomicInteger

val TWOPHASE_DEPTH1_CATCHFIRST_MAX = 5
val TWOPHASE_DEPTH1_MULTI_MAX = 6

private fun addBGcosetsT(threadNo : Int,
        cubesReprByDepth : CubesReprByDepth, bgCosets : CubeCosets,
        depth : Int, reprCubeCount : AtomicInteger)
{
    var reprCubeCountT : Int = 0
    val ccReprCubesC : CubesReprAtDepth = cubesReprByDepth.getAt(depth)
    for(idx in 0 ..< ccReprCubesC.ccpCubesList().size) {
        val ccpCubes = ccReprCubesC.ccpCubesList()[idx]
        val ccp = ccReprCubesC.getPermAt(idx)
        for(ccoCubes in ccpCubes.ccoCubesList()) {
            val cco : cubecorner_orients = ccoCubes.getOrients()
            for(reversed in 0 ..< (if(cubesReprByDepth.isUseReverse()) 2 else 1)) {
                val ccprev : cubecorners_perm = if(reversed!=0) ccp.reverse() else ccp
                val ccorev : cubecorner_orients = if(reversed!=0) cco.reverse(ccp) else cco
                for(symmetric in 0..1) {
                    val ccprevsymm : cubecorners_perm = if(symmetric!=0) ccprev.symmetric() else ccprev
                    val ccorevsymm : cubecorner_orients = if(symmetric!=0) ccorev.symmetric() else ccorev
                    for(td in 0 ..< transform_dir.TCOUNT.ordinal) {
                        val ccpT : cubecorners_perm = ccprevsymm.transform(td)
                        val ccoT : cubecorner_orients = ccorevsymm.transform(ccprevsymm, td)
                        val ccoReprBG : cubecorner_orients = ccoT.representativeBG(ccpT)
                        val reprCOrientIdx : Int = ccoReprBG.getOrientIdx()

                        if( reprCOrientIdx % THREAD_COUNT == threadNo ) {
                            for(edges in ccoCubes.edgeList()) {
                                val ce = cubeedges(edges)
                                val cerev : cubeedges = if(reversed!=0) ce.reverse() else ce
                                val cerevsymm : cubeedges = if(symmetric!=0) cerev.symmetric() else cerev
                                val ceT : cubeedges = cerevsymm.transform(td)
                                val ceReprBG : cubeedges = ceT.representativeBG()
                                val c : cube = cube(ccp = ccpT, cco = ccoT, ce = ceT)
                                if( bgCosets.getAt(depth).addCube(reprCOrientIdx, ceReprBG, c) )
                                    ++reprCubeCountT
                            }
                        }
                    }
                }
            }
        }
    }
    reprCubeCount.addAndGet(reprCubeCountT)
}

private fun addBGcosets(cubesReprByDepth : CubesReprByDepth,
        bgCosets : CubeCosets, requestedDepth : Int, responder : Responder)
{
    while( bgCosets.availCount() <= requestedDepth ) {
        val depth : Int = bgCosets.availCount()
        val reprCubeCount = AtomicInteger(0)
        runInThreadPool() { threadNo ->
            addBGcosetsT(threadNo, cubesReprByDepth, bgCosets, depth, reprCubeCount)
        }
        responder.message("depth $depth space repr cubes=${reprCubeCount.get()}")
        bgCosets.incAvailCount()
    }
}

class CubeCosetsAdd {
    val m_cubeCosets = CubeCosets(maxOf(TWOPHASE_DEPTH1_CATCHFIRST_MAX, TWOPHASE_DEPTH1_MULTI_MAX)+1)

    /* Returns set of cosets filled up to at least the specified depth (inclusive).
     * The function triggers filling procedure when the depth is requested first time.
     * When the filling procedure is canceled, NULL is returned.
     */
    fun getBGcosets(cubesReprByDepthAdd : CubesReprByDepthAdd,
            depth : Int, responder : Responder) : CubeCosets?
    {
        if( m_cubeCosets.availCount() <= depth ) {
            val cubesReprByDepth : CubesReprByDepth? = cubesReprByDepthAdd.getReprCubes(depth, responder)
            if( cubesReprByDepth == null )
                return null
            addBGcosets(cubesReprByDepth, m_cubeCosets, depth, responder)
        }
        return m_cubeCosets
    }
}
