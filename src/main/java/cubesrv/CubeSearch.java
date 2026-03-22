package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.CubesAdd.*;
import static cubesrv.CubesAddBG.*;
import cubesrv.CubeCosetsAdd;

public class CubeSearch {

    public static class CubeSearcher {
        private final int m_depthMax;
        private final CubesReprByDepthAdd m_cubesReprByDepthAdd;
        private final BGCubesReprByDepthAdd m_bgcubesReprByDepthAdd;
        private final CubeCosetsAdd m_cubeCosetsAdd;

        public CubeSearcher(int depthMax, boolean useReverse) {
            m_depthMax = depthMax;
            m_cubesReprByDepthAdd   = new CubesReprByDepthAdd(useReverse);
            m_bgcubesReprByDepthAdd = new BGCubesReprByDepthAdd(useReverse);
            m_cubeCosetsAdd         = new CubeCosetsAdd();
        }

        public void fillCubes(Responder responder) {
            m_cubesReprByDepthAdd.getReprCubes(m_depthMax, responder);
        }

        public void searchMoves(cube csearch, String mode, Responder responder) {
            String rev = m_cubesReprByDepthAdd.isUseReverse() ? " rev" : "";
            responder.message("setup: depth " + m_depthMax + rev);
            switch (mode) {
                case "q" -> SearchQuick.searchMovesQuickCatchFirst(m_cubesReprByDepthAdd,
                        m_bgcubesReprByDepthAdd, m_cubeCosetsAdd, csearch, responder);
                case "m" -> SearchQuick.searchMovesQuickMulti(m_cubesReprByDepthAdd,
                        m_bgcubesReprByDepthAdd, m_cubeCosetsAdd, csearch, responder);
                default  -> SearchOptimal.searchMovesOptimal(m_cubesReprByDepthAdd,
                        csearch, m_depthMax, responder);
            }
        }
    }
}

