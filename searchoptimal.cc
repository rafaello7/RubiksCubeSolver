#include "searchoptimal.h"
#include "progressbase.h"
#include "cubesadd.h"
#include "threadpoolhelper.h"
#include <algorithm>
#include <sstream>

struct SearchIndexes {
    unsigned permReprIdx;
    bool reversed;
    bool symmetric;
    unsigned td;
};

class SearchProgress : public ProgressBase {
    const unsigned m_depth;
    const bool m_useReverse;
    const unsigned m_itemCount;
    unsigned m_nextItemIdx;
    unsigned m_runningThreadCount;
    bool m_isFinish;

public:
    SearchProgress(unsigned depth, bool useReverse)
        : m_depth(depth), m_useReverse(useReverse),
        m_itemCount((useReverse? 2 * 654 : 984) * 2 * TCOUNT),
        m_nextItemIdx(0), m_runningThreadCount(THREAD_COUNT), m_isFinish(false)
    {
    }

    bool inc(Responder &responder, SearchIndexes*);
    bool isFinish() const { return m_isFinish; }
    std::string progressStr();
};

bool SearchProgress::inc(Responder &responder, SearchIndexes *indexesBuf)
{
    bool res;
    unsigned itemIdx;
    mutexLock();
    if( indexesBuf == NULL )
        m_isFinish = true;
    res = !m_isFinish && m_nextItemIdx < m_itemCount && !isStopRequestedNoLock();
    if( res )
        itemIdx = m_nextItemIdx++;
    else
        --m_runningThreadCount;
    mutexUnlock();
    if( res ) {
        indexesBuf->td = itemIdx % TCOUNT;
        unsigned itemIdxDiv = itemIdx / TCOUNT;
        indexesBuf->symmetric = itemIdxDiv & 1;
        itemIdxDiv >>= 1;
        if( m_useReverse ) {
            indexesBuf->reversed = itemIdxDiv & 1;
            itemIdxDiv >>= 1;
        }else
            indexesBuf->reversed = 0;
        indexesBuf->permReprIdx = itemIdxDiv;
        if( m_depth >= 17 ) {
            unsigned procCountNext = 100 * (itemIdx+1) / m_itemCount;
            unsigned procCountCur = 100 * itemIdx / m_itemCount;
            if( procCountNext != procCountCur && (m_depth>=18 || procCountCur % 10 == 0) )
                responder.progress("depth %d search %d%%",
                        m_depth, 100 * itemIdx / m_itemCount);
        }
    }else{
        if( m_depth >= 17 ) {
            responder.progress("depth %d search %d threads still running",
                    m_depth, m_runningThreadCount);
        }
    }
    return res;
}

std::string SearchProgress::progressStr()
{
    std::ostringstream res;
    res << 100 * m_nextItemIdx / m_itemCount << "%";
    return res.str();
}

static std::string getMovesForMatch(const CubesReprByDepth &cubesReprByDepth,
        const cube &cSearch, const cube &c, unsigned searchRev, unsigned searchTd,
        unsigned reversed, unsigned symmetric, unsigned td)
{
    // cube found:
    //  if reversed: cSearch = transform(symmetric((csearch rev))) ⊙  c
    //      (csearch rev) = symmetric(transformReverse(cSearch)) ⊙  symmetric(transformReverse(c rev))
    //  if not: cSearch = c ⊙  transform(symmetric(csearch))
    //      (csearch rev) = (symmetric(transformReverse(cSearch)) rev) ⊙  (symmetric(transformReverse(c rev)) rev)
    cube cSearchT = cSearch.transform(transformReverse(td));
    cube cSearchTsymm = symmetric ? cSearchT.symmetric() : cSearchT;
    cube cT = c.transform(transformReverse(td));
    cube cTsymm = symmetric ? cT.symmetric() : cT;
    if( searchTd ) {
        cSearchTsymm = cSearchTsymm.transform(transformReverse(searchTd));
        cTsymm = cTsymm.transform(transformReverse(searchTd));
    }
    std::string moves;
    if( searchRev ) {
        moves = cubesReprByDepth.getMoves(cTsymm, !reversed);
        moves += cubesReprByDepth.getMoves(cSearchTsymm, reversed);
    }else{
        moves = cubesReprByDepth.getMoves(cSearchTsymm, !reversed);
        moves += cubesReprByDepth.getMoves(cTsymm, reversed);
    }
    return moves;
}

