#include "cubecosets.h"


bool SpaceReprCubesAtDepth::addCube(unsigned ccoReprIdx, cubeedges ceRepr, const cube &c)
{
    std::vector<std::pair<cubeedges, unsigned>> &items = m_itemsArr[ccoReprIdx];

    std::vector<std::pair<cubeedges, unsigned>>::iterator reprIt = std::lower_bound(
            items.begin(), items.end(), std::make_pair(ceRepr, 0u));
    bool res = reprIt == items.end() || reprIt->first != ceRepr;
    if( res ) {
        unsigned idx = m_cubeArr[ccoReprIdx].size();
        m_cubeArr[ccoReprIdx].emplace_back();
        if( reprIt == items.end() ) {
            items.emplace_back(std::make_pair(ceRepr, idx));
            reprIt = items.end()-1;
        }else{
            unsigned reprPos = std::distance(items.begin(), reprIt);
            items.resize(items.size()+1);
            reprIt = items.begin() + reprPos;
            std::copy_backward(reprIt, items.end()-1, items.end());
            *reprIt = std::make_pair(ceRepr, idx);
        }
    }
    m_cubeArr[ccoReprIdx][reprIt->second].push_back(c);
    return res;
}

const std::vector<cube> *SpaceReprCubesAtDepth::getCubesForCE(unsigned ccoReprIdx, cubeedges ceRepr) const
{
    const std::vector<std::pair<cubeedges, unsigned>> &items = m_itemsArr[ccoReprIdx];
    std::vector<std::pair<cubeedges, unsigned>>::const_iterator reprIt = std::lower_bound(
            items.begin(), items.end(), std::make_pair(ceRepr, 0u));
	return reprIt != items.end() && reprIt->first == ceRepr ?
        &m_cubeArr[ccoReprIdx][reprIt->second] : NULL;
}

