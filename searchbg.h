#ifndef SEARCHBG_H
#define SEARCHBG_H

#include "cubesaddbg.h"
#include "responder.h"

/* The cSpace cube shall be in BG space. The function searches for a shortest
 * solution consisting from the BG moves, no longer than movesMax. If such
 * solution don't exist, -1 is returned. The -1 value can be returned also when
 * search operation is canceled. This can be distinguished by call of
 * ProgressBase::isStopRequested().
 *
 * When the searchRev parameter is set to true, the moves sequence is reversed.
 * The searchTd parameter should be one of transform enum values: TD_0,
 * TD_C0_7_CW, TD_C0_7_CCW. If non-zero, means that cube is transformed from
 * yellow-white or orange-red space into the blue-green space by the rotation.
 * The found moves are translated into moves in appropriate space.
 */
int searchInSpaceMoves(BGCubesReprByDepthAdd &cubesReprByDepthAdd,
        const cube &cSpace, bool searchRev, unsigned searchTd,
        unsigned movesMax, Responder&, std::string &moves);

#endif // SEARCHBG_H
