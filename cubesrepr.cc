#include "cubesrepr.h"
#include <iostream>
#include <algorithm>
#include <set>

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
        cubeedges ceSearchRepr = CubecornerReprPerms::getComposedReprCubeedges(
                ce, reversed, otransform);
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

CubesReprAtDepth::CubesReprAtDepth(const CubecornerReprPerms &reprPerms)
    : m_reprPerms(reprPerms), m_cornerPermReprCubes(reprPerms.reprPermCount())
{
}

CubesReprAtDepth::~CubesReprAtDepth() {
}

size_t CubesReprAtDepth::cubeCount() const {
    size_t res = 0;
    for(const CornerPermReprCubes &reprCube : m_cornerPermReprCubes)
        res += reprCube.cubeCount();
    return res;
}

void CubesReprAtDepth::initOccur(unsigned idx)
{
    m_cornerPermReprCubes[idx].initOccur();
}

CornerPermReprCubes &CubesReprAtDepth::add(unsigned idx) {
    return m_cornerPermReprCubes[idx];
}

cubecorners_perm CubesReprAtDepth::getPermAt(ccpcubes_iter it) const
{
    unsigned reprPermIdx = std::distance(m_cornerPermReprCubes.begin(), it);
    return m_reprPerms.getPermForIdx(reprPermIdx);
}

CubesReprByDepth::CubesReprByDepth(bool useReverse)
    : m_reprPerms(useReverse), m_availCount(1)
{
    // insert the solved cube at depth 0
    m_cubesAtDepths.emplace_back(new CubesReprAtDepth(m_reprPerms));
    unsigned cornerPermReprIdx = m_reprPerms.getReprPermIdx(csolved.ccp);
    CornerPermReprCubes &ccpCubes = m_cubesAtDepths[0]->add(cornerPermReprIdx);
    std::vector<EdgeReprCandidateTransform> otransform;
    cubecorner_orients ccoRepr = m_reprPerms.getReprOrients(csolved.ccp,
            csolved.cco, otransform);
    CornerOrientReprCubes &ccoCubes = ccpCubes.cornerOrientCubesAdd(ccoRepr);
    cubeedges ceRepr = m_reprPerms.getReprCubeedges(csolved.ce, otransform);
    std::vector<cubeedges> ceReprArr = { ceRepr };
    ccoCubes.addCubes(ceReprArr);
}

bool CubesReprByDepth::isUseReverse() const {
    return m_reprPerms.isUseReverse();
}

CubesReprAtDepth &CubesReprByDepth::operator[](unsigned idx)
{
    while( idx >= m_cubesAtDepths.size() )
        m_cubesAtDepths.emplace_back(new CubesReprAtDepth(m_reprPerms));
    return *m_cubesAtDepths[idx];
}

