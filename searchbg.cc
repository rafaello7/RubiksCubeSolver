#include "searchbg.h"
#include "cubesaddbg.h"
#include <algorithm>

struct SearchIndexes {
    unsigned permReprIdx;
    bool reversed;
    bool symmetric;
    unsigned td;

    bool inc(bool useReverse) {
        if( ++td == TCOUNTBG ) {
            if( symmetric ) {
                if( useReverse ) {
                    if( reversed && ++permReprIdx == 1672 )
                        return false;
                    reversed = !reversed;
                }else if( ++permReprIdx == 2768 )
                    return false;
            }
            symmetric = !symmetric;
            td = 0;
        }
        return true;
    }
};

static std::string getInSpaceMovesForMatch(const BGCubesReprByDepth &cubesReprByDepth,
        const cube &cSearch, const cube &c, bool searchRev, unsigned searchTd,
        unsigned reversed, unsigned symmetric, unsigned tdidx)
{
    // cube found:
    //  if reversed: cSearch = transform(symmetric((csearch rev))) ⊙  c
    //      (csearch rev) = symmetric(transformReverse(cSearch)) ⊙  symmetric(transformReverse(c rev))
    //  if not: cSearch = c ⊙  transform(symmetric(csearch))
    //      (csearch rev) = (symmetric(transformReverse(cSearch)) rev) ⊙  (symmetric(transformReverse(c rev)) rev)
    unsigned td = BGSpaceTransforms[tdidx];
    cube cSearchT = cSearch.transform(transformReverse(td));
    cube cSearchTsymm = symmetric ? cSearchT.symmetric() : cSearchT;
    cube cT = c.transform(transformReverse(td));
    cube cTsymm = symmetric ? cT.symmetric() : cT;
    std::string moves;
    if( searchRev ) {
        moves = cubesReprByDepth.getMoves(cTsymm, searchTd, !reversed);
        moves += cubesReprByDepth.getMoves(cSearchTsymm, searchTd, reversed);
    }else{
        moves = cubesReprByDepth.getMoves(cSearchTsymm, searchTd, !reversed);
        moves += cubesReprByDepth.getMoves(cTsymm, searchTd, reversed);
    }
    return moves;
}

static void generateInSpaceSearchTarr(const cube &csearch, bool useReverse,
        cube cSearchTarr[2][2][TCOUNTBG])
{
    for(unsigned reversed = 0;
            reversed < (useReverse ? 2 : 1); ++reversed)
    {
        cube csearchrev = reversed ? csearch.reverse() : csearch;
        for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
            cube csearchrevsymm = symmetric ? csearchrev.symmetric() : csearchrev;
            for(unsigned tdidx = 0; tdidx < TCOUNTBG; ++tdidx)
                cSearchTarr[reversed][symmetric][tdidx] =
                    csearchrevsymm.transform(BGSpaceTransforms[tdidx]);
        }
    }
}

static bool searchInSpaceMovesForIdxs(const BGCubesReprByDepth &cubesReprByDepth,
        unsigned depth, unsigned depthMax, const cube cSearchTarr[2][2][TCOUNTBG],
        const SearchIndexes &indexes, std::string &moves,
        bool searchRev, unsigned searchTd)
{
    const cube &cSearchT = cSearchTarr[indexes.reversed][indexes.symmetric][indexes.td];
    cube c, cSearch;
    if( cubesReprByDepth.searchMovesForReprPerm(indexes.permReprIdx,
            depth, depthMax, cSearchT, indexes.reversed, c, cSearch) )
    {
        moves = getInSpaceMovesForMatch(cubesReprByDepth, cSearch, c, searchRev,
                searchTd, indexes.reversed, indexes.symmetric, indexes.td);
        return true;
    }
    return false;
}

static bool searchInSpaceMovesA(const BGCubesReprByDepth &cubesReprByDepthBG,
        const cube cSpaceArr[2][2][TCOUNTBG], bool searchRev, unsigned searchTd,
        unsigned depth, unsigned depthMax, std::string &moves)
{
    SearchIndexes indexes {};
    bool useReverse = cubesReprByDepthBG.isUseReverse();
    do {
        if( searchInSpaceMovesForIdxs(cubesReprByDepthBG, depth, depthMax, cSpaceArr, indexes,
                    moves, searchRev, searchTd) )
            return true;
    } while( indexes.inc(useReverse) );
    return false;
}

