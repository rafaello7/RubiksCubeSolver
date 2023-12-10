#include "cubesearch.h"
#include "searchoptimal.h"
#include "searchquick.h"
#include <sstream>

void searchMoves(const cube &csearch, char mode, unsigned depthMax, Responder &responder)
{
    responder.message("setup: depth %d", depthMax);
    switch( mode ) {
    case 'q':
        searchMovesQuickCatchFirst(csearch, responder);
        break;
    case 'm':
        searchMovesQuickMulti(csearch, responder);
        break;
    default:
        searchMovesOptimal(csearch, depthMax, responder);
        break;
    }
}

