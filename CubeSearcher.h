#ifndef CUBESEARCHER_H
#define CUBESEARCHER_H

#include "CubesReprByDepthAdd.h"
#include "BGCubesReprByDepthAdd.h"
#include "CubeCosetsAdd.h"
#include "Responder.h"

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


#endif // CUBESEARCHER_H
