#ifndef SEARCHOPTIMAL_H
#define SEARCHOPTIMAL_H

#include "cubesadd.h"
#include "responder.h"

void searchMovesOptimal(CubesReprByDepthAdd&, const cube &csearch, unsigned depthMax, Responder&);

#endif // SEARCHOPTIMAL_H
