#ifndef CORNERPERMREPRCUBES_H
#define CORNERPERMREPRCUBES_H

#include "CornerOrientReprCubes.h"

/* A set of representative cubes for specific corners permutation.
 */
class CornerPermReprCubes {
    static const CornerOrientReprCubes m_coreprCubesEmpty;
    std::vector<CornerOrientReprCubes> m_coreprCubes;
    std::vector<std::array<unsigned,64>> m_orientOccurMem;
    struct ItemLessCco {
        bool operator()(const CornerOrientReprCubes &a, const CornersOrient &b) {
            return a.getOrients() < b;
        }
    };

public:
    typedef std::vector<CornerOrientReprCubes>::const_iterator ccocubes_iter;
	CornerPermReprCubes();
	~CornerPermReprCubes();
    bool empty() const { return m_coreprCubes.empty(); }
    unsigned size() const { return m_coreprCubes.size(); }
    size_t cubeCount() const;
    void initOccur();
    const CornerOrientReprCubes &cornerOrientCubesAt(CornersOrient cco) const;
	CornerOrientReprCubes &cornerOrientCubesAdd(CornersOrient);
    ccocubes_iter ccoCubesBegin() const { return m_coreprCubes.begin(); }
    ccocubes_iter ccoCubesEnd() const { return m_coreprCubes.end(); }
};

#endif // CORNERPERMREPRCUBES_H
