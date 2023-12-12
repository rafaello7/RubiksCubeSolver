#ifndef CUBECOSETS_H
#define CUBECOSETS_H

#include "cubedefs.h"
#include <vector>

/* A set of spaces (cosets of BG space) reachable at specific depth.
 */
class SpaceReprCubesAtDepth {
    // std::pair<edges of representative cube, index in m_cubeArr of cubes reachable at depth>
    // the representative cube corners permutation is identity; 2^7 corner orientations
    std::vector<std::pair<cubeedges, unsigned>> m_itemsArr[2187];
    std::vector<std::vector<cube>> m_cubeArr[2187];
public:
    bool addCube(unsigned ccoReprIdx, cubeedges ceRepr, const cube&);
    bool containsCCOrients(unsigned ccoReprIdx) const {
        return !m_itemsArr[ccoReprIdx].empty();
    }
    const std::vector<cube> *getCubesForCE(unsigned ccoReprIdx, cubeedges ceRepr) const;
};

class SpaceReprCubes {
    std::vector<SpaceReprCubesAtDepth> m_cubesAtDepths;
    unsigned m_availCount;
    SpaceReprCubes(const SpaceReprCubes&) = delete;
    SpaceReprCubes &operator=(const SpaceReprCubes&) = delete;
public:
    SpaceReprCubes(unsigned size)
        : m_cubesAtDepths(size), m_availCount(0)
    {
    }

    unsigned availCount() const { return m_availCount; }
    void incAvailCount() { ++m_availCount; }
    unsigned availMaxCount() const { return m_cubesAtDepths.size(); }
    const SpaceReprCubesAtDepth &operator[](unsigned idx) const { return m_cubesAtDepths[idx]; }
    SpaceReprCubesAtDepth &operator[](unsigned idx) { return m_cubesAtDepths[idx]; }
};

#endif // CUBECOSETS_H