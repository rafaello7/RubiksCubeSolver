#ifndef SEARCHQUICK_H
#define SEARCHQUICK_H

#include "cubesadd.h"
#include "responder.h"

void searchMovesQuickCatchFirst(CubesReprByDepthAdd&, const cube &csearch, Responder&);
void searchMovesQuickMulti(CubesReprByDepthAdd&, const cube &csearch, Responder&);

#endif // SEARCHQUICK_H
