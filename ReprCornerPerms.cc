#include "ReprCornerPerms.h"
#include <iostream>


ReprCornerPerms::ReprCornerPerms(bool useReverse)
    : m_permToRepr(40320), m_useReverse(useReverse)
{
    m_reprPerms.reserve(m_useReverse ? 654 : 984);
    for(unsigned pidx = 0; pidx < 40320; ++pidx) {
        CornersPerm perm = CornersPerm::fromPermIdx(pidx);
        if( m_permToRepr[pidx].reprIdx < 0 ) {
            CornersPerm permRepr;
            for(unsigned reversed = 0; reversed < (m_useReverse ? 2 : 1); ++reversed) {
                CornersPerm permr = reversed ? perm.reverse() : perm;
                for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
                    CornersPerm permchk = symmetric ? permr.symmetric() : permr;
                    for(unsigned short td = 0; td < TCOUNT; ++td) {
                        CornersPerm cand = permchk.transform(td);
                        if( td+reversed+symmetric == 0 || cand < permRepr )
                            permRepr = cand;
                    }
                }
            }
            int reprIdx = m_reprPerms.size();
            m_reprPerms.push_back(permRepr);
            for(unsigned short td = 0; td < TCOUNT; ++td) {
                CornersPerm permtd = permRepr.transform(transformReverse(td));
                for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
                    CornersPerm premsymmtd = symmetric ? permtd.symmetric() : permtd;
                    for(unsigned reversed = 0; reversed < (m_useReverse ? 2 : 1); ++reversed) {
                        CornersPerm permrsymmtd = reversed ? premsymmtd.reverse() : premsymmtd;
                        CubecornerPermToRepr &permToRepr = m_permToRepr[permrsymmtd.getPermIdx()];
                        permToRepr.reprIdx = reprIdx;
                        permToRepr.transform.push_back({ .reversed = (bool)reversed,
                                    .symmetric = (bool)symmetric, .transformIdx = td });
                    }
                }
            }
        }
    }
    std::cout << "repr size=" << m_reprPerms.size() << std::endl;
}

ReprCornerPerms::~ReprCornerPerms()
{
}

CornersPerm ReprCornerPerms::getReprPerm(CornersPerm ccp) const
{
    unsigned reprPermIdx = m_permToRepr.at(ccp.getPermIdx()).reprIdx;
    return m_reprPerms[reprPermIdx];
}

unsigned ReprCornerPerms::getReprPermIdx(CornersPerm ccp) const
{
    return m_permToRepr.at(ccp.getPermIdx()).reprIdx;
}

CornersPerm ReprCornerPerms::getPermForIdx(unsigned reprPermIdx) const
{
    return m_reprPerms[reprPermIdx];
}

bool ReprCornerPerms::isSingleTransform(CornersPerm ccp) const {
    return m_permToRepr[ccp.getPermIdx()].transform.size() == 1;
}

CornersOrient ReprCornerPerms::getReprOrients(
        CornersPerm ccp, CornersOrient cco,
        std::vector<EdgeReprCandidateTransform> &transform) const
{
    CubecornerPermToRepr permToRepr = m_permToRepr.at(ccp.getPermIdx());
    CornersPerm ccpsymm, ccprev, ccprevsymm;
    CornersOrient orepr, ccosymm, ccorev, ccorevsymm;
    bool isInit = false, isSymmInit = false, isRevInit = false, isRevSymmInit = false;
    transform.clear();
    for(const ReprCandidateTransform &rct : permToRepr.transform) {
        CornersOrient ocand;
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
CornersOrient ReprCornerPerms::getComposedReprOrients(
        CornersPerm ccp, CornersOrient cco, bool reverse,
        cubeedges ce2, std::vector<EdgeReprCandidateTransform> &transform) const
{
    const CubecornerPermToRepr &permToRepr = m_permToRepr.at(ccp.getPermIdx());
    CornersPerm ccpsymm, ccprev, ccprevsymm;
    CornersOrient orepr, ccosymm, ccorev, ccorevsymm;
    bool isInit = false, isSymmInit = false, isRevInit = false, isRevSymmInit = false;
    transform.clear();
    for(const ReprCandidateTransform &rct : permToRepr.transform) {
        CornersOrient ocand;
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

CornersOrient ReprCornerPerms::getOrientsForComposedRepr(
        CornersPerm ccpSearch, CornersOrient ccoSearchRepr,
        bool reversed, const cube &cSearchT,
        std::vector<EdgeReprCandidateTransform> &transform) const
{
    const CubecornerPermToRepr &permToRepr = m_permToRepr.at(ccpSearch.getPermIdx());
    CornersPerm ccpSearchRepr = m_reprPerms[permToRepr.reprIdx];
    cube cSearchTrev = cSearchT.reverse();

    transform.clear();
    const ReprCandidateTransform &rct = permToRepr.transform.front();
    // sequence was: ccpSearch reverse, then symmetric, then transform
    unsigned transformIdxRev = transformReverse(rct.transformIdx);
    //CornersPerm ccpSearchRevSymm = ccpSearchRepr.transform(transformIdxRev);
    CornersOrient ccoSearchRevSymm = ccoSearchRepr.transform(ccpSearchRepr, transformIdxRev);
    //CornersPerm ccpSearchRev = rct.symmetric ? ccpSearchRevSymm.symmetric() : ccpSearchRevSymm;
    CornersOrient ccoSearchRev = rct.symmetric ? ccoSearchRevSymm.symmetric() : ccoSearchRevSymm;
    //CornersPerm ccpSearch = rct.reversed ? ccpSearchRev.reverse() : ccpSearchRev;
    CornersOrient ccoSearch = rct.reversed ? ccoSearchRev.reverse(ccpSearch.reverse()) : ccoSearchRev;
    // ccpSearch is: ccpSearch = reversed ? (cSearchT.ccp ⊙  ccp) : (ccp ⊙  cSearchT.ccp)
    CornersOrient cco = reversed ?
        CornersOrient::compose(cSearchTrev.cco, ccpSearch, ccoSearch) :
        CornersOrient::compose(ccoSearch, cSearchTrev.ccp, cSearchTrev.cco);
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

cubeedges ReprCornerPerms::getReprCubeedges(cubeedges ce,
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

cubeedges ReprCornerPerms::getComposedReprCubeedges(
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

cube ReprCornerPerms::cubeRepresentative(const cube &c) const {
    std::vector<EdgeReprCandidateTransform> transform;

    CornersPerm ccpRepr = getReprPerm(c.ccp);
    CornersOrient ccoRepr = getReprOrients(c.ccp, c.cco, transform);
    cubeedges ceRepr = getReprCubeedges(c.ce, transform);
    return { .ccp = ccpRepr, .cco = ccoRepr, .ce = ceRepr };
}

