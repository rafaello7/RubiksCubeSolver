#ifndef CUBESADD_H
#define CUBESADD_H

#include "cubesrepr.h"
#include "responder.h"

enum { DEPTH_MAX_AVAIL = 10 };

/* Returns the cube set filled up to at least the specified depth (inclusive).
 * The function triggers filling procedure when the depth is requested first time.
 * When the filling procedure is canceled, NULL is returned.
 */
const CubesReprByDepth *getReprCubes(unsigned depth, Responder&);

#endif // CUBESADD_H
