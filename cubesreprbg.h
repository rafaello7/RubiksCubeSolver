#ifndef CUBESREPRBG_H
#define CUBESREPRBG_H

#include "cubedefs.h"
#include <vector>

extern const unsigned BGSpaceRotations[];
extern const unsigned BGSpaceTransforms[];

enum {
    RCOUNTBG = 10,
    TCOUNTBG = 8
};

unsigned inbgspaceCubecornerPermRepresentativeIdx(cubecorners_perm);
bool inbgspaceIsSingleTransform(cubecorners_perm);
cubeedges inbgspaceCubeedgesRepresentative(cubecorners_perm, cubeedges);
cubeedges inbgspaceCubeedgesForRepresentative(cubecorners_perm ccpSearch,
        cubeedges ceSearchRepr);

class BGSpaceCornerPermReprCubes {
    std::vector<cubeedges> m_items;
    BGSpaceCornerPermReprCubes(const BGSpaceCornerPermReprCubes&) = delete;
public:
    BGSpaceCornerPermReprCubes() = default;
    BGSpaceCornerPermReprCubes(BGSpaceCornerPermReprCubes &&other) {
        swap(other);
    }

    typedef std::vector<cubeedges>::const_iterator edges_iter;
    unsigned addCubes(const std::vector<cubeedges>&);
    bool containsCubeEdges(cubeedges) const;
    bool empty() const { return m_items.empty(); }
    size_t size() const { return m_items.size(); }
    edges_iter edgeBegin() const { return m_items.begin(); }
    edges_iter edgeEnd() const { return m_items.end(); }
    void swap(BGSpaceCornerPermReprCubes &other) {
        m_items.swap(other.m_items);
    }
};

/* A set of representative cubes from BG space reachable at specific depth.
 */
class BGSpaceCubesReprAtDepth {
    std::vector<std::pair<cubecorners_perm, BGSpaceCornerPermReprCubes>> m_cornerPermReprCubes;
public:
    typedef std::vector<std::pair<cubecorners_perm, BGSpaceCornerPermReprCubes>>::const_iterator ccpcubes_iter;
    static unsigned size();
    BGSpaceCubesReprAtDepth();
    BGSpaceCubesReprAtDepth(const BGSpaceCubesReprAtDepth&) = delete;
    ~BGSpaceCubesReprAtDepth();
    size_t cubeCount() const;
    BGSpaceCornerPermReprCubes &add(unsigned idx);
    const BGSpaceCornerPermReprCubes &getAt(unsigned idx) const {
        return m_cornerPermReprCubes[idx].second;
    }
    ccpcubes_iter ccpCubesBegin() const { return m_cornerPermReprCubes.begin(); }
    ccpcubes_iter ccpCubesEnd() const { return m_cornerPermReprCubes.end(); }
};

/* A set of representative cubes from BG space (i.e. from subset reachable from csolved
 * only by rotations: ORANGE180, RED180, YELLOW180, WHITE180, GREENCW, GREEN180, GREENCCW,
 * BLUECW, BLUE180, BLUECCW).
 */
class BGSpaceCubesReprByDepth {
    std::vector<BGSpaceCubesReprAtDepth> m_cubesAtDepths;
    unsigned m_availCount;
    BGSpaceCubesReprByDepth(const BGSpaceCubesReprByDepth&) = delete;
    BGSpaceCubesReprByDepth &operator=(const BGSpaceCubesReprByDepth&) = delete;
public:
    static bool isUseReverse();

    BGSpaceCubesReprByDepth(unsigned size)
        : m_cubesAtDepths(size), m_availCount(0)
    {
    }

    unsigned availCount() const { return m_availCount; }
    void incAvailCount() { ++m_availCount; }
    unsigned availMaxCount() const { return m_cubesAtDepths.size(); }
    const BGSpaceCubesReprAtDepth &operator[](unsigned idx) const { return m_cubesAtDepths[idx]; }
    BGSpaceCubesReprAtDepth &operator[](unsigned idx) { return m_cubesAtDepths[idx]; }
};

std::string printInSpaceMoves(const BGSpaceCubesReprByDepth&, const cube&,
        unsigned searchTd, bool movesRev = false);
unsigned long addInSpaceCubesForReprPerm(
        BGSpaceCubesReprByDepth*, unsigned permReprIdx,
        int depth);
void bgspacePermReprInit(bool useReverse);

#endif // CUBESREPRBG_H
