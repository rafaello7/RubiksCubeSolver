#ifndef CUBESEARCH_H
#define CUBESEARCH_H

#include "cubesadd.h"
#include "cubesaddbg.h"
#include "cubecosetsadd.h"
#include "responder.h"

class CubeSearcher {
    CubesReprByDepthAdd m_cubesReprByDepthAdd;
    BGCubesReprByDepthAdd m_bgcubesReprByDepthAdd;
    CubeCosetsAdd m_cubeCosetsAdd;
    const unsigned m_depthMax;
public:
    CubeSearcher(unsigned depthMax, bool useReverse);
    void fillCubes(Responder&);
    void searchMoves(const cube &csearch, char mode, Responder&);
};


#endif // CUBESEARCH_H