static void generateSearchTarr(const cube &csearch, bool useReverse, cube cSearchTarr[2][2][TCOUNT])
{
    for(unsigned reversed = 0; reversed < (useReverse ? 2 : 1); ++reversed) {
        cube csearchrev = reversed ? csearch.reverse() : csearch;
        for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
            cube csearchrevsymm = symmetric ? csearchrev.symmetric() : csearchrev;
            for(unsigned td = 0; td < TCOUNT; ++td)
                cSearchTarr[reversed][symmetric][td] =
                    csearchrevsymm.transform(td);
        }
    }
}

static bool searchMovesForIdxs(const CubesReprByDepth &cubesReprByDepth,
        unsigned depth, unsigned depthMax, const cube cSearchTarr[2][2][TCOUNT],
        const SearchIndexes &indexes, std::string &moves,
        unsigned searchRev, unsigned searchTd)
{
    const cube &cSearchT = cSearchTarr[indexes.reversed][indexes.symmetric][indexes.td];
    cube c, cSearch;
    if( cubesReprByDepth.searchMovesForReprPerm(indexes.permReprIdx,
            depth, depthMax, cSearchT, indexes.reversed, c, cSearch) )
    {
        moves = getMovesForMatch(cubesReprByDepth, cSearch, c, searchRev,
                searchTd, indexes.reversed, indexes.symmetric, indexes.td);
        return true;
    }

    return false;
}

static void searchMovesTa(unsigned threadNo,
        const CubesReprByDepth *cubesReprByDepth,
        unsigned depth, unsigned depthMax,
        const cube cSearchTarr[2][2][TCOUNT], Responder *responder,
        SearchProgress *searchProgress)
{
    SearchIndexes indexes;

    while( searchProgress->inc(*responder, &indexes) ) {
        std::string moves;
        if( searchMovesForIdxs(*cubesReprByDepth, depth, depthMax, cSearchTarr, indexes, moves, 0, 0) ) {
            responder->solution(moves.c_str());
            searchProgress->inc(*responder, NULL);
            return;
        }
    }
}

