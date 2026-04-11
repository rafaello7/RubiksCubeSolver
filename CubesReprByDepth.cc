#include "CubesReprByDepth.h"
#include <iostream>
#include <set>


CubesReprByDepth::CubesReprByDepth(bool useReverse)
    : m_reprPerms(useReverse), m_availCount(1)
{
    // insert the solved cube at depth 0
    m_cubesAtDepths.emplace_back(new CubesReprAtDepth(m_reprPerms));
    unsigned cornerPermReprIdx = m_reprPerms.getReprPermIdx(csolved.ccp);
    CornerPermReprCubes &ccpCubes = m_cubesAtDepths[0]->add(cornerPermReprIdx);
    std::vector<EdgeReprCandidateTransform> otransform;
    CornersOrient ccoRepr = m_reprPerms.getReprOrients(csolved.ccp,
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
    CornersPerm ccpNewRepr = m_reprPerms.getPermForIdx(reprPermIdx);
    std::set<CornersPerm> ccpChecked;
    for(unsigned trrev = 0; trrev < (m_reprPerms.isUseReverse() ? 2 : 1); ++trrev) {
        CornersPerm ccpNewReprRev = trrev ? ccpNewRepr.reverse() : ccpNewRepr;
        for(unsigned symmetric = 0; symmetric < 2; ++symmetric) {
            CornersPerm ccpNewS = symmetric ? ccpNewReprRev.symmetric() : ccpNewReprRev;
            for(unsigned td = 0; td < TCOUNT; ++td) {
                CornersPerm ccpNew = ccpNewS.transform(td);
                if( ccpChecked.find(ccpNew) == ccpChecked.end() ) {
                    ccpChecked.insert(ccpNew);
                    for(unsigned rd = 0; rd < RCOUNT; ++rd) {
                        unsigned rdRev = rotateDirReverse(rd);
                        for(unsigned reversed = 0;
                                reversed < (m_reprPerms.isUseReverse() ? 2 : 1); ++reversed)
                        {
                            CornersPerm ccp = reversed ?
                                CornersPerm::compose(crotated[rdRev].ccp, ccpNew) :
                                CornersPerm::compose(ccpNew, crotated[rdRev].ccp);
                            unsigned ccpReprIdx = m_reprPerms.getReprPermIdx(ccp);
                            if( m_reprPerms.getPermForIdx(ccpReprIdx) == ccp ) {
                                const CornerPermReprCubes &cpermReprCubesC = ccpReprCubesC.getAt(ccpReprIdx);
                                for(CornerPermReprCubes::ccocubes_iter ccoCubesItC =
                                        cpermReprCubesC.ccoCubesBegin();
                                        ccoCubesItC != cpermReprCubesC.ccoCubesEnd(); ++ccoCubesItC)
                                {
                                    const CornerOrientReprCubes &corientReprCubesC = *ccoCubesItC;
                                    CornersOrient cco = corientReprCubesC.getOrients();
                                    CornersOrient ccoNew = reversed ?
                                        CornersOrient::compose(crotated[rd].cco, ccp, cco) :
                                        CornersOrient::compose(cco, crotated[rd].ccp, crotated[rd].cco);
                                    CornersOrient ccoReprNew = m_reprPerms.getComposedReprOrients(
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
    CornersPerm ccp = m_reprPerms.getPermForIdx(reprPermIdx);
    if( !ccpReprCubes.empty() ) {
        CornersPerm ccpSearch = reversed ?
            CornersPerm::compose(cSearchT.ccp, ccp) :
            CornersPerm::compose(ccp, cSearchT.ccp);
        const CornerPermReprCubes &ccpReprSearchCubes = m_cubesAtDepths[depthMax]->getFor(ccpSearch);
        if( ccpReprCubes.size() <= ccpReprSearchCubes.size() ||
                !m_reprPerms.isSingleTransform(ccpSearch) )
        {
            for(CornerPermReprCubes::ccocubes_iter ccoCubesIt = ccpReprCubes.ccoCubesBegin();
                    ccoCubesIt != ccpReprCubes.ccoCubesEnd(); ++ccoCubesIt)
            {
                const CornerOrientReprCubes &ccoReprCubes = *ccoCubesIt;
                CornersOrient cco = ccoReprCubes.getOrients();
                CornersOrient ccoSearch = reversed ?
                    CornersOrient::compose(cSearchT.cco, ccp, cco) :
                    CornersOrient::compose(cco, cSearchT.ccp, cSearchT.cco);
                CornersOrient ccoSearchRepr = m_reprPerms.getComposedReprOrients(
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
                CornersOrient ccoSearchRepr = ccoReprSearchCubes.getOrients();
                CornersOrient cco = m_reprPerms.getOrientsForComposedRepr(
                        ccpSearch, ccoSearchRepr, reversed, cSearchT, otransform);
                const CornerOrientReprCubes &ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(cco);
                if( ccoReprCubes.empty() )
                    continue;
                cubeedges ce = CornerOrientReprCubes::findSolutionEdge(
                        ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
                if( !ce.isNil() ) {
                    CornersOrient ccoSearch = reversed ?
                        CornersOrient::compose(cSearchT.cco, ccp, cco) :
                        CornersOrient::compose(cco, cSearchT.ccp, cSearchT.cco);
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

