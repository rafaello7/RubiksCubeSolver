#include "cubesreprbg.h"
#include <algorithm>
#include <iostream>
#include <set>

unsigned BGCornerPermReprCubes::addCubes(const std::vector<cubeedges> &cearr)
{
    if( cearr.empty() )
        return 0;
    std::vector<std::pair<cubeedges, unsigned>> addedCubes;
    addedCubes.reserve(cearr.size());
    for(cubeedges ce : cearr) {
        std::vector<cubeedges>::iterator edgeIt = std::lower_bound(
                m_items.begin(), m_items.end(), ce);
        if( edgeIt == m_items.end() || *edgeIt != ce ) {
            unsigned idx = std::distance(m_items.begin(), edgeIt);
            addedCubes.push_back(std::make_pair(ce, idx));
        }
    }
    if( addedCubes.empty() )
        return 0;
    if( addedCubes.size() > 1 ) {
        std::sort(addedCubes.begin(), addedCubes.end());
        std::vector<std::pair<cubeedges, unsigned>>::iterator uniqEnd =
            std::unique(addedCubes.begin(), addedCubes.end());
        addedCubes.resize(std::distance(addedCubes.begin(), uniqEnd));
    }
    m_items.resize(m_items.size() + addedCubes.size());
    std::vector<cubeedges>::iterator destIt = m_items.end();
    std::vector<cubeedges>::iterator srcEndIt = destIt - addedCubes.size();
    for(std::vector<std::pair<cubeedges, unsigned>>::reverse_iterator addedIt =
            addedCubes.rbegin(); addedIt != addedCubes.rend(); ++addedIt)
    {
        std::vector<cubeedges>::iterator srcBegIt = m_items.begin() + addedIt->second;
        destIt = std::copy_backward(srcBegIt, srcEndIt, destIt);
        *--destIt = addedIt->first;
        srcEndIt = srcBegIt;
    }
    return addedCubes.size();
}

bool BGCornerPermReprCubes::containsCubeEdges(cubeedges ce) const
{
    edges_iter edgeIt = std::lower_bound(m_items.begin(), m_items.end(), ce);
    return edgeIt != m_items.end() && *edgeIt == ce;
}

BGCubesReprAtDepth::BGCubesReprAtDepth(const BGCubecornerReprPerms &reprPerms)
    : m_reprPerms(reprPerms), m_cornerPermReprCubes(reprPerms.reprPermCount())
{
}

BGCubesReprAtDepth::~BGCubesReprAtDepth() {
}

size_t BGCubesReprAtDepth::cubeCount() const {
    size_t res = 0;
    for(const BGCornerPermReprCubes &reprCube : m_cornerPermReprCubes)
        res += reprCube.size();
    return res;
}

BGCornerPermReprCubes &BGCubesReprAtDepth::add(unsigned idx) {
    return m_cornerPermReprCubes[idx];
}

cubecorners_perm BGCubesReprAtDepth::getPermAt(ccpcubes_iter it) const
{
    unsigned reprPermIdx = std::distance(m_cornerPermReprCubes.begin(), it);
    return m_reprPerms.getPermForIdx(reprPermIdx);
}

bool BGCubesReprAtDepth::containsCube(const cube &c) const {
    unsigned ccpReprSearchIdx = m_reprPerms.getReprPermIdx(c.ccp);
    const BGCornerPermReprCubes &ccpReprSearchCubes =
        m_cornerPermReprCubes[ccpReprSearchIdx];
    cubeedges ceSearchRepr = m_reprPerms.getReprCubeedges(c.ccp, c.ce);
    return ccpReprSearchCubes.containsCubeEdges(ceSearchRepr);
}

BGCubesReprByDepth::BGCubesReprByDepth(bool useReverse)
    : m_reprPerms(useReverse), m_availCount(1)
{
    // insert the solved cube at depth 0
    m_cubesAtDepths.emplace_back(new BGCubesReprAtDepth(m_reprPerms));
    unsigned cornerPermReprIdx = m_reprPerms.getReprPermIdx(csolved.ccp);
    BGCornerPermReprCubes &ccpCubes = m_cubesAtDepths[0]->add(cornerPermReprIdx);
    cubeedges ceRepr = m_reprPerms.getReprCubeedges(csolved.ccp, csolved.ce);
    std::vector<cubeedges> ceReprArr = { ceRepr };
    ccpCubes.addCubes(ceReprArr);
}

bool BGCubesReprByDepth::isUseReverse() const {
    return m_reprPerms.isUseReverse();
}

BGCubesReprAtDepth &BGCubesReprByDepth::operator[](unsigned idx)
{
    while( idx >= m_cubesAtDepths.size() )
        m_cubesAtDepths.emplace_back(new BGCubesReprAtDepth(m_reprPerms));
    return *m_cubesAtDepths[idx];
}

