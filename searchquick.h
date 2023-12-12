#ifndef SEARCHQUICK_H
#define SEARCHQUICK_H

#include "cubesadd.h"
#include "cubesaddbg.h"
#include "cubecosetsadd.h"
#include "responder.h"

void searchMovesQuickCatchFirst(CubesReprByDepthAdd&, BGCubesReprByDepthAdd&,
        CubeCosetsAdd&, const cube &csearch, Responder&);
void searchMovesQuickMulti(CubesReprByDepthAdd&, BGCubesReprByDepthAdd&,
        CubeCosetsAdd&, const cube &csearch, Responder&);

#endif // SEARCHQUICK_H
