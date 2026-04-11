#ifndef CUBESREPRBYDEPTH_H
#define CUBESREPRBYDEPTH_H

#include "CubesReprAtDepth.h"
#include <vector>
#include <memory>

class CubesReprByDepth {
    CubecornerReprPerms m_reprPerms;
    std::vector<std::shared_ptr<CubesReprAtDepth>> m_cubesAtDepths;
    unsigned m_availCount;

    CubesReprByDepth(const CubesReprByDepth&) = delete;
    CubesReprByDepth &operator=(const CubesReprByDepth&) = delete;
public:
    CubesReprByDepth(bool useReverse);
    bool isUseReverse() const;

    unsigned availCount() const { return m_availCount; }
    void incAvailCount() { ++m_availCount; }
    const CubesReprAtDepth &operator[](unsigned idx) const { return *m_cubesAtDepths[idx]; }
    CubesReprAtDepth &operator[](unsigned idx);
    CornersPerm getReprPermForIdx(unsigned reprPermIdx) const {
        return m_reprPerms.getPermForIdx(reprPermIdx);
    }

    // the cube passed as parameter shall exist in the set
    // returns the list of moves separated by spaces
    std::string getMoves(const cube&, bool movesRev = false) const;

    /* Auxiliary function to fill the cube set. Adds cubes for
     * the specified representative index at the specified depth.
     * Before call the set should be filled up to depth-1.
     */
    unsigned long addCubesForReprPerm(unsigned reprPermIdx, int depth);
    bool searchMovesForReprPerm(unsigned reprPermIdx,
            unsigned depth, unsigned depthMax, const cube &cSearchT,
            bool reversed, cube &c) const;
};

#endif // CUBESREPRBYDEPTH_H
