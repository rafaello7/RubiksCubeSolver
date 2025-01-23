#include "cpermrepr.h"
#include <iostream>


CubecornerReprPerms::CubecornerReprPerms(bool useReverse)
    : m_permToRepr(40320), m_useReverse(useReverse)
{
    m_reprPerms.reserve(m_useReverse ? 654 : 984);
    for(unsigned pidx = 0; pidx < 40320; ++pidx) {
        cubecorners_perm perm = cubecorners_perm::fromPermIdx(pidx);
        cubecorners_perm permRepr;
        std::vector<ReprCandidateTransform> transform;
        for(unsigned reversed = 0; reversed < (m_useReverse ? 2 : 1); ++reversed) {
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
        CubecornerPermToRepr &permToReprRepr = m_permToRepr[permRepr.getPermIdx()];
        if( permToReprRepr.reprIdx < 0 ) {
            permToReprRepr.reprIdx = m_reprPerms.size();
            m_reprPerms.push_back(permRepr);
        }
        CubecornerPermToRepr &permToRepr = m_permToRepr[perm.getPermIdx()];
        permToRepr.reprIdx = permToReprRepr.reprIdx;
        permToRepr.transform.swap(transform);
    }
    std::cout << "repr size=" << m_reprPerms.size() << std::endl;
}

CubecornerReprPerms::~CubecornerReprPerms()
{
}

cubecorners_perm CubecornerReprPerms::getReprPerm(cubecorners_perm ccp) const
{
    unsigned reprPermIdx = m_permToRepr.at(ccp.getPermIdx()).reprIdx;
    return m_reprPerms[reprPermIdx];
}

unsigned CubecornerReprPerms::getReprPermIdx(cubecorners_perm ccp) const
{
    return m_permToRepr.at(ccp.getPermIdx()).reprIdx;
}

cubecorners_perm CubecornerReprPerms::getPermForIdx(unsigned reprPermIdx) const
{
    return m_reprPerms[reprPermIdx];
}

bool CubecornerReprPerms::isSingleTransform(cubecorners_perm ccp) const {
    return m_permToRepr[ccp.getPermIdx()].transform.size() == 1;
}

cubecorner_orients CubecornerReprPerms::getReprOrients(
        cubecorners_perm ccp, cubecorner_orients cco,
        std::vector<EdgeReprCandidateTransform> &transform) const
{
    CubecornerPermToRepr permToRepr = m_permToRepr.at(ccp.getPermIdx());
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
        }else if( ocand == orepr )
            transform.push_back({ .transformedIdx = rct.transformIdx,
                    .reversed = rct.reversed, .symmetric = rct.symmetric, .ceTrans = csolved.ce });
    }
    return orepr;
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
cubecorner_orients CubecornerReprPerms::getComposedReprOrients(
        cubecorners_perm ccp, cubecorner_orients cco, bool reverse,
        cubeedges ce2, std::vector<EdgeReprCandidateTransform> &transform) const
{
    const CubecornerPermToRepr &permToRepr = m_permToRepr.at(ccp.getPermIdx());
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
        }else if( ocand == orepr )
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

cubecorner_orients CubecornerReprPerms::getOrientsForComposedRepr(
        cubecorners_perm ccpSearch, cubecorner_orients ccoSearchRepr,
        bool reversed, const cube &cSearchT,
        std::vector<EdgeReprCandidateTransform> &transform) const
{
    const CubecornerPermToRepr &permToRepr = m_permToRepr.at(ccpSearch.getPermIdx());
    cubecorners_perm ccpSearchRepr = m_reprPerms[permToRepr.reprIdx];
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

cubeedges CubecornerReprPerms::getReprCubeedges(cubeedges ce,
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

cubeedges CubecornerReprPerms::getComposedReprCubeedges(
        cubeedges ce, bool reverse,
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

cube CubecornerReprPerms::cubeRepresentative(const cube &c) const {
    std::vector<EdgeReprCandidateTransform> transform;

    cubecorners_perm ccpRepr = getReprPerm(c.ccp);
    cubecorner_orients ccoRepr = getReprOrients(c.ccp, c.cco, transform);
    cubeedges ceRepr = getReprCubeedges(c.ce, transform);
    return { .ccp = ccpRepr, .cco = ccoRepr, .ce = ceRepr };
}

