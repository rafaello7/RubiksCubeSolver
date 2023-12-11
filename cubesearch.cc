#include "cubesearch.h"
#include "searchoptimal.h"
#include "searchquick.h"
#include <sstream>

CubeSearcher::CubeSearcher(unsigned depthMax, bool useReverse)
    : m_cubesReprByDepthAdd(useReverse), m_depthMax(depthMax)
{
}

void CubeSearcher::fillCubes(Responder &responder)
{
    m_cubesReprByDepthAdd.getReprCubes(m_depthMax, responder);
}

void CubeSearcher::searchMoves(
        const cube &csearch, char mode, Responder &responder)
{
    responder.message("setup: depth %d", m_depthMax);
    switch( mode ) {
    case 'q':
        searchMovesQuickCatchFirst(m_cubesReprByDepthAdd, csearch, responder);
        break;
    case 'm':
        searchMovesQuickMulti(m_cubesReprByDepthAdd, csearch, responder);
        break;
    default:
        searchMovesOptimal(m_cubesReprByDepthAdd, csearch, m_depthMax, responder);
        break;
    }
}

