#include "searchquick.h"
#include "cubesadd.h"
#include "cubecosetsadd.h"
#include "searchbg.h"
#include "progressbase.h"
#include "threadpoolhelper.h"
#include <algorithm>


enum { TWOPHASE_SEARCHREV = 2 };

/* The cSearchMid is an intermediate cube, cube1 ⊙  csearch.
 * Assumes the cSearchMid was found in SpaceReprCubes at depthMax.
 * Searches for a cube from the same BG space among cubesReprByDepth[depthMax].
 */
static int searchPhase1Cube2(
        const CubesReprByDepth &cubesReprByDepth,
        BGCubesReprByDepthAdd &bgcubesReprByDepthAdd,
        const cube &cSearchMid, const std::vector<cube> &cubes,
        unsigned searchRev, unsigned searchTd, unsigned cube2Depth, unsigned movesMax,
        bool catchFirst, Responder &responder, std::string &moves)
{
    int bestMoveCount = -1;

    for(const cube &cube2 : cubes) {
        // cSearchMid = cube1 ⊙  (csearch rev) = cube2 ⊙  cSpace
        // csearch rev = (cube1 rev) ⊙  cube2 ⊙  cSpace
        //
        // cSearchMid = cube1 ⊙  csearch = cube2 ⊙  cSpace
        // cSpace = (cube2 rev) ⊙  cube1 ⊙  csearch
        // csearch = (cube1 rev) ⊙  cube2 ⊙  cSpace
        // csearch rev = (cSpace rev) ⊙  (cube2 rev) ⊙  cube1
        cube cSpace = cube::compose(cube2.reverse(), cSearchMid);
        std::string movesInSpace;
        int depthInSpace = searchInSpaceMoves(bgcubesReprByDepthAdd, cSpace, searchRev, searchTd,
                movesMax-cube2Depth, responder, movesInSpace);
        if( depthInSpace >= 0 ) {
            //responder.message("found in-space cube at depth %d", depthInSpace);
            cube cube2T = cube2.transform(transformReverse(searchTd));
            std::string cube2Moves = cubesReprByDepth.getMoves(cube2T, !searchRev);
            if( searchRev )
                moves = cube2Moves + movesInSpace;
            else
                moves = movesInSpace + cube2Moves;
            bestMoveCount = cube2Depth + depthInSpace;
            if( catchFirst || depthInSpace == 0 )
                return bestMoveCount;
            movesMax = bestMoveCount-1;
        }
    }
    return bestMoveCount;
}

class QuickSearchProgress : public ProgressBase {
    const unsigned m_itemCount;
    const unsigned m_depthSearch;
    const bool m_catchFirst;
    unsigned m_nextItemIdx;
    int m_movesMax;
    int m_bestMoveCount;
    std::string m_bestMoves;

public:
    QuickSearchProgress(unsigned itemCount, unsigned depthSearch,
            bool catchFirst, int movesMax)
        : m_itemCount(itemCount), m_depthSearch(depthSearch),
        m_catchFirst(catchFirst), m_nextItemIdx(0),
        m_movesMax(movesMax), m_bestMoveCount(-1)
    {
    }

    bool isCatchFirst() const { return m_catchFirst; }
    int inc(Responder &responder, unsigned *itemIdxBuf);
    int setBestMoves(const std::string &moves, int moveCount, Responder &responder);
    int getBestMoves(std::string &moves) const;
};

int QuickSearchProgress::inc(Responder &responder, unsigned *itemIdxBuf) {
    int movesMax;
    unsigned itemIdx;
    mutexLock();
    if( m_movesMax >= 0 && m_nextItemIdx < m_itemCount && !isStopRequestedNoLock() ) {
        itemIdx = m_nextItemIdx++;
        movesMax = m_movesMax;
    }else
        movesMax = -1;
    mutexUnlock();
    if( movesMax >= 0 ) {
        *itemIdxBuf = itemIdx;
        if( m_depthSearch >= 9 ) {
            unsigned procCountNext = 100 * (itemIdx+1) / m_itemCount;
            unsigned procCountCur = 100 * itemIdx / m_itemCount;
            if( procCountNext != procCountCur && (m_depthSearch>=10 || procCountCur % 10 == 0) )
                responder.progress("depth %d search %d%%",
                        m_depthSearch, 100 * itemIdx / m_itemCount);
        }
    }
    return movesMax;
}

