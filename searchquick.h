#ifndef SEARCHQUICK_H
#define SEARCHQUICK_H

#include "CubesReprByDepthAdd.h"
#include "BGCubesReprByDepthAdd.h"
#include "CubeCosetsAdd.h"
#include "Responder.h"

void searchMovesQuickCatchFirst(CubesReprByDepthAdd&, BGCubesReprByDepthAdd&,
        CubeCosetsAdd&, const cube &csearch, Responder&);
void searchMovesQuickMulti(CubesReprByDepthAdd&, BGCubesReprByDepthAdd&,
        CubeCosetsAdd&, const cube &csearch, Responder&);

#endif // SEARCHQUICK_H