std::string BGCubesReprByDepth::printMoves(const cube &c,
        unsigned searchTd, bool movesRev) const
{
    static const unsigned reverseMoveIdxs[RCOUNTBG] = {
        0, 1, 2, 3, 6, 5, 4, 9, 8, 7
    };
    static const unsigned transformedMoves[2][RCOUNTBG] = {
        {
            BLUE180, GREEN180, ORANGE180, RED180,
            WHITECW, WHITE180, WHITECCW, YELLOWCW, YELLOW180, YELLOWCCW
        },{
            YELLOW180, WHITE180, BLUE180, GREEN180,
            REDCW, RED180, REDCCW, ORANGECW, ORANGE180, ORANGECCW
        },
    };
    std::vector<unsigned> rotateIdxs;
    std::vector<unsigned>::iterator insertPos = rotateIdxs.end();
    cube crepr = m_reprPerms.cubeRepresentative(c);
    unsigned ccpReprIdx = m_reprPerms.getReprPermIdx(crepr.ccp);
    unsigned depth = 0;
    while( true ) {
        const BGCornerPermReprCubes &ccpReprCubes = m_cubesAtDepths[depth]->getAt(ccpReprIdx);
        if( ccpReprCubes.containsCubeEdges(crepr.ce) )
            break;
        ++depth;
        if( depth >= m_availCount ) {
            std::cout << "printInSpaceMoves: depth reached maximum, cube NOT FOUND" << std::endl;
            exit(1);
        }
    }
    cube cc = c;
    while( depth-- > 0 ) {
        int cmidx = 0;
        cube ccRev = cc.reverse();
        cube cc1;
        while( cmidx < RCOUNTBG ) {
            unsigned cm = BGSpaceRotations[cmidx];
            cc1 = cube::compose(cc, crotated[cm]);
            cube cc1repr = m_reprPerms.cubeRepresentative(cc1);
            ccpReprIdx = m_reprPerms.getReprPermIdx(cc1repr.ccp);
            const BGCornerPermReprCubes &ccpReprCubes = m_cubesAtDepths[depth]->getAt(ccpReprIdx);
            if( ccpReprCubes.containsCubeEdges(cc1repr.ce) )
                break;
            cc1 = cube::compose(ccRev, crotated[cm]);
            cc1repr = m_reprPerms.cubeRepresentative(cc1);
            ccpReprIdx = m_reprPerms.getReprPermIdx(cc1repr.ccp);
            const BGCornerPermReprCubes &ccpReprCubesRev =
                m_cubesAtDepths[depth]->getAt(ccpReprIdx);
            if( ccpReprCubesRev.containsCubeEdges(cc1repr.ce) ) {
                movesRev = !movesRev;
                break;
            }
            ++cmidx;
        }
        if( cmidx == RCOUNTBG ) {
            std::cout << "printInSpaceMoves: cube at depth " << depth << " NOT FOUND" << std::endl;
            exit(1);
        }
        insertPos = rotateIdxs.insert(insertPos, movesRev ? cmidx : reverseMoveIdxs[cmidx]);
        if( movesRev )
            ++insertPos;
        cc = cc1;
    }
    std::string res;
    for(unsigned rotateIdx : rotateIdxs) {
        unsigned rotateDir = searchTd ?
            transformedMoves[searchTd-1][rotateIdx] :
            BGSpaceRotations[rotateIdx];
        res += " ";
        res += rotateDirName(rotateDir);
    }
    return res;
}

