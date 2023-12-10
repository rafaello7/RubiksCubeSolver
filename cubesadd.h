#ifndef CUBESADD_H
#define CUBESADD_H

#include "cubesrepr.h"
#include "responder.h"

enum { DEPTH_MAX_AVAIL = 10 };

const CubesReprByDepth *getReprCubes(unsigned depth, Responder&);

#endif // CUBESADD_H
