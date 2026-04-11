#include "CornerPermReprCubes.h"
#include <algorithm>

const CornerOrientReprCubes CornerPermReprCubes::m_coreprCubesEmpty =
    CornerOrientReprCubes(CornersOrient());

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

const CornerOrientReprCubes &CornerPermReprCubes::cornerOrientCubesAt(CornersOrient cco) const {
    ccocubes_iter ccoIt = std::lower_bound(m_coreprCubes.begin(),
            m_coreprCubes.end(), cco, ItemLessCco());
    if( ccoIt != m_coreprCubes.end() && ccoIt->getOrients() == cco )
        return *ccoIt;
    return m_coreprCubesEmpty;
}

CornerOrientReprCubes &CornerPermReprCubes::cornerOrientCubesAdd(CornersOrient cco) {
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

