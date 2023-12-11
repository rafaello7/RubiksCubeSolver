#include "cubesearch.h"
#include "searchoptimal.h"
#include "searchquick.h"
#include <sstream>

CubeSearcher::CubeSearcher(unsigned depthMax, bool useReverse)
    : m_cubesReprByDepthAdd(useReverse), m_bgcubesReprByDepthAdd(useReverse),
    m_depthMax(depthMax)
{
}

void CubeSearcher::fillCubes(Responder &responder)
{
    m_cubesReprByDepthAdd.getReprCubes(m_depthMax, responder);
}

void CubeSearcher::searchMoves(
        const cube &csearch, char mode, Responder &responder)
{
    responder.message("setup: depth %d%s%s", m_depthMax,
            m_cubesReprByDepthAdd.isUseReverse() ? " rev" :"", ASM_SETUP);
    switch( mode ) {
    case 'q':
        searchMovesQuickCatchFirst(m_cubesReprByDepthAdd, m_bgcubesReprByDepthAdd,
                csearch, responder);
        break;
    case 'm':
        searchMovesQuickMulti(m_cubesReprByDepthAdd, m_bgcubesReprByDepthAdd,
                csearch, responder);
        break;
    default:
        searchMovesOptimal(m_cubesReprByDepthAdd, csearch, m_depthMax, responder);
        break;
    }
}

