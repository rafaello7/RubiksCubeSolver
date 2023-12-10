#include "cubesreprbg.h"
#include <algorithm>
#include <iostream>

const unsigned BGSpaceRotations[RCOUNTBG] = {
    ORANGE180, RED180, YELLOW180, WHITE180,
    GREENCW, GREEN180, GREENCCW,
    BLUECW, BLUE180, BLUECCW
};

const unsigned BGSpaceTransforms[TCOUNTBG] = {
    TD_0, TD_BG_CW, TD_BG_180, TD_BG_CCW,
    TD_YW_180, TD_OR_180, TD_E4_7, TD_E5_6
};

struct ReprCandidateTransform {
    bool reversed;
    bool symmetric;
    unsigned short transformIdx;
};

struct CubecornerPermRepr {
    cubecorners_perm ccp;
    std::vector<unsigned short> represented;
};

struct CubecornerPermToRepr {
    int reprIdx = -1;       // index in gReprPerms
    std::vector<ReprCandidateTransform> transform;
};

static bool USEREVERSE;
static std::vector<CubecornerPermRepr> gBGSpaceReprPerms;
static std::vector<CubecornerPermToRepr> gBGSpacePermToRepr(40320);

void bgspacePermReprInit(bool useReverse)
{
    USEREVERSE = useReverse;
    gBGSpaceReprPerms.reserve(USEREVERSE ? 1672 : 2768);
    for(unsigned pidx = 0; pidx < 40320; ++pidx) {
        cubecorners_perm perm = cubecorners_perm::fromPermIdx(pidx);
        cubecorners_perm permRepr;
        std::vector<ReprCandidateTransform> transform;
        for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
            cubecorners_perm permr = reversed ? perm.reverse() : perm;
            for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
                cubecorners_perm permchk = symmetric ? permr.symmetric() : permr;
                for(unsigned short tdidx = 0; tdidx < TCOUNTBG; ++tdidx) {
                    unsigned short td = BGSpaceTransforms[tdidx];
                    cubecorners_perm cand = permchk.transform(td);
                    if( tdidx == 0 && reversed == 0  && symmetric == 0 || cand < permRepr ) {
                        permRepr = cand;
                        transform.clear();
                        transform.push_back({ .reversed = (bool)reversed,
                                .symmetric = (bool)symmetric, .transformIdx = td });
                    }else if( cand == permRepr ) {
                        transform.push_back({ .reversed = (bool)reversed,
                                .symmetric = (bool)symmetric, .transformIdx = td });
                    }
                }
            }
        }
        CubecornerPermToRepr &permToReprRepr = gBGSpacePermToRepr[permRepr.getPermIdx()];
        if( permToReprRepr.reprIdx < 0 ) {
            permToReprRepr.reprIdx = gBGSpaceReprPerms.size();
            gBGSpaceReprPerms.push_back({ .ccp = permRepr });
        }
        gBGSpaceReprPerms[permToReprRepr.reprIdx].represented.push_back(pidx);
        CubecornerPermToRepr &permToRepr = gBGSpacePermToRepr[perm.getPermIdx()];
        permToRepr.reprIdx = permToReprRepr.reprIdx;
        permToRepr.transform.swap(transform);
    }
    std::cout << "bg repr size=" << gBGSpaceReprPerms.size() << std::endl;
}

unsigned inbgspaceCubecornerPermRepresentativeIdx(cubecorners_perm ccp)
{
    return gBGSpacePermToRepr.at(ccp.getPermIdx()).reprIdx;
}

static cubecorners_perm inbgspaceCubecornerPermsRepresentative(cubecorners_perm ccp)
{
    unsigned permReprIdx = gBGSpacePermToRepr.at(ccp.getPermIdx()).reprIdx;
    return gBGSpaceReprPerms[permReprIdx].ccp;
}

