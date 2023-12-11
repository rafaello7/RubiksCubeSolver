#ifndef CUBESREPR_H
#define CUBESREPR_H

#include <array>
#include <string>
#include <vector>
#include "cubedefs.h"

/* Representative cube is a cube chosen from set of similar cubes.
 * Two cubes are similar when one cube can be converted into another
 * one by a transformation (colors switch) after cube rotation, mirroring
 * (a symmetric cube), optionally combined with reversing of moves
 * sequence used to get the cube.
 *
 * The whole set of representative cubes (CubesReprByDepth) is grouped by
 * depth - number of moves needed to get the cube starting from solved one.
 * Within each depth the cubes are grouped by corner permutation. Within one
 * corner permutation the cubes are grouped by corner orientations.
 *
 * To get a representative cube from the set of similar cubes, a subset
 * of cubes with specific corners permutation is chosen first. Further
 * choice is based on the cube corner orientations. When a few cubes
 * have the same corners permutation and orientations, the choice is
 * based on the cube edges.
 *
 * There are 984 or 654 distinct corner permutations, depend on use of moves
 * reversing.
 */

struct EdgeReprCandidateTransform {
    unsigned transformedIdx;
    bool reversed;
    bool symmetric;
    cubeedges ceTrans;
};

/* Returns corners permutation of representative cube for a cube
 * given by the corners permutation.
 */
cubecorners_perm cubecornerPermsRepresentative(cubecorners_perm);

/* Returns a number in range 0..653 or 0..983, depend on use of moves
 * reversing.
 */
unsigned cubecornerPermRepresentativeIdx(cubecorners_perm);

/* Returns true when the corners permutation determines uniquely
 * the transformation needed to convert a cube having the permutation to
 * representative one.
 */
bool isSingleTransform(cubecorners_perm);

/* Returns corner orientations of representative cube for a cube given
 * by corners permutation and orientations. The transform is an output
 * parameter: provides a list of transformations converting the given
 * corner permutation and orientations to the representative corner
 * orientations. All the transformations give also identical
 * representative corners permutation.
 * The transform can be passed later to cubeedgesRepresentative
 * to get the edges of representative cube.
 */
cubecorner_orients cubecornerOrientsRepresentative(
        cubecorners_perm, cubecorner_orients,
         std::vector<EdgeReprCandidateTransform> &transform);

cubecorner_orients cubecornerOrientsComposedRepresentative(
        cubecorners_perm, cubecorner_orients, bool reverse,
        cubeedges ce2, std::vector<EdgeReprCandidateTransform>&);
cubecorner_orients cubecornerOrientsForComposedRepresentative(cubecorners_perm ccpSearch,
        cubecorner_orients ccoSearchRepr, bool reversed, const cube &cSearchT,
        std::vector<EdgeReprCandidateTransform>&);

cubeedges cubeedgesRepresentative(cubeedges,
        const std::vector<EdgeReprCandidateTransform>&);

cubeedges cubeedgesComposedRepresentative(cubeedges, bool reverse,
        const std::vector<EdgeReprCandidateTransform>&);

cube cubeRepresentative(const cube&);

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
    std::vector<std::pair<cubecorners_perm, CornerPermReprCubes>> m_cornerPermReprCubes;
public:
    typedef std::vector<std::pair<cubecorners_perm, CornerPermReprCubes>>::const_iterator ccpcubes_iter;
    static unsigned size();
    CubesReprAtDepth();
    CubesReprAtDepth(const CubesReprAtDepth&) = delete;
    ~CubesReprAtDepth();
    size_t cubeCount() const;
    CornerPermReprCubes &add(unsigned idx);
    void initOccur(unsigned idx);
    const CornerPermReprCubes &getAt(unsigned idx) const {
        return m_cornerPermReprCubes[idx].second;
    }
    ccpcubes_iter ccpCubesBegin() const { return m_cornerPermReprCubes.begin(); }
    ccpcubes_iter ccpCubesEnd() const { return m_cornerPermReprCubes.end(); }
};

class CubesReprByDepth {
    std::vector<CubesReprAtDepth> m_cubesAtDepths;
    unsigned m_availCount;
    CubesReprByDepth(const CubesReprByDepth&) = delete;
    CubesReprByDepth &operator=(const CubesReprByDepth&) = delete;
public:
    static bool isUseReverse();

    CubesReprByDepth(unsigned size)
        : m_cubesAtDepths(size), m_availCount(0)
    {
    }

    unsigned availCount() const { return m_availCount; }
    void incAvailCount() { ++m_availCount; }
    const CubesReprAtDepth &operator[](unsigned idx) const { return m_cubesAtDepths[idx]; }
    CubesReprAtDepth &operator[](unsigned idx) { return m_cubesAtDepths[idx]; }
};

std::string printMoves(const CubesReprByDepth &cubesByDepth, const cube &c, bool movesRev = false);
unsigned long addCubesForReprPerm(CubesReprByDepth*, unsigned permReprIdx, int depth);
void permReprInit(bool useReverse);

#endif // CUBESREPR_H