static void searchMovesTb(unsigned threadNo,
        const CubesReprByDepth *cubesReprByDepth,
		int depth, unsigned depthMax, const cube *csearch,
        Responder *responder, SearchProgress *searchProgress)
{
    const CubesReprAtDepth &ccReprCubesC = (*cubesReprByDepth)[depth];
    std::vector<CubesReprAtDepth::ccpcubes_iter> ccp1FilledIters;
    for(CubesReprAtDepth::ccpcubes_iter ccpCubes1It = ccReprCubesC.ccpCubesBegin();
            ccpCubes1It != ccReprCubesC.ccpCubesEnd(); ++ccpCubes1It)
    {
        if( !ccpCubes1It->empty() )
            ccp1FilledIters.push_back(ccpCubes1It);
    }

    SearchIndexes indexes2;
	while( searchProgress->inc(*responder, &indexes2) ) {
        CubesReprAtDepth::ccpcubes_iter cornerPerm2It =
            (*cubesReprByDepth)[depthMax].ccpCubesBegin() + indexes2.permReprIdx;
        const CornerPermReprCubes &ccpReprCubes2 = *cornerPerm2It;
        if( ccpReprCubes2.empty() )
            continue;
        for(CubesReprAtDepth::ccpcubes_iter ccpCubes1It : ccp1FilledIters) {
            const CornerPermReprCubes &ccpCubes1 = *ccpCubes1It;
            cubecorners_perm ccp1 = ccReprCubesC.getPermAt(ccpCubes1It);
            for(CornerPermReprCubes::ccocubes_iter ccoCubes1It = ccpCubes1.ccoCubesBegin();
                    ccoCubes1It != ccpCubes1.ccoCubesEnd(); ++ccoCubes1It)
            {
                const CornerOrientReprCubes &ccoCubes1 = *ccoCubes1It;
                cubecorner_orients cco1 = ccoCubes1.getOrients();
                for(CornerOrientReprCubes::edges_iter edge1It = ccoCubes1.edgeBegin();
                        edge1It != ccoCubes1.edgeEnd(); ++edge1It)
                {
                    const cubeedges ce1 = *edge1It;
                    cube c1 = { .ccp = ccp1, .cco = cco1, .ce = ce1 };
                    std::vector<cube> cubesChecked;
                    for(unsigned reversed1 = 0;
                            reversed1 < (cubesReprByDepth->isUseReverse() ? 2 : 1); ++reversed1)
                    {
                        cube c1r = reversed1 ? c1.reverse() : c1;
                        for(unsigned symmetric1 = 0; symmetric1 < 2; ++symmetric1) {
                            cube c1rs = symmetric1 ? c1r.symmetric() : c1r;
                            for(unsigned td1 = 0; td1 < TCOUNT; ++td1) {
                                const cube c1T = c1rs.transform(td1);
                                bool isDup = std::find(cubesChecked.begin(),
                                        cubesChecked.end(), c1T) != cubesChecked.end();
                                if( isDup )
                                    continue;
                                cubesChecked.push_back(c1T);
                                cube cSearch1 = cube::compose(c1T, *csearch);
                                cube cSearchTarr[2][2][TCOUNT];
                                generateSearchTarr(cSearch1,
                                        cubesReprByDepth->isUseReverse(), cSearchTarr);
                                std::string moves2;
                                if( searchMovesForIdxs(*cubesReprByDepth, depthMax,
                                            depthMax, cSearchTarr, indexes2, moves2, 0, 0) )
                                {
                                    std::string moves = moves2;
                                    moves += cubesReprByDepth->getMoves(c1T);
                                    responder->solution(moves.c_str());
                                    searchProgress->inc(*responder, NULL);
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
	}
}

static bool searchMovesOptimalA(CubesReprByDepthAdd &cubesReprByDepthAdd,
        const cube &csearch, unsigned depthSearch, Responder &responder)
{
    const CubesReprByDepth *cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(0, responder);
    if( cubesReprByDepth == NULL )
        return true;
    unsigned depthsAvail = cubesReprByDepth->availCount() - 1;
    unsigned depth, depthMax;
    if( depthSearch <= depthsAvail ) {
        depth = 0;
        depthMax = depthSearch;
    }else if( depthSearch <= 2 * depthsAvail ) {
        depth = depthSearch - depthsAvail;
        depthMax = depthsAvail;
    }else{
        depth = depthSearch / 2;
        depthMax = depthSearch - depth;
        cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depthMax, responder);
        if( cubesReprByDepth == NULL )
            return true;
    }
    cube cSearchTarr[2][2][TCOUNT];
    generateSearchTarr(csearch, cubesReprByDepth->isUseReverse(), cSearchTarr);
    SearchProgress searchProgress(depthSearch, cubesReprByDepth->isUseReverse());
    runInThreadPool(searchMovesTa, cubesReprByDepth,
                depth, depthMax, cSearchTarr, &responder, &searchProgress);
    if( searchProgress.isFinish() ) {
        responder.movecount("%u", depthSearch);
        responder.message("finished at %s", searchProgress.progressStr().c_str());
        return true;
    }
    bool isStopRequested = ProgressBase::isStopRequested();
    if( isStopRequested )
        responder.message("canceled");
    else
        responder.message("depth %d end", depthSearch);
    return isStopRequested;
}

static bool searchMovesOptimalB(CubesReprByDepthAdd &cubesReprByDepthAdd,
        const cube &csearch, unsigned depth, unsigned depthMax,
        Responder &responder)
{
    const CubesReprByDepth *cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depthMax, responder);
    if( cubesReprByDepth == NULL )
        return true;
    SearchProgress searchProgress(2*depthMax+depth, cubesReprByDepth->isUseReverse());
    runInThreadPool(searchMovesTb, cubesReprByDepth, depth, depthMax, &csearch,
                &responder, &searchProgress);
    if( searchProgress.isFinish() ) {
        responder.movecount("%u", 2*depthMax+depth);
        responder.message("finished at %s", searchProgress.progressStr().c_str());
        return true;
    }
    bool isStopRequested = ProgressBase::isStopRequested();
    if( isStopRequested )
        responder.message("canceled");
    else
        responder.message("depth %d end", 2*depthMax+depth);
    return isStopRequested;
}

void searchMovesOptimal(CubesReprByDepthAdd &cubesReprByDepthAdd,
        const cube &csearch, unsigned depthMax, Responder &responder)
{
    for(unsigned depthSearch = 0; depthSearch <= 2*depthMax; ++depthSearch) {
        if( searchMovesOptimalA(cubesReprByDepthAdd, csearch, depthSearch, responder) )
            return;
    }
    for(unsigned depth = 1; depth <= depthMax; ++depth) {
        if( searchMovesOptimalB(cubesReprByDepthAdd, csearch, depth, depthMax, responder) )
            return;
    }
    responder.message("not found");
}

