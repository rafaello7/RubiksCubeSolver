#ifndef CUBESREPRBG_H
#define CUBESREPRBG_H

#include "cpermreprbg.h"
#include <vector>
#include <memory>

class BGCornerPermReprCubes {
    std::vector<cubeedges> m_items;
    BGCornerPermReprCubes(const BGCornerPermReprCubes&) = delete;
public:
    BGCornerPermReprCubes() = default;
    BGCornerPermReprCubes(BGCornerPermReprCubes &&other) {
        swap(other);
    }

    typedef std::vector<cubeedges>::const_iterator edges_iter;
    unsigned addCubes(const std::vector<cubeedges>&);
    bool containsCubeEdges(cubeedges) const;
    bool empty() const { return m_items.empty(); }
    size_t size() const { return m_items.size(); }
    edges_iter edgeBegin() const { return m_items.begin(); }
    edges_iter edgeEnd() const { return m_items.end(); }
    void swap(BGCornerPermReprCubes &other) {
        m_items.swap(other.m_items);
    }
};

/* A set of representative cubes from BG space reachable at specific depth.
 */
class BGCubesReprAtDepth {
    const BGCubecornerReprPerms &m_reprPerms;
    std::vector<BGCornerPermReprCubes> m_cornerPermReprCubes;
public:
    typedef std::vector<BGCornerPermReprCubes>::const_iterator ccpcubes_iter;
    explicit BGCubesReprAtDepth(const BGCubecornerReprPerms&);
    BGCubesReprAtDepth(const BGCubesReprAtDepth&) = delete;
    ~BGCubesReprAtDepth();
    size_t size() const { return m_cornerPermReprCubes.size(); }
    size_t cubeCount() const;
    BGCornerPermReprCubes &add(unsigned idx);
    const BGCornerPermReprCubes &getAt(unsigned idx) const {
        return m_cornerPermReprCubes[idx];
    }
    ccpcubes_iter ccpCubesBegin() const { return m_cornerPermReprCubes.begin(); }
    ccpcubes_iter ccpCubesEnd() const { return m_cornerPermReprCubes.end(); }
    cubecorners_perm getPermAt(ccpcubes_iter) const;
    bool containsCube(const cube&) const;
};

/* A set of representative cubes from BG space (i.e. from subset reachable from csolved
 * only by rotations: ORANGE180, RED180, YELLOW180, WHITE180, GREENCW, GREEN180, GREENCCW,
 * BLUECW, BLUE180, BLUECCW).
 */
class BGCubesReprByDepth {
    BGCubecornerReprPerms m_reprPerms;
    std::vector<std::shared_ptr<BGCubesReprAtDepth>> m_cubesAtDepths;
    unsigned m_availCount;
    BGCubesReprByDepth(const BGCubesReprByDepth&) = delete;
    BGCubesReprByDepth &operator=(const BGCubesReprByDepth&) = delete;
public:
    BGCubesReprByDepth(bool useReverse);

    bool isUseReverse() const;

    unsigned availCount() const { return m_availCount; }
    void incAvailCount() { ++m_availCount; }
    const BGCubesReprAtDepth &operator[](unsigned idx) const { return *m_cubesAtDepths[idx]; }
    BGCubesReprAtDepth &operator[](unsigned idx);
    std::string getMoves(const cube&, unsigned searchTd, bool movesRev = false) const;
    unsigned long addCubesForReprPerm(unsigned permReprIdx, int depth);
    bool searchMovesForReprPerm(unsigned reprPermIdx,
            unsigned depth, unsigned depthMax, const cube &cSearchT,
            bool reversed, cube &c, cube &cSearch) const;
};

#endif // CUBESREPRBG_H
