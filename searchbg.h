#ifndef SEARCHBG_H
#define SEARCHBG_H

#include "cubedefs.h"
#include "responder.h"

int searchInSpaceMoves(const cube &cSpace, unsigned searchRev, unsigned searchTd,
        unsigned movesMax, Responder&, std::string &moves);

#endif // SEARCHBG_H
