#ifndef BGCUBESREPRBYDEPTH_H
#define BGCUBESREPRBYDEPTH_H

#include "BGReprCornerPerms.h"
#include "BGCubesReprAtDepth.h"
#include <vector>

/* A set of representative cubes from BG space (i.e. from subset reachable from csolved
 * only by rotations: ORANGE180, RED180, YELLOW180, WHITE180, GREENCW, GREEN180, GREENCCW,
 * BLUECW, BLUE180, BLUECCW).
 */
class BGCubesReprByDepth {
    BGReprCornerPerms m_reprPerms;
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

#endif // BGCUBESREPRBYDEPTH_H
