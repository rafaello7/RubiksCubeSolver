#include "BGCornerPermReprCubes.h"
#include <algorithm>

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