static bool searchInSpaceMovesB(const BGCubesReprByDepth &cubesReprByDepthBG,
		const cube &cSpace, bool searchRev, unsigned searchTd, unsigned depth, unsigned depthMax,
        std::string &moves)
{
    const BGCubesReprAtDepth &ccReprCubesC = cubesReprByDepthBG[depth];
    for(BGCubesReprAtDepth::ccpcubes_iter ccpCubes1It = ccReprCubesC.ccpCubesBegin();
            ccpCubes1It != ccReprCubesC.ccpCubesEnd(); ++ccpCubes1It)
    {
        const BGCornerPermReprCubes &ccpCubes1 = *ccpCubes1It;
        cubecorners_perm ccp1 = ccReprCubesC.getPermAt(ccpCubes1It);
        if( ccpCubes1.empty() )
            continue;
        for(BGCornerPermReprCubes::edges_iter edge1It = ccpCubes1.edgeBegin();
                edge1It != ccpCubes1.edgeEnd(); ++edge1It)
        {
            const cubeedges ce1 = *edge1It;
            cube c1 = { .ccp = ccp1, .cco = csolved.cco, .ce = ce1 };
            std::vector<cube> cubesChecked;
            for(unsigned reversed1 = 0;
                    reversed1 < (cubesReprByDepthBG.isUseReverse() ? 2 : 1); ++reversed1)
            {
                cube c1r = reversed1 ? c1.reverse() : c1;
                for(unsigned symmetric1 = 0; symmetric1 < 2; ++symmetric1) {
                    cube c1rs = symmetric1 ? c1r.symmetric() : c1r;
                    for(unsigned td1idx = 0; td1idx < TCOUNTBG; ++td1idx) {
                        const cube c1T = c1rs.transform(BGSpaceTransforms[td1idx]);
                        bool isDup = std::find(cubesChecked.begin(),
                                cubesChecked.end(), c1T) != cubesChecked.end();
                        if( isDup )
                            continue;
                        cubesChecked.push_back(c1T);
                        cube cSearch1 = cube::compose(c1T, cSpace);
                        std::string moves2;
                        cube cSpaceArr[2][2][TCOUNTBG];
                        generateInSpaceSearchTarr(cSearch1,
                                cubesReprByDepthBG.isUseReverse(), cSpaceArr);
                        if( searchInSpaceMovesA(cubesReprByDepthBG, cSpaceArr, searchRev,
                                    searchTd, depthMax, depthMax, moves2) )
                        {
                            if( searchRev ) {
                                moves = cubesReprByDepthBG.getMoves(
                                        c1T, searchTd, true) + moves2;
                            }else{
                                moves = moves2 + cubesReprByDepthBG.getMoves(c1T, searchTd);
                            }
                            return true;
                        }
                    }
                }
            }
        }
	}
    return false;
}

int searchInSpaceMoves(BGCubesReprByDepthAdd &cubesReprByDepthAdd,
        const cube &cSpace, bool searchRev, unsigned searchTd,
        unsigned movesMax, Responder &responder, std::string &moves)
{
    const BGCubesReprByDepth *cubesReprByDepthBG = cubesReprByDepthAdd.getReprCubes(0, responder);
    if( cubesReprByDepthBG == NULL )
        return -1;
    for(unsigned depthSearch = 0; depthSearch <= movesMax &&
            depthSearch < cubesReprByDepthBG->availCount(); ++depthSearch)
    {
        if( (*cubesReprByDepthBG)[depthSearch].containsCube(cSpace) ) {
            moves = cubesReprByDepthBG->getMoves(cSpace, searchTd, !searchRev);
            return depthSearch;
        }
    }
    if( cubesReprByDepthBG->availCount() <= movesMax ) {
        cube cSpaceTarr[2][2][TCOUNTBG];
        generateInSpaceSearchTarr(cSpace, cubesReprByDepthBG->isUseReverse(), cSpaceTarr);
        for(unsigned depthSearch = cubesReprByDepthBG->availCount();
                depthSearch <= movesMax && depthSearch <= 3*TWOPHASE_DEPTH2_MAX;
                ++depthSearch)
        {
            if( depthSearch < 2*cubesReprByDepthBG->availCount()-1 && depthSearch <= movesMax )
            {
                unsigned depthMax = cubesReprByDepthBG->availCount() - 1;
                if( searchInSpaceMovesA(*cubesReprByDepthBG, cSpaceTarr, searchRev, searchTd,
                            depthSearch-depthMax, depthMax, moves) )
                    return depthSearch;
            }else if( depthSearch <= 2*TWOPHASE_DEPTH2_MAX ) {
                unsigned depth = depthSearch / 2;
                unsigned depthMax = depthSearch - depth;
                cubesReprByDepthBG = cubesReprByDepthAdd.getReprCubes(depthMax, responder);
                if( cubesReprByDepthBG == NULL )
                    return -1;
                if( searchInSpaceMovesA(*cubesReprByDepthBG, cSpaceTarr, searchRev, searchTd,
                            depth, depthMax, moves) )
                    return depthSearch;
                ++depthSearch;
            }else{
                cubesReprByDepthBG = cubesReprByDepthAdd.getReprCubes(TWOPHASE_DEPTH2_MAX, responder);
                if( cubesReprByDepthBG == NULL )
                    return -1;
                unsigned depth = depthSearch - 2*TWOPHASE_DEPTH2_MAX;
                if( searchInSpaceMovesB(*cubesReprByDepthBG, cSpace, searchRev, searchTd,
                            depth, TWOPHASE_DEPTH2_MAX, moves) )
                    return 2*TWOPHASE_DEPTH2_MAX + depth;
            }
        }
    }
    return -1;
}

