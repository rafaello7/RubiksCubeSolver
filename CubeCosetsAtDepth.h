#ifndef CUBECOSETSATDEPTH_H
#define CUBECOSETSATDEPTH_H

#include "cube.h"
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

#endif // CUBECOSETSATDEPTH_H
