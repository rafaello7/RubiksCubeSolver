#ifndef CUBECOSETS_H
#define CUBECOSETS_H

#include "cubedefs.h"
#include <vector>

/* A set of spaces (cosets of BG space) reachable at specific depth.
 * A coset is identified by a representative cube.
 */
class CubeCosetsAtDepth {
    // std::pair<edges of representative cube, index in m_cubeArr of cubes reachable at depth>
    // the representative cube corners permutation is identity; 2^7 corner orientations
    std::vector<std::pair<cubeedges, unsigned>> m_itemsArr[2187];
    std::vector<std::vector<cube>> m_cubeArr[2187];
public:

    /* Adds a cube to coset identified by corner orients and representative cubeedges.
     */
    bool addCube(unsigned ccoReprIdx, cubeedges ceRepr, const cube&);
    bool containsCCOrients(unsigned ccoReprIdx) const {
        return !m_itemsArr[ccoReprIdx].empty();
    }
    const std::vector<cube> *getCubesForCE(unsigned ccoReprIdx, cubeedges ceRepr) const;
};

class CubeCosets {
    std::vector<CubeCosetsAtDepth> m_cubesAtDepths;
    unsigned m_availCount;
    CubeCosets(const CubeCosets&) = delete;
    CubeCosets &operator=(const CubeCosets&) = delete;
public:
    CubeCosets(unsigned size)
        : m_cubesAtDepths(size), m_availCount(0)
    {
    }

    unsigned availCount() const { return m_availCount; }
    void incAvailCount() { ++m_availCount; }
    unsigned availMaxCount() const { return m_cubesAtDepths.size(); }
    const CubeCosetsAtDepth &operator[](unsigned idx) const { return m_cubesAtDepths[idx]; }
    CubeCosetsAtDepth &operator[](unsigned idx) { return m_cubesAtDepths[idx]; }
};

#endif // CUBECOSETS_H