int QuickSearchProgress::setBestMoves(const std::string &moves, int moveCount, Responder &responder) {
    int movesMax;
    mutexLock();
    if( m_bestMoveCount < 0 || moveCount < m_bestMoveCount ) {
        m_bestMoves = moves;
        m_bestMoveCount = moveCount;
        m_movesMax = m_catchFirst ? -1 : moveCount-1;
        responder.movecount("%u at depth: %u", moveCount, m_depthSearch);
    }
    movesMax = m_movesMax;
    mutexUnlock();
    return movesMax;
}

int QuickSearchProgress::getBestMoves(std::string &moves) const {
    moves = m_bestMoves;
    return m_bestMoveCount;
}

static bool searchMovesQuickForCcp(cubecorners_perm ccp, const CornerPermReprCubes &ccpReprCubes,
        const CubesReprByDepth &cubesReprByDepth,
        BGCubesReprByDepthAdd &bgcubesReprByDepthAdd,
        const SpaceReprCubes *bgSpaceCubes,
        const std::vector<std::pair<cube, std::string>> &csearchWithMovesAppend,
        unsigned depth, unsigned depth1Max, unsigned movesMax,
        Responder &responder, QuickSearchProgress *searchProgress)
{
    std::vector<cube> csearchTarr[2][3];

    for(auto [c, moves] : csearchWithMovesAppend) {
        csearchTarr[0][0].emplace_back(c);
        csearchTarr[0][1].emplace_back(c.transform(1));
        csearchTarr[0][2].emplace_back(c.transform(2));
        cube crev = c.reverse();
        csearchTarr[1][0].emplace_back(crev);
        csearchTarr[1][1].emplace_back(crev.transform(1));
        csearchTarr[1][2].emplace_back(crev.transform(2));
    }
    for(unsigned reversed = 0; reversed < (cubesReprByDepth.isUseReverse() ? 2 : 1); ++reversed) {
        cubecorners_perm ccprev = reversed ? ccp.reverse() : ccp;
        for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
            cubecorners_perm ccprevsymm = symmetric ? ccprev.symmetric() : ccprev;
            for(unsigned td = 0; td < TCOUNT; ++td) {
                cubecorners_perm ccpT = ccprevsymm.transform(td);
                for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpReprCubes.ccoCubesBegin();
                        ccoCubesIt != ccpReprCubes.ccoCubesEnd(); ++ccoCubesIt)
                {
                    const CornerOrientReprCubes &ccoReprCubes = *ccoCubesIt;
                    cubecorner_orients cco = ccoReprCubes.getOrients();
                    cubecorner_orients ccorev = reversed ? cco.reverse(ccp) : cco;
                    cubecorner_orients ccorevsymm = symmetric ? ccorev.symmetric() : ccorev;
                    cubecorner_orients ccoT = ccorevsymm.transform(ccprevsymm, td);
                    std::vector<cubeedges> ceTarr;
                    for(unsigned srchItem = 0; srchItem < csearchTarr[0][0].size(); ++srchItem) {
                        for(unsigned searchRev = 0; searchRev < TWOPHASE_SEARCHREV; ++searchRev)
                        {
                            for(unsigned searchTd = 0; searchTd < 3; ++searchTd) {
                                const cube &csearchT = csearchTarr[searchRev][searchTd][srchItem];
                                cubecorners_perm ccpSearch = cubecorners_perm::compose(ccpT, csearchT.ccp);
                                cubecorner_orients ccoSearch = cubecorner_orients::compose(ccoT,
                                        csearchT.ccp, csearchT.cco);
                                cubecorner_orients ccoSearchReprBG = ccoSearch.representativeBG(ccpSearch);
                                unsigned short searchReprCOrientIdx = ccoSearchReprBG.getOrientIdx();
                                if( (*bgSpaceCubes)[depth1Max].containsCCOrients(searchReprCOrientIdx) ) {
                                    if( ceTarr.empty() ) {
                                        for(CornerOrientReprCubes::edges_iter edgeIt = ccoReprCubes.edgeBegin();
                                                edgeIt != ccoReprCubes.edgeEnd(); ++edgeIt)
                                        {
                                            cubeedges ce = *edgeIt;
                                            cubeedges cerev = reversed ? ce.reverse() : ce;
                                            cubeedges cerevsymm = symmetric ? cerev.symmetric() : cerev;
                                            cubeedges ceT = cerevsymm.transform(td);
                                            ceTarr.push_back(ceT);
                                        }
                                    }
                                    for(const cubeedges &ceT : ceTarr) {
                                        cubeedges ceSearch = cubeedges::compose(ceT, csearchT.ce);
                                        cubeedges ceSearchSpaceRepr = ceSearch.representativeBG();
                                        const std::vector<cube> *cubesForCE =
                                            (*bgSpaceCubes)[depth1Max].getCubesForCE(
                                                    searchReprCOrientIdx, ceSearchSpaceRepr);
                                        if( cubesForCE ) {
                                            cube cSearch1 = { .ccp = ccpSearch, .cco = ccoSearch, .ce = ceSearch };
                                            std::string inspaceWithCube2Moves;
                                            int moveCount = searchPhase1Cube2(cubesReprByDepth,
                                                    bgcubesReprByDepthAdd,
                                                    cSearch1, *cubesForCE, searchRev, searchTd,
                                                    depth1Max, movesMax-depth, searchProgress->isCatchFirst(),
                                                    responder, inspaceWithCube2Moves);
                                            if( moveCount >= 0 ) {
                                                cube cube1 = { .ccp = ccpT, .cco = ccoT, .ce = ceT };
                                                cube cube1T = cube1.transform(transformReverse(searchTd));
                                                std::string cube1Moves = cubesReprByDepth.getMoves(cube1T, searchRev);
                                                const std::string &cubeMovesAppend =
                                                    csearchWithMovesAppend[srchItem].second;
                                                std::string moves;
                                                if( searchRev )
                                                    moves = cube1Moves + inspaceWithCube2Moves;
                                                else
                                                    moves = inspaceWithCube2Moves + cube1Moves;
                                                moves += cubeMovesAppend;
                                                movesMax = searchProgress->setBestMoves(moves,
                                                        depth+moveCount, responder);
                                                if( movesMax < 0 )
                                                    return true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if( searchProgress->isStopRequested() )
                        return true;
                }
            }
        }
    }
    return false;
}

static void searchMovesQuickTa(unsigned threadNo,
        const CubesReprByDepth *cubesReprByDepth,
        BGCubesReprByDepthAdd *bgcubesReprByDepthAdd,
        const SpaceReprCubes *bgSpaceCubes,
        const cube *csearch, unsigned depth, unsigned depth1Max,
        Responder *responder, QuickSearchProgress *searchProgress)
{
    unsigned itemIdx;
    int movesMax;

    while( (movesMax = searchProgress->inc(*responder, &itemIdx)) >= 0 ) {
        const CornerPermReprCubes &ccpReprCubes = (*cubesReprByDepth)[depth].getAt(itemIdx);
        cubecorners_perm ccp = cubesReprByDepth->getReprPermForIdx(itemIdx);
        if( !ccpReprCubes.empty() ) {
            std::vector<std::pair<cube, std::string>> cubesWithMoves;
            cubesWithMoves.emplace_back(std::make_pair(*csearch, std::string()));
            if( searchMovesQuickForCcp(ccp, ccpReprCubes, *cubesReprByDepth,
                        *bgcubesReprByDepthAdd, bgSpaceCubes,
                    cubesWithMoves, depth, depth1Max, movesMax,
                    *responder, searchProgress) )
                return;
        }
    }
}

static void searchMovesQuickTb1(unsigned threadNo, const CubesReprByDepth *cubesReprByDepth,
        BGCubesReprByDepthAdd *bgcubesReprByDepthAdd,
        const SpaceReprCubes *bgSpaceCubes, const cube *csearch, unsigned depth1Max,
        Responder *responder, QuickSearchProgress *searchProgress)
{
    unsigned item2Idx;
    int movesMax;
    while( (movesMax = searchProgress->inc(*responder, &item2Idx)) >= 0 ) {
        const CornerPermReprCubes &ccp2ReprCubes = (*cubesReprByDepth)[depth1Max].getAt(item2Idx);
        cubecorners_perm ccp2 = cubesReprByDepth->getReprPermForIdx(item2Idx);
        if( ccp2ReprCubes.empty() )
            continue;
        std::vector<std::pair<cube, std::string>> cubesWithMoves;
        for(unsigned rd = 0; rd < RCOUNT; ++rd) {
            cube c1Search = cube::compose(crotated[rd], *csearch);
            std::string cube1Moves = cubesReprByDepth->getMoves(crotated[rd]);
            cubesWithMoves.emplace_back(std::make_pair(c1Search, cube1Moves));
        }
        if( searchMovesQuickForCcp(ccp2, ccp2ReprCubes, *cubesReprByDepth,
                    *bgcubesReprByDepthAdd, bgSpaceCubes,
                    cubesWithMoves, depth1Max+1, depth1Max, movesMax,
                    *responder, searchProgress) )
            return;
    }
}

static void searchMovesQuickTb(unsigned threadNo, const CubesReprByDepth *cubesReprByDepth,
        BGCubesReprByDepthAdd *bgcubesReprByDepthAdd,
        const SpaceReprCubes *bgSpaceCubes, const cube *csearch, unsigned depth, unsigned depth1Max,
        Responder *responder, QuickSearchProgress *searchProgress)
{
    CubesReprAtDepth::ccpcubes_iter ccp2It;
    unsigned item2Idx;
    int movesMax;
    const CubesReprAtDepth &ccReprCubesC = (*cubesReprByDepth)[depth];

    while( (movesMax = searchProgress->inc(*responder, &item2Idx)) >= 0 ) {
        const CornerPermReprCubes &ccp2ReprCubes = (*cubesReprByDepth)[depth1Max].getAt(item2Idx);
        cubecorners_perm ccp2 = cubesReprByDepth->getReprPermForIdx(item2Idx);
        if( ccp2ReprCubes.empty() )
            continue;
        for(CubesReprAtDepth::ccpcubes_iter ccp1It = ccReprCubesC.ccpCubesBegin();
                ccp1It != ccReprCubesC.ccpCubesEnd(); ++ccp1It)
        {
            const CornerPermReprCubes &ccp1ReprCubes = *ccp1It;
            cubecorners_perm ccp1 = ccReprCubesC.getPermAt(ccp1It);
            if( ccp1ReprCubes.empty() )
                continue;
            for(CornerPermReprCubes::ccocubes_iter cco1CubesIt = ccp1ReprCubes.ccoCubesBegin();
                    cco1CubesIt != ccp1ReprCubes.ccoCubesEnd(); ++cco1CubesIt)
            {
                const CornerOrientReprCubes &cco1ReprCubes = *cco1CubesIt;
                cubecorner_orients cco1 = cco1ReprCubes.getOrients();
                for(CornerOrientReprCubes::edges_iter edge1It = cco1ReprCubes.edgeBegin();
                        edge1It != cco1ReprCubes.edgeEnd(); ++edge1It)
                {
                    cubeedges ce1 = *edge1It;
                    std::vector<cube> cubesChecked;
                    std::vector<std::pair<cube, std::string>> cubesWithMoves;
                    for(unsigned reversed1 = 0;
                            reversed1 < (cubesReprByDepth->isUseReverse() ? 2 : 1); ++reversed1)
                    {
                        cubecorners_perm ccp1rev = reversed1 ? ccp1.reverse() : ccp1;
                        cubecorner_orients cco1rev = reversed1 ? cco1.reverse(ccp1) : cco1;
                        cubeedges ce1rev = reversed1 ? ce1.reverse() : ce1;
                        for(unsigned symmetric1 = 0; symmetric1 < 2; ++symmetric1) {
                            cubecorners_perm ccp1revsymm = symmetric1 ? ccp1rev.symmetric() : ccp1rev;
                            cubecorner_orients cco1revsymm = symmetric1 ? cco1rev.symmetric() : cco1rev;
                            cubeedges ce1revsymm = symmetric1 ? ce1rev.symmetric() : ce1rev;
                            for(unsigned td1 = 0; td1 < TCOUNT; ++td1) {
                                cubecorners_perm ccp1T = ccp1revsymm.transform(td1);
                                cubecorner_orients cco1T = cco1revsymm.transform(ccp1revsymm, td1);
                                cubeedges ce1T = ce1revsymm.transform(td1);

                                cube c1T = { .ccp = ccp1T, .cco = cco1T, .ce = ce1T };
                                bool isDup = std::find(cubesChecked.begin(), cubesChecked.end(), c1T) != cubesChecked.end();
                                if( isDup )
                                    continue;
                                cubesChecked.push_back(c1T);

                                cubecorners_perm ccp1Search = cubecorners_perm::compose(ccp1T, csearch->ccp);
                                cubecorner_orients cco1Search = cubecorner_orients::compose(cco1T,
                                        csearch->ccp, csearch->cco);
                                cubeedges ce1Search = cubeedges::compose(ce1T, csearch->ce);
                                cube c1Search = { .ccp = ccp1Search, .cco = cco1Search, .ce = ce1Search };
                                std::string cube1Moves = cubesReprByDepth->getMoves(c1T);
                                cubesWithMoves.emplace_back(std::make_pair(c1Search, cube1Moves));
                            }
                        }
                    }
                    if( searchMovesQuickForCcp(ccp2, ccp2ReprCubes, *cubesReprByDepth,
                                *bgcubesReprByDepthAdd, bgSpaceCubes,
                                cubesWithMoves, depth+depth1Max, depth1Max, movesMax,
                                *responder, searchProgress) )
                        return;
                }
            }
        }
    }
}

static bool searchMovesQuickA(CubesReprByDepthAdd &cubesReprByDepthAdd,
        BGCubesReprByDepthAdd &bgcubesReprByDepthAdd,
        const cube &csearch, unsigned depthSearch, bool catchFirst,
        Responder &responder, unsigned movesMax,
        int &moveCount, std::string &moves)
{
    moveCount = -1;
    const CubesReprByDepth *cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(0, responder);
    const SpaceReprCubes *bgSpaceCubes = getBGSpaceReprCubes(cubesReprByDepthAdd, 0, responder);
    if( cubesReprByDepth == NULL || bgSpaceCubes == NULL )
        return true;
    unsigned depthsAvail = std::min(cubesReprByDepth->availCount(), bgSpaceCubes->availCount()) - 1;
    unsigned depth, depth1Max;
    if( depthSearch <= depthsAvail ) {
        depth = 0;
        depth1Max = depthSearch;
    }else if( depthSearch <= 2 * depthsAvail ) {
        depth = depthSearch - depthsAvail;
        depth1Max = depthsAvail;
    }else{
        depth = depthSearch / 2;
        depth1Max = depthSearch - depth;
        cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depth1Max, responder);
        bgSpaceCubes = getBGSpaceReprCubes(cubesReprByDepthAdd, depth1Max, responder);
        if( cubesReprByDepth == NULL || bgSpaceCubes == NULL )
            return true;
    }
    QuickSearchProgress searchProgress((*cubesReprByDepth)[depth].size(),
            depthSearch, catchFirst, movesMax);
    runInThreadPool(searchMovesQuickTa, cubesReprByDepth, &bgcubesReprByDepthAdd, bgSpaceCubes,
            &csearch, depth, depth1Max, &responder, &searchProgress);
    moveCount = searchProgress.getBestMoves(moves);
    return searchProgress.isStopRequested();
}

static bool searchMovesQuickB(CubesReprByDepthAdd &cubesReprByDepthAdd,
        BGCubesReprByDepthAdd &bgcubesReprByDepthAdd,
        const cube &csearch, unsigned depth1Max, unsigned depthSearch, bool catchFirst,
        Responder &responder, unsigned movesMax, int &moveCount, std::string &moves)
{
    moveCount = -1;
    const CubesReprByDepth *cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depth1Max, responder);
    const SpaceReprCubes *bgSpaceCubes = getBGSpaceReprCubes(cubesReprByDepthAdd, depth1Max, responder);
    if( cubesReprByDepth == NULL || bgSpaceCubes == NULL )
        return true;
    unsigned depth = depthSearch - 2*depth1Max;
    QuickSearchProgress searchProgress((*cubesReprByDepth)[depth1Max].size(),
            depthSearch, catchFirst, movesMax);
    if( depth == 1 )
        runInThreadPool(searchMovesQuickTb1, cubesReprByDepth, &bgcubesReprByDepthAdd,
                bgSpaceCubes, &csearch, depth1Max, &responder, &searchProgress);
    else
        runInThreadPool(searchMovesQuickTb, cubesReprByDepth, &bgcubesReprByDepthAdd,
                bgSpaceCubes, &csearch, depth, depth1Max, &responder, &searchProgress);
    moveCount = searchProgress.getBestMoves(moves);
    return searchProgress.isStopRequested();
}

void searchMovesQuickCatchFirst(
        CubesReprByDepthAdd &cubesReprByDepthAdd,
        BGCubesReprByDepthAdd &bgcubesReprByDepthAdd,
        const cube &csearch, Responder &responder)
{
    unsigned depth1Max;
    {
        const CubesReprByDepth *cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(0, responder);
        const SpaceReprCubes *bgSpaceCubes = getBGSpaceReprCubes(cubesReprByDepthAdd, 0, responder);
        if( cubesReprByDepth == NULL || bgSpaceCubes == NULL )
            return;
        depth1Max = std::min(cubesReprByDepth->availCount(), bgSpaceCubes->availCount()) - 1;
        depth1Max = std::max(depth1Max, (unsigned)TWOPHASE_DEPTH1_CATCHFIRST_MAX);
    }
    for(unsigned depthSearch = 0;
            depthSearch <= 12 && depthSearch <= 3*TWOPHASE_DEPTH1_CATCHFIRST_MAX;
            ++depthSearch)
    {
        int moveCount;
        std::string bestMoves;
        bool isFinish;
        if( depthSearch <= 2 * depth1Max )
            isFinish = searchMovesQuickA(cubesReprByDepthAdd, bgcubesReprByDepthAdd, csearch,
                    depthSearch, true, responder, 999, moveCount, bestMoves);
        else
            isFinish = searchMovesQuickB(cubesReprByDepthAdd, bgcubesReprByDepthAdd, csearch,
                    TWOPHASE_DEPTH1_CATCHFIRST_MAX, depthSearch,
                    true, responder, 999, moveCount, bestMoves);
        if( moveCount >= 0 ) {
            responder.solution(bestMoves.c_str());
            return;
        }
        if( isFinish )
            return;
        responder.message("depth %u end", depthSearch);
    }
    responder.message("not found");
}

void searchMovesQuickMulti(
        CubesReprByDepthAdd &cubesReprByDepthAdd,
        BGCubesReprByDepthAdd &bgcubesReprByDepthAdd,
        const cube &csearch, Responder &responder)
{
    std::string movesBest;
    unsigned bestMoveCount = 999;
    for(unsigned depthSearch = 0;
            depthSearch <= 12 && depthSearch <= 3*TWOPHASE_DEPTH1_MULTI_MAX &&
            depthSearch < bestMoveCount;
            ++depthSearch)
    {
        int moveCount;
        std::string moves;
        bool isFinish;
        if( depthSearch <= 2 * TWOPHASE_DEPTH1_MULTI_MAX )
            isFinish = searchMovesQuickA(cubesReprByDepthAdd, bgcubesReprByDepthAdd,
                    csearch, depthSearch, false, responder, bestMoveCount-1,
                    moveCount, moves);
        else
            isFinish = searchMovesQuickB(cubesReprByDepthAdd, bgcubesReprByDepthAdd,
                    csearch, TWOPHASE_DEPTH1_MULTI_MAX, depthSearch,
                    false, responder, bestMoveCount-1, moveCount, moves);
        if( moveCount >= 0 ) {
            movesBest = moves;
            bestMoveCount = moveCount;
            if( moveCount == 0 )
                break;
        }
        if( isFinish )
            break;
        responder.message("depth %u end", depthSearch);
    }
    if( !movesBest.empty() ) {
        responder.solution(movesBest.c_str());
        return;
    }
    responder.message("not found");
}