unsigned long BGCubesReprByDepth::addCubesForReprPerm(unsigned reprPermIdx, int depth)
{
    unsigned long cubeCount = 0;

    const BGCubesReprAtDepth &ccpReprCubesC = *m_cubesAtDepths[depth-1];
    const BGCornerPermReprCubes *ccpReprCubesNewP = depth == 1 ? nullptr :
        &m_cubesAtDepths[depth-2]->getAt(reprPermIdx);
    const BGCornerPermReprCubes &ccpReprCubesNewC = ccpReprCubesC.getAt(reprPermIdx);
    BGCornerPermReprCubes &ccpReprCubesNewN = m_cubesAtDepths[depth]->add(reprPermIdx);
    cubecorners_perm ccpNewRepr = m_reprPerms.getPermForIdx(reprPermIdx);
    std::set<cubecorners_perm> ccpChecked;
    for(unsigned trrev = 0; trrev < (m_reprPerms.isUseReverse() ? 2 : 1); ++trrev) {
        cubecorners_perm ccpNewReprRev = trrev ? ccpNewRepr.reverse() : ccpNewRepr;
        for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
            cubecorners_perm ccpNewS = symmetric ? ccpNewReprRev.symmetric() : ccpNewReprRev;
            for(unsigned tdidx = 0; tdidx < TCOUNTBG; ++tdidx) {
                unsigned short td = BGSpaceTransforms[tdidx];
                cubecorners_perm ccpNew = ccpNewS.transform(td);
                if( ccpChecked.find(ccpNew) == ccpChecked.end() ) {
                    ccpChecked.insert(ccpNew);
                    for(unsigned rdidx = 0; rdidx < RCOUNTBG; ++rdidx) {
                        unsigned rd = BGSpaceRotations[rdidx];
                        unsigned rdRev = rotateDirReverse(rd);
                        for(unsigned reversed = 0;
                                reversed < (m_reprPerms.isUseReverse() ? 2 : 1); ++reversed) {
                            cubecorners_perm ccp = reversed ?
                                cubecorners_perm::compose(crotated[rdRev].ccp, ccpNew) :
                                cubecorners_perm::compose(ccpNew, crotated[rdRev].ccp);
                            unsigned ccpReprIdx = m_reprPerms.getReprPermIdx(ccp);
                            if( m_reprPerms.getPermForIdx(ccpReprIdx) == ccp ) {
                                const BGCornerPermReprCubes &cpermReprCubesC =
                                    ccpReprCubesC.getAt(ccpReprIdx);
                                std::vector<cubeedges> ceNewArr;
                                for(BGCornerPermReprCubes::edges_iter edgeIt =
                                        cpermReprCubesC.edgeBegin();
                                        edgeIt != cpermReprCubesC.edgeEnd(); ++edgeIt)
                                {
                                    const cubeedges ce = *edgeIt;
                                    cubeedges cenew = reversed ?
                                        cubeedges::compose(crotated[rd].ce, ce) :
                                        cubeedges::compose(ce, crotated[rd].ce);
                                    cubeedges cenewRepr = m_reprPerms.getReprCubeedges(ccpNew, cenew);
                                    if( ccpReprCubesNewP != NULL &&
                                            ccpReprCubesNewP->containsCubeEdges(cenewRepr) )
                                        continue;
                                    if( ccpReprCubesNewC.containsCubeEdges(cenewRepr) )
                                        continue;
                                    ceNewArr.push_back(cenewRepr);
                                }
                                if( !ceNewArr.empty() )
                                    cubeCount += ccpReprCubesNewN.addCubes(ceNewArr);
                            }
                        }
                    }
                }
            }
        }
    }
    return cubeCount;
}

bool BGCubesReprByDepth::searchMovesForReprPerm(unsigned reprPermIdx,
            unsigned depth, unsigned depthMax, const cube &cSearchT,
            bool reversed, cube &c, cube &cSearch) const
{
    const BGCornerPermReprCubes &ccpReprCubes = m_cubesAtDepths[depth]->getAt(reprPermIdx);
    cubecorners_perm ccp = m_reprPerms.getPermForIdx(reprPermIdx);
    if( !ccpReprCubes.empty() ) {
        cubecorners_perm ccpSearch = reversed ?
            cubecorners_perm::compose(cSearchT.ccp, ccp) :
            cubecorners_perm::compose(ccp, cSearchT.ccp);
        unsigned ccpReprSearchIdx = m_reprPerms.getReprPermIdx(ccpSearch);
        const BGCornerPermReprCubes &ccpReprSearchCubes =
            m_cubesAtDepths[depthMax]->getAt(ccpReprSearchIdx);
        if( ccpReprCubes.size() <= ccpReprSearchCubes.size() ||
                !m_reprPerms.isSingleTransform(ccpSearch) )
        {
            for(BGCornerPermReprCubes::edges_iter edgeIt = ccpReprCubes.edgeBegin();
                    edgeIt != ccpReprCubes.edgeEnd(); ++edgeIt)
            {
                cubeedges ce = *edgeIt;
                cubeedges ceSearch = reversed ?
                    cubeedges::compose(cSearchT.ce, ce) :
                    cubeedges::compose(ce, cSearchT.ce);
                cubeedges ceSearchRepr = m_reprPerms.getReprCubeedges(ccpSearch, ceSearch);
                if( ccpReprSearchCubes.containsCubeEdges(ceSearchRepr) ) {
                    cSearch = { .ccp = ccpSearch, .cco = csolved.cco, .ce = ceSearch };
                    c = { .ccp = ccp, .cco = csolved.cco, .ce = ce };
                    return true;
                }
            }
        }else{
            cube cSearchTrev = cSearchT.reverse();
            for(BGCornerPermReprCubes::edges_iter edgeIt = ccpReprSearchCubes.edgeBegin();
                    edgeIt != ccpReprSearchCubes.edgeEnd(); ++edgeIt)
            {
                cubeedges ceSearchRepr = *edgeIt;
                cubeedges ceSearch = m_reprPerms.getCubeedgesForRepresentative(ccpSearch, ceSearchRepr);
                cubeedges ce = reversed ?
                    cubeedges::compose(cSearchTrev.ce, ceSearch) :
                    cubeedges::compose(ceSearch, cSearchTrev.ce);
                if( ccpReprCubes.containsCubeEdges(ce) ) {
                    cSearch = { .ccp = ccpSearch, .cco = csolved.cco, .ce = ceSearch };
                    c = { .ccp = ccp, .cco = csolved.cco, .ce = ce };
                    return true;
                }
            }
        }
    }
    return false;
}

