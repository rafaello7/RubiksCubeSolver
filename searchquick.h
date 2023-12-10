#ifndef SEARCHQUICK_H
#define SEARCHQUICK_H

#include "cubedefs.h"
#include "responder.h"

void searchMovesQuickCatchFirst(const cube &csearch, Responder&);
void searchMovesQuickMulti(const cube &csearch, Responder&);

#endif // SEARCHQUICK_H