std::string CubesReprByDepth::getMoves(const cube &c, bool movesRev) const
{
	std::vector<int> rotateDirs;
    std::vector<int>::iterator insertPos = rotateDirs.end();
    cube crepr = m_reprPerms.cubeRepresentative(c);
    unsigned ccpReprIdx = m_reprPerms.getReprPermIdx(crepr.ccp);
	unsigned depth = 0;
	while( true ) {
        const CornerPermReprCubes &ccpReprCubes = m_cubesAtDepths[depth]->getAt(ccpReprIdx);
        const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(crepr.cco);
		if( ccoReprCubes.containsCubeEdges(crepr.ce) )
			break;
		++depth;
        if( depth >= m_availCount ) {
            std::cout << "getMoves: depth reached maximum, cube NOT FOUND" << std::endl;
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
            cube cc1repr = m_reprPerms.cubeRepresentative(cc1);
            ccpReprIdx = m_reprPerms.getReprPermIdx(cc1repr.ccp);
            const CornerPermReprCubes &ccpReprCubes = m_cubesAtDepths[depth]->getAt(ccpReprIdx);
            const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(cc1repr.cco);
            if( ccoReprCubes.containsCubeEdges(cc1repr.ce) )
                break;
			cc1 = cube::compose(ccRev, crotated[cm]);
            cc1repr = m_reprPerms.cubeRepresentative(cc1);
            ccpReprIdx = m_reprPerms.getReprPermIdx(cc1repr.ccp);
            const CornerPermReprCubes &ccpReprCubesRev = m_cubesAtDepths[depth]->getAt(ccpReprIdx);
            const CornerOrientReprCubes &ccoReprCubesRev = ccpReprCubesRev.cornerOrientCubesAt(cc1repr.cco);
            if( ccoReprCubesRev.containsCubeEdges(cc1repr.ce) ) {
                movesRev = !movesRev;
                break;
            }
			++cm;
		}
        if( cm == RCOUNT ) {
            std::cout << "getMoves: cube at depth " << depth << " NOT FOUND" << std::endl;
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

unsigned long CubesReprByDepth::addCubesForReprPerm(unsigned reprPermIdx, int depth)
{
    std::vector<EdgeReprCandidateTransform> otransformNew;
    unsigned long cubeCount = 0;

    const CubesReprAtDepth &ccpReprCubesC = *m_cubesAtDepths[depth-1];
    const CornerPermReprCubes *ccpReprCubesNewP = depth == 1 ? nullptr :
        &m_cubesAtDepths[depth-2]->getAt(reprPermIdx);
    const CornerPermReprCubes &ccpReprCubesNewC = ccpReprCubesC.getAt(reprPermIdx);
    CornerPermReprCubes &ccpReprCubesNewN = m_cubesAtDepths[depth]->add(reprPermIdx);
    cubecorners_perm ccpNewRepr = m_reprPerms.getPermForIdx(reprPermIdx);
    std::set<cubecorners_perm> ccpChecked;
    for(unsigned trrev = 0; trrev < (m_reprPerms.isUseReverse() ? 2 : 1); ++trrev) {
        cubecorners_perm ccpNewReprRev = trrev ? ccpNewRepr.reverse() : ccpNewRepr;
        for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
            cubecorners_perm ccpNewS = symmetric ? ccpNewReprRev.symmetric() : ccpNewReprRev;
            for(unsigned td = 0; td < TCOUNT; ++td) {
                cubecorners_perm ccpNew = ccpNewS.transform(td);
                if( ccpChecked.find(ccpNew) == ccpChecked.end() ) {
                    ccpChecked.insert(ccpNew);
                    for(unsigned rd = 0; rd < RCOUNT; ++rd) {
                        unsigned rdRev = rotateDirReverse(rd);
                        for(unsigned reversed = 0;
                                reversed < (m_reprPerms.isUseReverse() ? 2 : 1); ++reversed)
                        {
                            cubecorners_perm ccp = reversed ?
                                cubecorners_perm::compose(crotated[rdRev].ccp, ccpNew) :
                                cubecorners_perm::compose(ccpNew, crotated[rdRev].ccp);
                            unsigned ccpReprIdx = m_reprPerms.getReprPermIdx(ccp);
                            if( m_reprPerms.getPermForIdx(ccpReprIdx) == ccp ) {
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
                                    cubecorner_orients ccoReprNew = m_reprPerms.getComposedReprOrients(
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
                                        cubeedges cenewRepr = CubecornerReprPerms::getComposedReprCubeedges(
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
            }
        }
    }
    return cubeCount;
}

bool CubesReprByDepth::searchMovesForReprPerm(unsigned reprPermIdx,
        unsigned depth, unsigned depthMax, const cube &cSearchT,
        bool reversed, cube &c, cube &cSearch) const
{
    std::vector<EdgeReprCandidateTransform> otransform;
    const CornerPermReprCubes &ccpReprCubes = m_cubesAtDepths[depth]->getAt(reprPermIdx);
    cubecorners_perm ccp = m_reprPerms.getPermForIdx(reprPermIdx);
    if( !ccpReprCubes.empty() ) {
        cubecorners_perm ccpSearch = reversed ?
            cubecorners_perm::compose(cSearchT.ccp, ccp) :
            cubecorners_perm::compose(ccp, cSearchT.ccp);
        const CornerPermReprCubes &ccpReprSearchCubes = m_cubesAtDepths[depthMax]->getFor(ccpSearch);
        if( ccpReprCubes.size() <= ccpReprSearchCubes.size() ||
                !m_reprPerms.isSingleTransform(ccpSearch) )
        {
            for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpReprCubes.ccoCubesBegin();
                    ccoCubesIt != ccpReprCubes.ccoCubesEnd(); ++ccoCubesIt)
            {
                const CornerOrientReprCubes &ccoReprCubes = *ccoCubesIt;
                cubecorner_orients cco = ccoReprCubes.getOrients();
                cubecorner_orients ccoSearch = reversed ?
                    cubecorner_orients::compose(cSearchT.cco, ccp, cco) :
                    cubecorner_orients::compose(cco, cSearchT.ccp, cSearchT.cco);
                cubecorner_orients ccoSearchRepr = m_reprPerms.getComposedReprOrients(
                        ccpSearch, ccoSearch, reversed, cSearchT.ce, otransform);
                const CornerOrientReprCubes &ccoReprSearchCubes =
                    ccpReprSearchCubes.cornerOrientCubesAt(ccoSearchRepr);
                if( ccoReprSearchCubes.empty() )
                    continue;
                cubeedges ce = CornerOrientReprCubes::findSolutionEdge(
                        ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
                if( !ce.isNil() ) {
                    cubeedges ceSearch = reversed ?
                        cubeedges::compose(cSearchT.ce, ce) :
                        cubeedges::compose(ce, cSearchT.ce);
                    cSearch = { .ccp = ccpSearch, .cco = ccoSearch, .ce = ceSearch };
                    c = { .ccp = ccp, .cco = cco, .ce = ce };
                    return true;
                }
            }
        }else{
            for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpReprSearchCubes.ccoCubesBegin();
                    ccoCubesIt != ccpReprSearchCubes.ccoCubesEnd(); ++ccoCubesIt)
            {
                const CornerOrientReprCubes &ccoReprSearchCubes = *ccoCubesIt;
                cubecorner_orients ccoSearchRepr = ccoReprSearchCubes.getOrients();
                cubecorner_orients cco = m_reprPerms.getOrientsForComposedRepr(
                        ccpSearch, ccoSearchRepr, reversed, cSearchT, otransform);
                const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(cco);
                if( ccoReprCubes.empty() )
                    continue;
                cubeedges ce = CornerOrientReprCubes::findSolutionEdge(
                        ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
                if( !ce.isNil() ) {
                    cubecorner_orients ccoSearch = reversed ?
                        cubecorner_orients::compose(cSearchT.cco, ccp, cco) :
                        cubecorner_orients::compose(cco, cSearchT.ccp, cSearchT.cco);
                    cubeedges ceSearch = reversed ?
                        cubeedges::compose(cSearchT.ce, ce) :
                        cubeedges::compose(ce, cSearchT.ce);
                    cSearch = { .ccp = ccpSearch, .cco = ccoSearch, .ce = ceSearch };
                    c = { .ccp = ccp, .cco = cco, .ce = ce };
                    return true;
                }
            }
        }
    }
    return false;
}

