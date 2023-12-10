#include "searchbg.h"
#include "cubesaddbg.h"
#include <algorithm>

struct SearchIndexes {
    unsigned permReprIdx;
    bool reversed;
    bool symmetric;
    unsigned td;

    bool inc() {
        if( ++td == TCOUNTBG ) {
            if( symmetric ) {
                if( BGSpaceCubesReprByDepth::isUseReverse() ) {
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

static std::string getInSpaceMovesForMatch(const BGSpaceCubesReprByDepth &cubesReprByDepth,
        const cube &cSearch, const cube &c, unsigned searchRev, unsigned searchTd,
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
        moves = printInSpaceMoves(cubesReprByDepth, cTsymm, searchTd, !reversed);
        moves += printInSpaceMoves(cubesReprByDepth, cSearchTsymm, searchTd, reversed);
    }else{
        moves = printInSpaceMoves(cubesReprByDepth, cSearchTsymm, searchTd, !reversed);
        moves += printInSpaceMoves(cubesReprByDepth, cTsymm, searchTd, reversed);
    }
    return moves;
}

static void generateInSpaceSearchTarr(const cube &csearch, cube cSearchTarr[2][2][TCOUNTBG])
{
    for(unsigned reversed = 0;
            reversed < (BGSpaceCubesReprByDepth::isUseReverse() ? 2 : 1); ++reversed)
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

static bool searchInSpaceMovesForIdxs(const BGSpaceCubesReprByDepth &cubesReprByDepth,
        unsigned depth, unsigned depthMax, const cube cSearchTarr[2][2][TCOUNTBG],
        const SearchIndexes &indexes, std::string &moves,
        unsigned searchRev, unsigned searchTd)
{
    BGSpaceCubesReprAtDepth::ccpcubes_iter ccpIt =
        cubesReprByDepth[depth].ccpCubesBegin() + indexes.permReprIdx;
    const BGSpaceCornerPermReprCubes &ccpReprCubes = ccpIt->second;
    cubecorners_perm ccp = ccpIt->first;
    if( ccpReprCubes.empty() )
        return false;
    const cube &cSearchT = cSearchTarr[indexes.reversed][indexes.symmetric][indexes.td];
    cubecorners_perm ccpSearch = indexes.reversed ?
        cubecorners_perm::compose(cSearchT.ccp, ccp) :
        cubecorners_perm::compose(ccp, cSearchT.ccp);
    unsigned ccpReprSearchIdx = inbgspaceCubecornerPermRepresentativeIdx(ccpSearch);
    const BGSpaceCornerPermReprCubes &ccpReprSearchCubes =
        cubesReprByDepth[depthMax].getAt(ccpReprSearchIdx);
    if( ccpReprCubes.size() <= ccpReprSearchCubes.size() || !inbgspaceIsSingleTransform(ccpSearch) ) {
        for(BGSpaceCornerPermReprCubes::edges_iter edgeIt = ccpReprCubes.edgeBegin();
                edgeIt != ccpReprCubes.edgeEnd(); ++edgeIt)
        {
            cubeedges ce = *edgeIt;
            cubeedges ceSearch = indexes.reversed ?
                cubeedges::compose(cSearchT.ce, ce) :
                cubeedges::compose(ce, cSearchT.ce);
            cubeedges ceSearchRepr = inbgspaceCubeedgesRepresentative(ccpSearch, ceSearch);
            if( ccpReprSearchCubes.containsCubeEdges(ceSearchRepr) ) {
                cube cSearch = { .ccp = ccpSearch, .cco = csolved.cco, .ce = ceSearch };
                cube c = { .ccp = ccp, .cco = csolved.cco, .ce = ce };
                moves = getInSpaceMovesForMatch(cubesReprByDepth, cSearch, c, searchRev,
                        searchTd, indexes.reversed, indexes.symmetric, indexes.td);
                return true;
            }
        }
    }else{
        cube cSearchTrev = cSearchT.reverse();
        for(BGSpaceCornerPermReprCubes::edges_iter edgeIt = ccpReprSearchCubes.edgeBegin();
                edgeIt != ccpReprSearchCubes.edgeEnd(); ++edgeIt)
        {
            cubeedges ceSearchRepr = *edgeIt;
            cubeedges ceSearch = inbgspaceCubeedgesForRepresentative(ccpSearch, ceSearchRepr);
            cubeedges ce = indexes.reversed ?
                cubeedges::compose(cSearchTrev.ce, ceSearch) :
                cubeedges::compose(ceSearch, cSearchTrev.ce);
            if( ccpReprCubes.containsCubeEdges(ce) ) {
                cube cSearch = { .ccp = ccpSearch, .cco = csolved.cco, .ce = ceSearch };
                cube c = { .ccp = ccp, .cco = csolved.cco, .ce = ce };
                moves = getInSpaceMovesForMatch(cubesReprByDepth, cSearch, c, searchRev,
                        searchTd, indexes.reversed, indexes.symmetric, indexes.td);
                return true;
            }
        }
    }
    return false;
}

static bool searchInSpaceMovesA(const BGSpaceCubesReprByDepth &cubesReprByDepthBG,
        const cube cSpaceArr[2][2][TCOUNTBG], unsigned searchRev, unsigned searchTd,
        unsigned depth, unsigned depthMax, std::string &moves)
{
    SearchIndexes indexes {};
    do {
        if( searchInSpaceMovesForIdxs(cubesReprByDepthBG, depth, depthMax, cSpaceArr, indexes,
                    moves, searchRev, searchTd) )
            return true;
    } while( indexes.inc() );
    return false;
}

static bool searchInSpaceMovesB(const BGSpaceCubesReprByDepth &cubesReprByDepthBG,
		const cube &cSpace, unsigned searchRev, unsigned searchTd, unsigned depth, unsigned depthMax,
        std::string &moves)
{
    const BGSpaceCubesReprAtDepth &ccReprCubesC = cubesReprByDepthBG[depth];
    for(BGSpaceCubesReprAtDepth::ccpcubes_iter ccpCubes1It = ccReprCubesC.ccpCubesBegin();
            ccpCubes1It != ccReprCubesC.ccpCubesEnd(); ++ccpCubes1It)
    {
        const BGSpaceCornerPermReprCubes &ccpCubes1 = ccpCubes1It->second;
        cubecorners_perm ccp1 = ccpCubes1It->first;
        if( ccpCubes1.empty() )
            continue;
        for(BGSpaceCornerPermReprCubes::edges_iter edge1It = ccpCubes1.edgeBegin();
                edge1It != ccpCubes1.edgeEnd(); ++edge1It)
        {
            const cubeedges ce1 = *edge1It;
            cube c1 = { .ccp = ccp1, .cco = csolved.cco, .ce = ce1 };
            std::vector<cube> cubesChecked;
            for(unsigned reversed1 = 0;
                    reversed1 < (BGSpaceCubesReprByDepth::isUseReverse() ? 2 : 1); ++reversed1)
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
                        generateInSpaceSearchTarr(cSearch1, cSpaceArr);
                        if( searchInSpaceMovesA(cubesReprByDepthBG, cSpaceArr, searchRev,
                                    searchTd, depthMax, depthMax, moves2) )
                        {
                            if( searchRev ) {
                                moves = printInSpaceMoves(
                                        cubesReprByDepthBG, c1T, searchTd, true) + moves2;
                            }else{
                                moves = moves2 + printInSpaceMoves(
                                        cubesReprByDepthBG, c1T, searchTd);
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

static bool containsInSpaceCube(const BGSpaceCubesReprAtDepth &cubesRepr, const cube &c)
{
    unsigned ccpReprSearchIdx = inbgspaceCubecornerPermRepresentativeIdx(c.ccp);
    const BGSpaceCornerPermReprCubes &ccpReprSearchCubes = cubesRepr.getAt(ccpReprSearchIdx);
    cubeedges ceSearchRepr = inbgspaceCubeedgesRepresentative(c.ccp, c.ce);
    return ccpReprSearchCubes.containsCubeEdges(ceSearchRepr);
}

int searchInSpaceMoves(const cube &cSpace, unsigned searchRev, unsigned searchTd,
        unsigned movesMax, Responder &responder, std::string &moves)
{
    const BGSpaceCubesReprByDepth *cubesReprByDepthBG = getCubesInSpace(0, responder);
    if( cubesReprByDepthBG == NULL )
        return -1;
    for(unsigned depthSearch = 0; depthSearch <= movesMax &&
            depthSearch < cubesReprByDepthBG->availCount(); ++depthSearch)
    {
        if( containsInSpaceCube((*cubesReprByDepthBG)[depthSearch], cSpace) ) {
            moves = printInSpaceMoves(*cubesReprByDepthBG, cSpace, searchTd, !searchRev);
            return depthSearch;
        }
    }
    if( cubesReprByDepthBG->availCount() <= movesMax ) {
        cube cSpaceTarr[2][2][TCOUNTBG];
        generateInSpaceSearchTarr(cSpace, cSpaceTarr);
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
                cubesReprByDepthBG = getCubesInSpace(depthMax, responder);
                if( cubesReprByDepthBG == NULL )
                    return -1;
                if( searchInSpaceMovesA(*cubesReprByDepthBG, cSpaceTarr, searchRev, searchTd,
                            depth, depthMax, moves) )
                    return depthSearch;
                ++depthSearch;
            }else{
                cubesReprByDepthBG = getCubesInSpace(TWOPHASE_DEPTH2_MAX, responder);
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

