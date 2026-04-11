#ifndef CUBESREPR_H
#define CUBESREPR_H

#include "cpermrepr.h"
#include <vector>
#include <array>

/* The whole set of representative cubes (CubesReprByDepth) is grouped by
 * depth - number of moves needed to get the cube starting from solved one.
 * Within each depth the cubes are grouped by corner permutation. Within one
 * corner permutation the cubes are grouped by corner orientations.
 */

/* A set of representative cubes for specific corners permutation and
 * orientations.
 */
class CornerOrientReprCubes {
	std::vector<cubeedges> m_items;
    const std::array<unsigned,64> *m_orientOccur;
    CornersOrient m_orients;
    CornerOrientReprCubes(const CornerOrientReprCubes&) = delete;
    static cubeedges findSolutionEdgeMulti(
            const CornerOrientReprCubes &ccoReprCubes,
            const CornerOrientReprCubes &ccoReprSearchCubes,
            const std::vector<EdgeReprCandidateTransform> &otransform,
            bool reversed);
    static cubeedges findSolutionEdgeSingle(
            const CornerOrientReprCubes &ccoReprCubes,
            const CornerOrientReprCubes &ccoReprSearchCubes,
            const EdgeReprCandidateTransform&,
            bool reversed);
public:
    explicit CornerOrientReprCubes(CornersOrient orients)
        : m_orientOccur(nullptr), m_orients(orients)
    {
    }

    CornerOrientReprCubes(CornerOrientReprCubes &&other) {
        swap(other);
    }

    typedef std::vector<cubeedges>::const_iterator edges_iter;
    CornersOrient getOrients() const { return m_orients; }
    unsigned addCubes(const std::vector<cubeedges>&);
    void initOccur(std::array<unsigned, 64>&);
	bool containsCubeEdges(cubeedges) const;
    bool empty() const { return m_items.empty(); }
    size_t size() const { return m_items.size(); }
    edges_iter edgeBegin() const { return m_items.begin(); }
    edges_iter edgeEnd() const { return m_items.end(); }
    static cubeedges findSolutionEdge(
            const CornerOrientReprCubes &ccoReprCubes,
            const CornerOrientReprCubes &ccoReprSearchCubes,
            const std::vector<EdgeReprCandidateTransform> &otransform,
            bool reversed);
    void swap(CornerOrientReprCubes &other) {
        m_items.swap(other.m_items);
        std::swap(m_orientOccur, other.m_orientOccur);
        std::swap(m_orients, other.m_orients);
    }
};

#endif // CUBESREPR_H
