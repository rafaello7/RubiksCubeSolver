#ifndef SEARCHOPTIMAL_H
#define SEARCHOPTIMAL_H

#include "cubedefs.h"
#include "responder.h"

void searchMovesOptimal(const cube &csearch, unsigned depthMax, Responder &responder);

#endif // SEARCHOPTIMAL_H
