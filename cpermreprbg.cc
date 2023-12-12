#include "cpermreprbg.h"
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


BGCubecornerReprPerms::BGCubecornerReprPerms(bool useReverse)
    : m_permToRepr(40320), m_useReverse(useReverse)
{
    m_reprPerms.reserve(useReverse ? 1672 : 2768);
    for(unsigned pidx = 0; pidx < 40320; ++pidx) {
        cubecorners_perm perm = cubecorners_perm::fromPermIdx(pidx);
        cubecorners_perm permRepr;
        std::vector<ReprCandidateTransform> transform;
        for(unsigned reversed = 0; reversed < (useReverse ? 2 : 1); ++reversed) {
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
        CubecornerPermToRepr &permToReprRepr = m_permToRepr[permRepr.getPermIdx()];
        if( permToReprRepr.reprIdx < 0 ) {
            permToReprRepr.reprIdx = m_reprPerms.size();
            m_reprPerms.push_back(permRepr);
        }
        CubecornerPermToRepr &permToRepr = m_permToRepr[perm.getPermIdx()];
        permToRepr.reprIdx = permToReprRepr.reprIdx;
        permToRepr.transform.swap(transform);
    }
    std::cout << "bg repr size=" << m_reprPerms.size() << std::endl;
}

BGCubecornerReprPerms::~BGCubecornerReprPerms()
{
}

cubecorners_perm BGCubecornerReprPerms::getReprPerm(cubecorners_perm ccp) const
{
    unsigned permReprIdx = m_permToRepr.at(ccp.getPermIdx()).reprIdx;
    return m_reprPerms[permReprIdx];
}

unsigned BGCubecornerReprPerms::getReprPermIdx(cubecorners_perm ccp) const
{
    return m_permToRepr.at(ccp.getPermIdx()).reprIdx;
}

cubecorners_perm BGCubecornerReprPerms::getPermForIdx(unsigned reprPermIdx) const
{
    return m_reprPerms[reprPermIdx];
}

bool BGCubecornerReprPerms::isSingleTransform(cubecorners_perm ccp) const {
    return m_permToRepr[ccp.getPermIdx()].transform.size() == 1;
}

cubeedges BGCubecornerReprPerms::getReprCubeedges(
        cubecorners_perm ccp, cubeedges ce) const
{
    CubecornerPermToRepr permToRepr = m_permToRepr.at(ccp.getPermIdx());
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

cube BGCubecornerReprPerms::cubeRepresentative(const cube &c) const {
    cubecorners_perm ccpRepr = getReprPerm(c.ccp);
    cubeedges ceRepr = getReprCubeedges(c.ccp, c.ce);
    return { .ccp = ccpRepr, .cco = csolved.cco, .ce = ceRepr };
}

cubeedges BGCubecornerReprPerms::getCubeedgesForRepresentative(cubecorners_perm ccpSearch,
        cubeedges ceSearchRepr) const
{
    const CubecornerPermToRepr &permToRepr = m_permToRepr.at(ccpSearch.getPermIdx());
    const ReprCandidateTransform &rct = permToRepr.transform.front();
    // sequence was: ccpSearch reverse, then symmetric, then transform
    unsigned transformIdxRev = transformReverse(rct.transformIdx);
    cubeedges ceSearchRevSymm = ceSearchRepr.transform(transformIdxRev);
    cubeedges ceSearchRev = rct.symmetric ? ceSearchRevSymm.symmetric() : ceSearchRevSymm;
    cubeedges ceSearch = rct.reversed ? ceSearchRev.reverse() : ceSearchRev;
    return ceSearch;
}

