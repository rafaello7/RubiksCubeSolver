#ifndef CUBECOSETSADD_H
#define CUBECOSETSADD_H

#include "cubecosets.h"
#include "cubesadd.h"
#include "responder.h"

enum {
    TWOPHASE_DEPTH1_CATCHFIRST_MAX = 5u,
    TWOPHASE_DEPTH1_MULTI_MAX = 6u,
};

class CubeCosetsAdd {
    CubeCosets m_spaceReprCubes;
public:
    CubeCosetsAdd();
    const CubeCosets *getBGcosets(CubesReprByDepthAdd&, unsigned depth, Responder&);
};

#endif // CUBECOSETSADD_H
