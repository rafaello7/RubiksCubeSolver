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
    CubeCosets m_cubeCosets;
public:
    CubeCosetsAdd();

    /* Returns set of cosets filled up to at least the specified depth (inclusive).
     * The function triggers filling procedure when the depth is requested first time.
     * When the filling procedure is canceled, NULL is returned.
     */
    const CubeCosets *getBGcosets(CubesReprByDepthAdd&, unsigned depth, Responder&);
};

#endif // CUBECOSETSADD_H
