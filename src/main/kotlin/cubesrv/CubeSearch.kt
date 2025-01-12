package cubesrv

class CubeSearcher(val m_depthMax : Int, useReverse : Boolean) {
    private val m_cubesReprByDepthAdd = CubesReprByDepthAdd(useReverse)
    private val m_bgcubesReprByDepthAdd = BGCubesReprByDepthAdd(useReverse)
    private val m_cubeCosetsAdd = CubeCosetsAdd()

    fun fillCubes(responder : Responder) {
        m_cubesReprByDepthAdd.getReprCubes(m_depthMax, responder)
    }

    fun searchMoves(csearch : cube, mode : String, responder : Responder) {
        val rev = if(m_cubesReprByDepthAdd.isUseReverse()) " rev" else ""
        responder.message("setup: depth $m_depthMax$rev")
        when( mode ) {
            "q" -> searchMovesQuickCatchFirst(m_cubesReprByDepthAdd, m_bgcubesReprByDepthAdd,
                        m_cubeCosetsAdd, csearch, responder)
            "m" -> searchMovesQuickMulti(m_cubesReprByDepthAdd, m_bgcubesReprByDepthAdd,
                        m_cubeCosetsAdd, csearch, responder)
            else -> searchMovesOptimal(m_cubesReprByDepthAdd, csearch, m_depthMax, responder)
        }
    }
}

