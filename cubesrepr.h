#ifndef CUBESREPR_H
#define CUBESREPR_H

#include "cpermrepr.h"
#include <array>
#include <string>
#include <vector>
#include <memory>

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
    cubecorner_orients m_orients;
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
    explicit CornerOrientReprCubes(cubecorner_orients orients)
        : m_orientOccur(nullptr), m_orients(orients)
    {
    }

    CornerOrientReprCubes(CornerOrientReprCubes &&other) {
        swap(other);
    }

    typedef std::vector<cubeedges>::const_iterator edges_iter;
    cubecorner_orients getOrients() const { return m_orients; }
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

/* A set of representative cubes for specific corners permutation.
 */
class CornerPermReprCubes {
    static const CornerOrientReprCubes m_coreprCubesEmpty;
    std::vector<CornerOrientReprCubes> m_coreprCubes;
    std::vector<std::array<unsigned,64>> m_orientOccurMem;
    struct ItemLessCco {
        bool operator()(const CornerOrientReprCubes &a, const cubecorner_orients &b) {
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
    const CornerOrientReprCubes &cornerOrientCubesAt(cubecorner_orients cco) const;
	CornerOrientReprCubes &cornerOrientCubesAdd(cubecorner_orients);
    ccocubes_iter ccoCubesBegin() const { return m_coreprCubes.begin(); }
    ccocubes_iter ccoCubesEnd() const { return m_coreprCubes.end(); }
};

/* A set of representative cubes reachable at specific depth.
 */
class CubesReprAtDepth {
    const CubecornerReprPerms &m_reprPerms;
    std::vector<CornerPermReprCubes> m_cornerPermReprCubes;
public:
    typedef std::vector<CornerPermReprCubes>::const_iterator ccpcubes_iter;
    explicit CubesReprAtDepth(const CubecornerReprPerms&);
    CubesReprAtDepth(const CubesReprAtDepth&) = delete;
    ~CubesReprAtDepth();
    size_t size() const { return m_cornerPermReprCubes.size(); }
    size_t cubeCount() const;
    CornerPermReprCubes &add(unsigned idx);
    void initOccur(unsigned idx);
    const CornerPermReprCubes &getAt(unsigned idx) const {
        return m_cornerPermReprCubes[idx];
    }
    const CornerPermReprCubes &getFor(cubecorners_perm ccp) const {
        unsigned reprPermIdx = m_reprPerms.getReprPermIdx(ccp);
        return m_cornerPermReprCubes[reprPermIdx];
    }
    ccpcubes_iter ccpCubesBegin() const { return m_cornerPermReprCubes.begin(); }
    ccpcubes_iter ccpCubesEnd() const { return m_cornerPermReprCubes.end(); }
    cubecorners_perm getPermAt(ccpcubes_iter) const;
};

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
    cubecorners_perm getReprPermForIdx(unsigned reprPermIdx) const {
        return m_reprPerms.getPermForIdx(reprPermIdx);
    }

    // the cube passed as parameter shall exist in the set
    // returns the list of moves separated by spaces
    std::string getMoves(const cube&, bool movesRev = false) const;
    unsigned long addCubesForReprPerm(unsigned reprPermIdx, int depth);
    bool searchMovesForReprPerm(unsigned reprPermIdx,
            unsigned depth, unsigned depthMax, const cube &cSearchT,
            bool reversed, cube &c, cube &cSearch) const;
};

#endif // CUBESREPR_H
