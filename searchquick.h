#ifndef SEARCHQUICK_H
#define SEARCHQUICK_H

#include "cubedefs.h"
#include "responder.h"

void searchMovesQuickCatchFirst(const cube &csearch, Responder&, bool useTwoPhaseSearchRev = true);
void searchMovesQuickMulti(const cube &csearch, Responder&, bool useTwoPhaseSearchRev = true);

#endif // SEARCHQUICK_H