cubeedges inbgspaceCubeedgesRepresentative(cubecorners_perm ccp, cubeedges ce)
{
    CubecornerPermToRepr permToRepr = gBGSpacePermToRepr.at(ccp.getPermIdx());
    cubeedges erepr, cesymm, cerev, cerevsymm;
    bool isInit = false, isSymmInit = false, isRevInit = false, isRevSymmInit = false;
    for(const ReprCandidateTransform &rct : permToRepr.transform) {
        cubeedges ecand;
        if( rct.reversed ) {
            if( !isRevInit ) {
                cerev = ce.reverse();
                isRevInit = true;
            }
            if( rct.symmetric ) {
                if( !isRevSymmInit ) {
                    cerevsymm = cerev.symmetric();
                    isRevSymmInit = true;
                }
                ecand = cerevsymm.transform(rct.transformIdx);
            }else{
                ecand = cerev.transform(rct.transformIdx);
            }
        }else{
            if( rct.symmetric ) {
                if( !isSymmInit ) {
                    cesymm = ce.symmetric();
                    isSymmInit = true;
                }
                ecand = cesymm.transform(rct.transformIdx);
            }else{
                ecand = ce.transform(rct.transformIdx);
            }
        }
        if( !isInit || ecand < erepr ) {
            erepr = ecand;
            isInit = true;
        }
    }
    return erepr;
}

static cube inbgspaceCubeRepresentative(const cube &c) {
    cubecorners_perm ccpRepr = inbgspaceCubecornerPermsRepresentative(c.ccp);
    cubeedges ceRepr = inbgspaceCubeedgesRepresentative(c.ccp, c.ce);
    return { .ccp = ccpRepr, .cco = csolved.cco, .ce = ceRepr };
}

bool inbgspaceIsSingleTransform(cubecorners_perm ccp) {
    return gBGSpacePermToRepr[ccp.getPermIdx()].transform.size() == 1;
}

cubeedges inbgspaceCubeedgesForRepresentative(cubecorners_perm ccpSearch,
        cubeedges ceSearchRepr)
{
    const CubecornerPermToRepr &permToRepr = gBGSpacePermToRepr.at(ccpSearch.getPermIdx());
    const ReprCandidateTransform &rct = permToRepr.transform.front();
    // sequence was: ccpSearch reverse, then symmetric, then transform
    unsigned transformIdxRev = transformReverse(rct.transformIdx);
    cubeedges ceSearchRevSymm = ceSearchRepr.transform(transformIdxRev);
    cubeedges ceSearchRev = rct.symmetric ? ceSearchRevSymm.symmetric() : ceSearchRevSymm;
    cubeedges ceSearch = rct.reversed ? ceSearchRev.reverse() : ceSearchRev;
    return ceSearch;
}

unsigned BGSpaceCornerPermReprCubes::addCubes(const std::vector<cubeedges> &cearr)
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

bool BGSpaceCornerPermReprCubes::containsCubeEdges(cubeedges ce) const
{
    edges_iter edgeIt = std::lower_bound(m_items.begin(), m_items.end(), ce);
    return edgeIt != m_items.end() && *edgeIt == ce;
}

unsigned BGSpaceCubesReprAtDepth::size() {
    return USEREVERSE ? 1672 : 2768;
}

BGSpaceCubesReprAtDepth::BGSpaceCubesReprAtDepth()
    : m_cornerPermReprCubes(size())
{
    for(unsigned i = 0; i < m_cornerPermReprCubes.size(); ++i)
        m_cornerPermReprCubes[i].first = gBGSpaceReprPerms[i].ccp;
}

BGSpaceCubesReprAtDepth::~BGSpaceCubesReprAtDepth() {
}

size_t BGSpaceCubesReprAtDepth::cubeCount() const {
    size_t res = 0;
    for(const std::pair<cubecorners_perm, BGSpaceCornerPermReprCubes> &reprCube : m_cornerPermReprCubes)
        res += reprCube.second.size();
    return res;
}

BGSpaceCornerPermReprCubes &BGSpaceCubesReprAtDepth::add(unsigned idx) {
    return m_cornerPermReprCubes[idx].second;
}

bool BGSpaceCubesReprByDepth::isUseReverse() {
    return USEREVERSE;
}

