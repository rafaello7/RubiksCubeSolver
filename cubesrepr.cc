#include "cubesrepr.h"
#include <iostream>
#include <algorithm>

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
static std::vector<CubecornerPermRepr> gReprPerms;
static std::vector<CubecornerPermToRepr> gPermToRepr(40320);

void permReprInit(bool useReverse)
{
    USEREVERSE = useReverse;
    gReprPerms.reserve(USEREVERSE ? 654 : 984);
    for(unsigned pidx = 0; pidx < 40320; ++pidx) {
        cubecorners_perm perm = cubecorners_perm::fromPermIdx(pidx);
        cubecorners_perm permRepr;
        std::vector<ReprCandidateTransform> transform;
        for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
            cubecorners_perm permr = reversed ? perm.reverse() : perm;
            for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
                cubecorners_perm permchk = symmetric ? permr.symmetric() : permr;
                for(unsigned short td = 0; td < TCOUNT; ++td) {
                    cubecorners_perm cand = permchk.transform(td);
                    if( td+reversed+symmetric == 0 || cand < permRepr ) {
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
        CubecornerPermToRepr &permToReprRepr = gPermToRepr[permRepr.getPermIdx()];
        if( permToReprRepr.reprIdx < 0 ) {
            permToReprRepr.reprIdx = gReprPerms.size();
            gReprPerms.push_back({ .ccp = permRepr });
        }
        gReprPerms[permToReprRepr.reprIdx].represented.push_back(pidx);
        CubecornerPermToRepr &permToRepr = gPermToRepr[perm.getPermIdx()];
        permToRepr.reprIdx = permToReprRepr.reprIdx;
        permToRepr.transform.swap(transform);
    }
    std::cout << "repr size=" << gReprPerms.size() << std::endl;
}

unsigned cubecornerPermRepresentativeIdx(cubecorners_perm ccp)
{
    return gPermToRepr.at(ccp.getPermIdx()).reprIdx;
}

cubecorners_perm cubecornerPermsRepresentative(cubecorners_perm ccp)
{
    unsigned permReprIdx = gPermToRepr.at(ccp.getPermIdx()).reprIdx;
    return gReprPerms[permReprIdx].ccp;
}

cubecorner_orients cubecornerOrientsRepresentative(cubecorners_perm ccp, cubecorner_orients cco,
         std::vector<EdgeReprCandidateTransform> &transform)
{
    CubecornerPermToRepr permToRepr = gPermToRepr.at(ccp.getPermIdx());
    cubecorners_perm ccpsymm, ccprev, ccprevsymm;
    cubecorner_orients orepr, ccosymm, ccorev, ccorevsymm;
    bool isInit = false, isSymmInit = false, isRevInit = false, isRevSymmInit = false;
	transform.clear();
    for(const ReprCandidateTransform &rct : permToRepr.transform) {
        cubecorner_orients ocand;
        if( rct.reversed ) {
            if( !isRevInit ) {
                ccprev = ccp.reverse();
                ccorev = cco.reverse(ccp);
                isRevInit = true;
            }
            if( rct.symmetric ) {
                if( !isRevSymmInit ) {
                    ccprevsymm = ccprev.symmetric();
                    ccorevsymm = ccorev.symmetric();
                    isRevSymmInit = true;
                }
                ocand = ccorevsymm.transform(ccprevsymm, rct.transformIdx);
            }else{
                ocand = ccorev.transform(ccprev, rct.transformIdx);
            }
        }else{
            if( rct.symmetric ) {
                if( !isSymmInit ) {
                    ccpsymm = ccp.symmetric();
                    ccosymm = cco.symmetric();
                    isSymmInit = true;
                }
                ocand = ccosymm.transform(ccpsymm, rct.transformIdx);
            }else{
                ocand = cco.transform(ccp, rct.transformIdx);
            }
        }
        if( !isInit || ocand < orepr ) {
            orepr = ocand;
            transform.clear();
            transform.push_back({ .transformedIdx = rct.transformIdx,
                    .reversed = rct.reversed, .symmetric = rct.symmetric, .ceTrans = csolved.ce });
            isInit = true;
        }else if( isInit && ocand == orepr )
            transform.push_back({ .transformedIdx = rct.transformIdx,
                    .reversed = rct.reversed, .symmetric = rct.symmetric, .ceTrans = csolved.ce });
    }
    return orepr;
}

cubeedges cubeedgesRepresentative(cubeedges ce,
        const std::vector<EdgeReprCandidateTransform> &transform)
{
    cubeedges cerepr;

    if( transform.size() == 1 ) {
        const EdgeReprCandidateTransform &erct = transform.front();
        cubeedges cechk = erct.reversed ? ce.reverse() : ce;
        if( erct.symmetric )
            cechk = cechk.symmetric();
        cerepr = cechk.transform(erct.transformedIdx);
    }else{
        cubeedges cesymm, cerev, cerevsymm;
        bool isInit = false, isSymmInit = false, isRevInit = false, isRevSymmInit = false;
        for(const EdgeReprCandidateTransform &erct : transform) {
            cubeedges cand;
            if( erct.reversed ) {
                if( ! isRevInit ) {
                    cerev = ce.reverse();
                    isRevInit = true;
                }
                if( erct.symmetric ) {
                    if( ! isRevSymmInit ) {
                        cerevsymm = cerev.symmetric();
                        isRevSymmInit = true;
                    }
                    cand = cerevsymm.transform(erct.transformedIdx);
                }else{
                    cand = cerev.transform(erct.transformedIdx);
                }
            }else{
                if( erct.symmetric ) {
                    if( ! isSymmInit ) {
                        cesymm = ce.symmetric();
                        isSymmInit = true;
                    }
                    cand = cesymm.transform(erct.transformedIdx);
                }else{
                    cand = ce.transform(erct.transformedIdx);
                }
            }
            if( !isInit || cand < cerepr )
                cerepr = cand;
            isInit = true;
        }
    }
    return cerepr;
}

/* Gets the representative orients of cco.
 * Assumes the ccp,cco are cubecorners of a cube compose of two cubes, c1 ⊙  c2
 *
 * Parameters:
 *    ccp, cco      - cubecorners of the c1.cc composed with c2.cc
 *    reverse       - whether the ccp,cco is reversed
 *    ce2           - c2.ce
 *    transform     - output list to pass later to cubeedgesComposedRepresentative()
 *                    along with c1.ce, to get representative cubeedges of
 *                    c1.ce ⊙  ce2
 */
cubecorner_orients cubecornerOrientsComposedRepresentative(
        cubecorners_perm ccp, cubecorner_orients cco, bool reverse,
        cubeedges ce2, std::vector<EdgeReprCandidateTransform> &transform)
{
    const CubecornerPermToRepr &permToRepr = gPermToRepr.at(ccp.getPermIdx());
    cubecorners_perm ccpsymm, ccprev, ccprevsymm;
    cubecorner_orients orepr, ccosymm, ccorev, ccorevsymm;
    bool isInit = false, isSymmInit = false, isRevInit = false, isRevSymmInit = false;
	transform.clear();
    for(const ReprCandidateTransform &rct : permToRepr.transform) {
        cubecorner_orients ocand;
        if( rct.reversed ) {
            if( !isRevInit ) {
                ccprev = ccp.reverse();
                ccorev = cco.reverse(ccp);
                isRevInit = true;
            }
            if( rct.symmetric ) {
                if( !isRevSymmInit ) {
                    ccprevsymm = ccprev.symmetric();
                    ccorevsymm = ccorev.symmetric();
                    isRevSymmInit = true;
                }
                ocand = ccorevsymm.transform(ccprevsymm, rct.transformIdx);
            }else{
                ocand = ccorev.transform(ccprev, rct.transformIdx);
            }
        }else{
            if( rct.symmetric ) {
                if( !isSymmInit ) {
                    ccpsymm = ccp.symmetric();
                    ccosymm = cco.symmetric();
                    isSymmInit = true;
                }
                ocand = ccosymm.transform(ccpsymm, rct.transformIdx);
            }else{
                ocand = cco.transform(ccp, rct.transformIdx);
            }
        }
        if( !isInit || ocand < orepr ) {
            orepr = ocand;
            transform.clear();
            transform.push_back({ .transformedIdx = rct.transformIdx,
                    .reversed = rct.reversed, .symmetric = rct.symmetric });
            isInit = true;
        }else if( isInit && ocand == orepr )
            transform.push_back({ .transformedIdx = rct.transformIdx,
                    .reversed = rct.reversed, .symmetric = rct.symmetric });
    }
    for(EdgeReprCandidateTransform &erct : transform) {
        cubeedges ce2symm = erct.symmetric ? ce2.symmetric() : ce2;
        if( reverse ) {
            if( erct.reversed ) {
                // transform((ce1 rev) ⊙  (ce2 rev)) = cetrans ⊙  (ce1 rev) ⊙  (ce2 rev) ⊙  cetransRev
                // ceTrans = (ce2 rev) ⊙  cetransRev
                erct.ceTrans = cubeedges::compose(ce2symm.reverse(),
                        ctransformed[transformReverse(erct.transformedIdx)].ce);
            }else{
                // transform(ce2 ⊙  ce1) = cetrans ⊙  ce2 ⊙  ce1 ⊙  cetransRev
                // ceTrans = cetrans ⊙  ce2
                erct.ceTrans = cubeedges::compose(
                        ctransformed[erct.transformedIdx].ce, ce2symm);
            }
        }else{
            if( erct.reversed ) {
                // transform((ce2 rev) ⊙  (ce1 rev)) = cetrans ⊙  (ce2 rev) ⊙  (ce1 rev) ⊙  cetransRev
                // ceTrans = cetrans ⊙  (ce2 rev)
                erct.ceTrans = cubeedges::compose(
                        ctransformed[erct.transformedIdx].ce, ce2symm.reverse());
            }else{
                // transform(ce1 ⊙  ce2) = cetrans ⊙  ce1 ⊙  ce2 ⊙  cetransRev
                // ceTrans = ce2 ⊙  cetransRev
                erct.ceTrans = cubeedges::compose(ce2symm,
                        ctransformed[transformReverse(erct.transformedIdx)].ce);
            }
        }
    }
    return orepr;
}

bool isSingleTransform(cubecorners_perm ccp) {
    return gPermToRepr[ccp.getPermIdx()].transform.size() == 1;
}

cubecorner_orients cubecornerOrientsForComposedRepresentative(cubecorners_perm ccpSearch,
        cubecorner_orients ccoSearchRepr, bool reversed, const cube &cSearchT,
        std::vector<EdgeReprCandidateTransform> &transform)
{
    const CubecornerPermToRepr &permToRepr = gPermToRepr.at(ccpSearch.getPermIdx());
    cubecorners_perm ccpSearchRepr = gReprPerms[permToRepr.reprIdx].ccp;
    cube cSearchTrev = cSearchT.reverse();

	transform.clear();
    const ReprCandidateTransform &rct = permToRepr.transform.front();
    // sequence was: ccpSearch reverse, then symmetric, then transform
    unsigned transformIdxRev = transformReverse(rct.transformIdx);
    //cubecorners_perm ccpSearchRevSymm = ccpSearchRepr.transform(transformIdxRev);
    cubecorner_orients ccoSearchRevSymm = ccoSearchRepr.transform(ccpSearchRepr, transformIdxRev);
    //cubecorners_perm ccpSearchRev = rct.symmetric ? ccpSearchRevSymm.symmetric() : ccpSearchRevSymm;
    cubecorner_orients ccoSearchRev = rct.symmetric ? ccoSearchRevSymm.symmetric() : ccoSearchRevSymm;
    //cubecorners_perm ccpSearch = rct.reversed ? ccpSearchRev.reverse() : ccpSearchRev;
    cubecorner_orients ccoSearch = rct.reversed ? ccoSearchRev.reverse(ccpSearch.reverse()) : ccoSearchRev;
    // ccpSearch is: ccpSearch = reversed ? (cSearchT.ccp ⊙  ccp) : (ccp ⊙  cSearchT.ccp)
    cubecorner_orients cco = reversed ?
        cubecorner_orients::compose(cSearchTrev.cco, ccpSearch, ccoSearch) :
        cubecorner_orients::compose(ccoSearch, cSearchTrev.ccp, cSearchTrev.cco);
    transform.push_back({ .transformedIdx = rct.transformIdx,
                .reversed = rct.reversed, .symmetric = rct.symmetric });
    EdgeReprCandidateTransform &erct = transform.front();
    cubeedges ce2symm = erct.symmetric ? cSearchT.ce.symmetric() : cSearchT.ce;
    if( reversed ) {
        if( erct.reversed ) {
            // transform((ce1 rev) ⊙  (ce2 rev)) = cetrans ⊙  (ce1 rev) ⊙  (ce2 rev) ⊙  cetransRev
            // ceTrans = (ce2 rev) ⊙  cetransRev
            erct.ceTrans = cubeedges::compose(ce2symm.reverse(),
                    ctransformed[transformReverse(erct.transformedIdx)].ce);
        }else{
            // transform(ce2 ⊙  ce1) = cetrans ⊙  ce2 ⊙  ce1 ⊙  cetransRev
            // ceTrans = ctrans ⊙  ce2
            erct.ceTrans = cubeedges::compose(
                    ctransformed[erct.transformedIdx].ce, ce2symm);
        }
    }else{
        if( erct.reversed ) {
            // transform((ce2 rev) ⊙  (ce1 rev)) = cetrans ⊙  (ce2 rev) ⊙  (ce1 rev) ⊙  cetransRev
            // ceTrans = cetrans ⊙  (ce2 rev)
            erct.ceTrans = cubeedges::compose(
                    ctransformed[erct.transformedIdx].ce, ce2symm.reverse());
        }else{
            // transform(ce1 ⊙  ce2) = cetrans ⊙  ce1 ⊙  ce2 ⊙  cetransRev
            // ceTrans = ce2 ⊙  cetransRev
            erct.ceTrans = cubeedges::compose(ce2symm,
                    ctransformed[transformReverse(erct.transformedIdx)].ce);
        }
    }
    return cco;
}

cubeedges cubeedgesComposedRepresentative(cubeedges ce, bool reverse,
        const std::vector<EdgeReprCandidateTransform> &transform)
{
    cubeedges cerepr;
    if( transform.size() == 1 ) {
        const EdgeReprCandidateTransform &erct = transform.front();
        cubeedges cesymm = erct.symmetric ? ce.symmetric() : ce;
        if( reverse ) {
            if( erct.reversed ) {
                // erct.ceTrans = (ce2 rev) ⊙  cetransRev
                // transform((ce1 rev) ⊙  (ce2 rev)) = cetrans ⊙  (ce1 rev) ⊙  (ce2 rev) ⊙  cetransRev
                //      = cetrans ⊙  (ce1 rev) ⊙  erct.ceTrans
                cerepr = cubeedges::compose3revmid(ctransformed[erct.transformedIdx].ce,
                        cesymm, erct.ceTrans);
            }else{
                // erct.ceTrans = ctrans ⊙  ce2
                // transform(ce2 ⊙  ce1) = cetrans ⊙  ce2 ⊙  ce1 ⊙  cetransRev
                //      = erct.ceTrans ⊙  ce1 ⊙  cetransRev
                cerepr = cubeedges::compose3(erct.ceTrans, cesymm,
                        ctransformed[transformReverse(erct.transformedIdx)].ce);
            }
        }else{
            if( erct.reversed ) {
                // erct.ceTrans = cetrans ⊙  (ce2 rev)
                // transform((ce2 rev) ⊙  (ce1 rev)) = cetrans ⊙  (ce2 rev) ⊙  (ce1 rev) ⊙  cetransRev
                //      = erct.ceTrans ⊙  (ce1 rev) ⊙  cetransRev
                cerepr = cubeedges::compose3revmid(erct.ceTrans, cesymm,
                        ctransformed[transformReverse(erct.transformedIdx)].ce);
            }else{
                // erct.ceTrans = ce2 ⊙  cetransRev
                // transform(ce1 ⊙  ce2) = cetrans ⊙  ce1 ⊙  ce2 ⊙  cetransRev
                //      = cetrans ⊙  ce1 ⊙  erct.ceTrans
                cerepr = cubeedges::compose3(ctransformed[erct.transformedIdx].ce,
                        cesymm, erct.ceTrans);
            }
        }
    }else{
        bool isInit = false;
        for(const EdgeReprCandidateTransform &erct : transform) {
            cubeedges cand;
            cubeedges cesymm = erct.symmetric ? ce.symmetric() : ce;
            if( reverse ) {
                if( erct.reversed ) {
                    cand = cubeedges::compose3revmid(ctransformed[erct.transformedIdx].ce,
                            cesymm, erct.ceTrans);
                }else{
                    cand = cubeedges::compose3(erct.ceTrans, cesymm,
                            ctransformed[transformReverse(erct.transformedIdx)].ce);
                }
            }else{
                if( erct.reversed ) {
                    cand = cubeedges::compose3revmid(erct.ceTrans, cesymm,
                            ctransformed[transformReverse(erct.transformedIdx)].ce);
                }else{
                    cand = cubeedges::compose3(ctransformed[erct.transformedIdx].ce,
                            cesymm, erct.ceTrans);
                }
            }

            if( !isInit || cand < cerepr )
                cerepr = cand;
            isInit = true;
        }
    }
    return cerepr;
}

cube cubeRepresentative(const cube &c) {
    std::vector<EdgeReprCandidateTransform> transform;

    cubecorners_perm ccpRepr = cubecornerPermsRepresentative(c.ccp);
    cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(c.ccp,
            c.cco, transform);
    cubeedges ceRepr = cubeedgesRepresentative(c.ce, transform);
    return { .ccp = ccpRepr, .cco = ccoRepr, .ce = ceRepr };
}

unsigned CornerOrientReprCubes::addCubes(const std::vector<cubeedges> &cearr)
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

void CornerOrientReprCubes::initOccur(std::array<unsigned,64> &orientOcc) {
    m_orientOccur = &orientOcc;
    for(cubeedges ce : m_items) {
        unsigned short orientIdx = ce.getOrientIdx();
        orientOcc[orientIdx >> 5] |= 1ul << (orientIdx & 0x1f);
    }
}

bool CornerOrientReprCubes::containsCubeEdges(cubeedges ce) const
{
    if( m_orientOccur ) {
        unsigned short orientIdx = ce.getOrientIdx();
        if( ((*m_orientOccur)[orientIdx >> 5] & 1ul << (orientIdx & 0x1f)) == 0 )
            return false;
    }
    edges_iter edgeIt = std::lower_bound(m_items.begin(), m_items.end(), ce);
	return edgeIt != m_items.end() && *edgeIt == ce;
}

cubeedges CornerOrientReprCubes::findSolutionEdgeMulti(
        const CornerOrientReprCubes &ccoReprCubes,
        const CornerOrientReprCubes &ccoReprSearchCubes,
        const std::vector<EdgeReprCandidateTransform> &otransform,
        bool reversed)
{
    for(CornerOrientReprCubes::edges_iter edgeIt = ccoReprCubes.edgeBegin();
            edgeIt != ccoReprCubes.edgeEnd(); ++edgeIt)
    {
        const cubeedges ce = *edgeIt;
        cubeedges ceSearchRepr = cubeedgesComposedRepresentative(ce, reversed, otransform);
        if( ccoReprSearchCubes.containsCubeEdges(ceSearchRepr) )
            return ce;
    }
    return cubeedges();
}

cubeedges CornerOrientReprCubes::findSolutionEdgeSingle(
        const CornerOrientReprCubes &ccoReprCubes,
        const CornerOrientReprCubes &ccoReprSearchCubes,
        const EdgeReprCandidateTransform &erct,
        bool reversed)
{
    cubeedges cetrans = ctransformed[erct.transformedIdx].ce;
    cubeedges cetransRev = ctransformed[transformReverse(erct.transformedIdx)].ce;
    if( ccoReprCubes.size() <= ccoReprSearchCubes.size() ) {
        for(CornerOrientReprCubes::edges_iter edgeIt = ccoReprCubes.edgeBegin();
                edgeIt != ccoReprCubes.edgeEnd(); ++edgeIt)
        {
            cubeedges ce = *edgeIt;
            cubeedges cesymm = erct.symmetric ? ce.symmetric() : ce;
            cubeedges ceSearchRepr;
            if( reversed ) {
                if( erct.reversed )
                    ceSearchRepr = cubeedges::compose3revmid(cetrans, cesymm, erct.ceTrans);
                else
                    ceSearchRepr = cubeedges::compose3(erct.ceTrans, cesymm, cetransRev);
            }else{
                if( erct.reversed )
                    ceSearchRepr = cubeedges::compose3revmid(erct.ceTrans, cesymm, cetransRev);
                else
                    ceSearchRepr = cubeedges::compose3(cetrans, cesymm, erct.ceTrans);
            }
            if( ccoReprSearchCubes.containsCubeEdges(ceSearchRepr) )
                return ce;
        }
    }else{
        cubeedges erctCeTransRev = erct.ceTrans.reverse();
        for(CornerOrientReprCubes::edges_iter edgeIt = ccoReprSearchCubes.edgeBegin();
                edgeIt != ccoReprSearchCubes.edgeEnd(); ++edgeIt)
        {
            cubeedges ce = *edgeIt;
            cubeedges ceSearch;

            if( reversed ) {
                if( erct.reversed ) {
                    ceSearch = cubeedges::compose3revmid(erct.ceTrans, ce, cetrans);
                }else{
                    ceSearch = cubeedges::compose3(erctCeTransRev, ce, cetrans);
                }
            }else{
                if( erct.reversed ) {
                    ceSearch = cubeedges::compose3revmid(cetransRev, ce, erct.ceTrans);
                }else{
                    ceSearch = cubeedges::compose3(cetransRev, ce, erctCeTransRev);
                }
            }
            cubeedges cesymmSearch = erct.symmetric ? ceSearch.symmetric() : ceSearch;
            if( ccoReprCubes.containsCubeEdges(cesymmSearch) )
                return cesymmSearch;
        }
    }
    return cubeedges();
}

cubeedges CornerOrientReprCubes::findSolutionEdge(
        const CornerOrientReprCubes &ccoReprCubes,
        const CornerOrientReprCubes &ccoReprSearchCubes,
        const std::vector<EdgeReprCandidateTransform> &otransform,
        bool reversed)
{
    cubeedges ce;
    if( otransform.size() == 1 )
        ce = CornerOrientReprCubes::findSolutionEdgeSingle(
                ccoReprCubes, ccoReprSearchCubes, otransform.front(), reversed);
    else{
        ce = CornerOrientReprCubes::findSolutionEdgeMulti(
                ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
    }
    return ce;
}


const CornerOrientReprCubes CornerPermReprCubes::m_coreprCubesEmpty =
    CornerOrientReprCubes(cubecorner_orients());

CornerPermReprCubes::CornerPermReprCubes()
{
}

CornerPermReprCubes::~CornerPermReprCubes()
{
}

size_t CornerPermReprCubes::cubeCount() const {
    size_t res = 0;
    for(ccocubes_iter it = ccoCubesBegin(); it != ccoCubesEnd(); ++it)
        res += it->size();
    return res;
}

void CornerPermReprCubes::initOccur() {
    m_orientOccurMem.resize(m_coreprCubes.size());
    for(unsigned i = 0; i < m_coreprCubes.size(); ++i)
        m_coreprCubes[i].initOccur(m_orientOccurMem[i]);
}

const CornerOrientReprCubes &CornerPermReprCubes::cornerOrientCubesAt(cubecorner_orients cco) const {
    ccocubes_iter ccoIt = std::lower_bound(m_coreprCubes.begin(),
            m_coreprCubes.end(), cco, ItemLessCco());
    if( ccoIt != m_coreprCubes.end() && ccoIt->getOrients() == cco )
        return *ccoIt;
    return m_coreprCubesEmpty;
}

CornerOrientReprCubes &CornerPermReprCubes::cornerOrientCubesAdd(cubecorner_orients cco) {
    std::vector<CornerOrientReprCubes>::iterator ccoIt = std::lower_bound(m_coreprCubes.begin(),
            m_coreprCubes.end(), cco, ItemLessCco());
    if( ccoIt == m_coreprCubes.end() || cco < ccoIt->getOrients() ) {
        unsigned lo = std::distance(m_coreprCubes.begin(), ccoIt), hi = m_coreprCubes.size();
        m_coreprCubes.emplace_back(cco);
        while( hi > lo ) {
            m_coreprCubes[hi].swap(m_coreprCubes[hi-1]);
            --hi;
        }
        ccoIt = m_coreprCubes.begin()+lo;
    }
    return *ccoIt;
}

unsigned CubesReprAtDepth::size() {
    return USEREVERSE ? 654 : 984;
}

CubesReprAtDepth::CubesReprAtDepth()
    : m_cornerPermReprCubes(size())
{
    for(unsigned i = 0; i < m_cornerPermReprCubes.size(); ++i)
        m_cornerPermReprCubes[i].first = gReprPerms[i].ccp;
}

CubesReprAtDepth::~CubesReprAtDepth() {
}

size_t CubesReprAtDepth::cubeCount() const {
    size_t res = 0;
    for(const std::pair<cubecorners_perm, CornerPermReprCubes> &reprCube : m_cornerPermReprCubes)
        res += reprCube.second.cubeCount();
    return res;
}

void CubesReprAtDepth::initOccur(unsigned idx)
{
    m_cornerPermReprCubes[idx].second.initOccur();
}

CornerPermReprCubes &CubesReprAtDepth::add(unsigned idx) {
    return m_cornerPermReprCubes[idx].second;
}

bool CubesReprByDepth::isUseReverse() {
    return USEREVERSE;
}

std::string CubesReprByDepth::getMoves(const cube &c, bool movesRev) const
{
	std::vector<int> rotateDirs;
    std::vector<int>::iterator insertPos = rotateDirs.end();
    cube crepr = cubeRepresentative(c);
    unsigned ccpReprIdx = cubecornerPermRepresentativeIdx(crepr.ccp);
	unsigned depth = 0;
	while( true ) {
        const CornerPermReprCubes &ccpReprCubes = m_cubesAtDepths[depth].getAt(ccpReprIdx);
        const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(crepr.cco);
		if( ccoReprCubes.containsCubeEdges(crepr.ce) )
			break;
		++depth;
        if( depth >= m_availCount ) {
            std::cout << "printMoves: depth reached maximum, cube NOT FOUND" << std::endl;
            exit(1);
        }
	}
	cube cc = c;
	while( depth-- > 0 ) {
		int cm = 0;
        cube ccRev = cc.reverse();
		cube cc1;
		while( cm < RCOUNT ) {
			cc1 = cube::compose(cc, crotated[cm]);
            cube cc1repr = cubeRepresentative(cc1);
            ccpReprIdx = cubecornerPermRepresentativeIdx(cc1repr.ccp);
            const CornerPermReprCubes &ccpReprCubes = m_cubesAtDepths[depth].getAt(ccpReprIdx);
            const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(cc1repr.cco);
            if( ccoReprCubes.containsCubeEdges(cc1repr.ce) )
                break;
			cc1 = cube::compose(ccRev, crotated[cm]);
            cc1repr = cubeRepresentative(cc1);
            ccpReprIdx = cubecornerPermRepresentativeIdx(cc1repr.ccp);
            const CornerPermReprCubes &ccpReprCubesRev = m_cubesAtDepths[depth].getAt(ccpReprIdx);
            const CornerOrientReprCubes &ccoReprCubesRev = ccpReprCubesRev.cornerOrientCubesAt(cc1repr.cco);
            if( ccoReprCubesRev.containsCubeEdges(cc1repr.ce) ) {
                movesRev = !movesRev;
                break;
            }
			++cm;
		}
        if( cm == RCOUNT ) {
            std::cout << "printMoves: cube at depth " << depth << " NOT FOUND" << std::endl;
            exit(1);
        }
		insertPos = rotateDirs.insert(insertPos, movesRev ? cm : rotateDirReverse(cm));
        if( movesRev )
            ++insertPos;
		cc = cc1;
	}
    std::string res;
	for(auto rotateDir : rotateDirs) {
		res += " ";
        res += rotateDirName(rotateDir);
    }
    return res;
}

unsigned long addCubesForReprPerm(CubesReprByDepth *cubesReprByDepth, unsigned permReprIdx, int depth)
{
    std::vector<EdgeReprCandidateTransform> otransformNew;
    unsigned long cubeCount = 0;

    const CubesReprAtDepth &ccpReprCubesC = (*cubesReprByDepth)[depth-1];
    const CornerPermReprCubes *ccpReprCubesNewP = depth == 1 ? nullptr :
        &(*cubesReprByDepth)[depth-2].getAt(permReprIdx);
    const CornerPermReprCubes &ccpReprCubesNewC = ccpReprCubesC.getAt(permReprIdx);
    CornerPermReprCubes &ccpReprCubesNewN = (*cubesReprByDepth)[depth].add(permReprIdx);
    for(unsigned permIdx : gReprPerms[permReprIdx].represented) {
        cubecorners_perm ccpNew = cubecorners_perm::fromPermIdx(permIdx);
        for(unsigned rd = 0; rd < RCOUNT; ++rd) {
            unsigned rdRev = rotateDirReverse(rd);
            for(unsigned reversed = 0; reversed < (USEREVERSE ? 2 : 1); ++reversed) {
                cubecorners_perm ccp = reversed ?
                    cubecorners_perm::compose(crotated[rdRev].ccp, ccpNew) :
                    cubecorners_perm::compose(ccpNew, crotated[rdRev].ccp);
                unsigned ccpReprIdx = cubecornerPermRepresentativeIdx(ccp);
                if( gReprPerms[ccpReprIdx].ccp == ccp ) {
                    const CornerPermReprCubes &cpermReprCubesC = ccpReprCubesC.getAt(ccpReprIdx);
                    for(CornerPermReprCubes::ccocubes_iter ccoCubesItC =
                            cpermReprCubesC.ccoCubesBegin();
                            ccoCubesItC != cpermReprCubesC.ccoCubesEnd(); ++ccoCubesItC)
                    {
                        const CornerOrientReprCubes &corientReprCubesC = *ccoCubesItC;
                        cubecorner_orients cco = corientReprCubesC.getOrients();
                        cubecorner_orients ccoNew = reversed ?
                            cubecorner_orients::compose(crotated[rd].cco, ccp, cco) :
                            cubecorner_orients::compose(cco, crotated[rd].ccp, crotated[rd].cco);
                        cubecorner_orients ccoReprNew = cubecornerOrientsComposedRepresentative(
                                ccpNew, ccoNew, reversed, crotated[rd].ce, otransformNew);
                        const CornerOrientReprCubes *corientReprCubesNewP =
                            ccpReprCubesNewP == NULL ? NULL :
                            &ccpReprCubesNewP->cornerOrientCubesAt(ccoReprNew);
                        const CornerOrientReprCubes &corientReprCubesNewC =
                            ccpReprCubesNewC.cornerOrientCubesAt(ccoReprNew);
                        std::vector<cubeedges> ceNewArr;
                        for(CornerOrientReprCubes::edges_iter edgeIt = corientReprCubesC.edgeBegin();
                                edgeIt != corientReprCubesC.edgeEnd(); ++edgeIt)
                        {
                            const cubeedges ce = *edgeIt;
                            cubeedges cenewRepr = cubeedgesComposedRepresentative(
                                    ce, reversed, otransformNew);
                            if( corientReprCubesNewP != NULL &&
                                    corientReprCubesNewP->containsCubeEdges(cenewRepr) )
                                continue;
                            if( corientReprCubesNewC.containsCubeEdges(cenewRepr) )
                                continue;
                            ceNewArr.push_back(cenewRepr);
                        }
                        if( !ceNewArr.empty() ) {
                            CornerOrientReprCubes &corientReprCubesNewN =
                                ccpReprCubesNewN.cornerOrientCubesAdd(ccoReprNew);
                            cubeCount += corientReprCubesNewN.addCubes(ceNewArr);
                        }
                    }
                }
            }
        }
    }
    return cubeCount;
}

