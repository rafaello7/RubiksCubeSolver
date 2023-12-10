#ifndef CUBESADDBG_H
#define CUBESADDBG_H

#include "cubesreprbg.h"
#include "responder.h"

enum {
    TWOPHASE_DEPTH2_MAX = 8u
};

const BGSpaceCubesReprByDepth *getCubesInSpace(unsigned depth, Responder&);

#endif // CUBESADDBG_H
