#ifndef CUBECOSETSADD_H
#define CUBECOSETSADD_H

#include "cubecosets.h"
#include "cubesadd.h"
#include "responder.h"

enum {
    TWOPHASE_DEPTH1_CATCHFIRST_MAX = 5u,
    TWOPHASE_DEPTH1_MULTI_MAX = 6u,
};

const SpaceReprCubes *getBGSpaceReprCubes(CubesReprByDepthAdd&,
        unsigned depth, Responder &responder);

#endif // CUBECOSETSADD_H
