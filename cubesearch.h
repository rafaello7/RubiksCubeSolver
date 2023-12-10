#ifndef CUBESEARCH_H
#define CUBESEARCH_H

#include "cubedefs.h"
#include "responder.h"

void searchMoves(const cube &csearch, char mode, unsigned depthMax, Responder&);

#endif // CUBESEARCH_H
