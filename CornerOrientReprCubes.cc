#include "CornerOrientReprCubes.h"
#include "ReprCornerPerms.h"
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
        cubeedges ceSearchRepr = ReprCornerPerms::getComposedReprCubeedges(
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