std::string printInSpaceMoves(
        const BGSpaceCubesReprByDepth &cubesByDepth, const cube &c,
        unsigned searchTd, bool movesRev)
{
    static const unsigned reverseMoveIdxs[RCOUNTBG] = {
        0, 1, 2, 3, 6, 5, 4, 9, 8, 7
    };
    static const unsigned transformedMoves[2][RCOUNTBG] = {
        {
            YELLOW180, WHITE180, BLUE180, GREEN180,
            REDCW, RED180, REDCCW, ORANGECW, ORANGE180, ORANGECCW
        },{
            BLUE180, GREEN180, ORANGE180, RED180,
            WHITECW, WHITE180, WHITECCW, YELLOWCW, YELLOW180, YELLOWCCW
        },
    };
    std::vector<unsigned> rotateIdxs;
    std::vector<unsigned>::iterator insertPos = rotateIdxs.end();
    cube crepr = inbgspaceCubeRepresentative(c);
    unsigned ccpReprIdx = inbgspaceCubecornerPermRepresentativeIdx(crepr.ccp);
    unsigned depth = 0;
    while( true ) {
        const BGSpaceCornerPermReprCubes &ccpReprCubes = cubesByDepth[depth].getAt(ccpReprIdx);
        if( ccpReprCubes.containsCubeEdges(crepr.ce) )
            break;
        ++depth;
        if( depth >= cubesByDepth.availCount() ) {
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
            cube cc1repr = inbgspaceCubeRepresentative(cc1);
            ccpReprIdx = inbgspaceCubecornerPermRepresentativeIdx(cc1repr.ccp);
            const BGSpaceCornerPermReprCubes &ccpReprCubes = cubesByDepth[depth].getAt(ccpReprIdx);
            if( ccpReprCubes.containsCubeEdges(cc1repr.ce) )
                break;
            cc1 = cube::compose(ccRev, crotated[cm]);
            cc1repr = inbgspaceCubeRepresentative(cc1);
            ccpReprIdx = inbgspaceCubecornerPermRepresentativeIdx(cc1repr.ccp);
            const BGSpaceCornerPermReprCubes &ccpReprCubesRev = cubesByDepth[depth].getAt(ccpReprIdx);
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

unsigned long addInSpaceCubesForReprPerm(
        BGSpaceCubesReprByDepth *cubesReprByDepth, unsigned permReprIdx,
        int depth)
{
    unsigned long cubeCount = 0;

    const BGSpaceCubesReprAtDepth &ccpReprCubesC = (*cubesReprByDepth)[depth-1];
    const BGSpaceCornerPermReprCubes *ccpReprCubesNewP = depth == 1 ? nullptr :
        &(*cubesReprByDepth)[depth-2].getAt(permReprIdx);
    const BGSpaceCornerPermReprCubes &ccpReprCubesNewC = ccpReprCubesC.getAt(permReprIdx);
    BGSpaceCornerPermReprCubes &ccpReprCubesNewN = (*cubesReprByDepth)[depth].add(permReprIdx);
    for(unsigned permIdx : gBGSpaceReprPerms[permReprIdx].represented) {
        cubecorners_perm ccpNew = cubecorners_perm::fromPermIdx(permIdx);
        for(unsigned rdidx = 0; rdidx < RCOUNTBG; ++rdidx) {
            unsigned rd = BGSpaceRotations[rdidx];
            unsigned rdRev = rotateDirReverse(rd);
            for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
                cubecorners_perm ccp = reversed ?
                    cubecorners_perm::compose(crotated[rdRev].ccp, ccpNew) :
                    cubecorners_perm::compose(ccpNew, crotated[rdRev].ccp);
                unsigned ccpReprIdx = inbgspaceCubecornerPermRepresentativeIdx(ccp);
                if( gBGSpaceReprPerms[ccpReprIdx].ccp == ccp ) {
                    const BGSpaceCornerPermReprCubes &cpermReprCubesC = ccpReprCubesC.getAt(ccpReprIdx);
                    std::vector<cubeedges> ceNewArr;
                    for(BGSpaceCornerPermReprCubes::edges_iter edgeIt = cpermReprCubesC.edgeBegin();
                            edgeIt != cpermReprCubesC.edgeEnd(); ++edgeIt)
                    {
                        const cubeedges ce = *edgeIt;
                        cubeedges cenew = reversed ?
                            cubeedges::compose(crotated[rd].ce, ce) :
                            cubeedges::compose(ce, crotated[rd].ce);
                        cubeedges cenewRepr = inbgspaceCubeedgesRepresentative(ccpNew, cenew);
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
    return cubeCount;
}

