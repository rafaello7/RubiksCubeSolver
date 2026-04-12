#ifndef BGCUBESREPRATDEPTH_H
#define BGCUBESREPRATDEPTH_H

#include "BGReprCornerPerms.h"
#include "BGCornerPermReprCubes.h"
#include <vector>

/* A set of representative cubes from BG space reachable at specific depth.
 */
class BGCubesReprAtDepth {
    const BGReprCornerPerms &m_reprPerms;
    std::vector<BGCornerPermReprCubes> m_cornerPermReprCubes;
public:
    typedef std::vector<BGCornerPermReprCubes>::const_iterator ccpcubes_iter;
    explicit BGCubesReprAtDepth(const BGReprCornerPerms&);
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
    CornersPerm getPermAt(ccpcubes_iter) const;
    bool containsCube(const cube&) const;
};

#endif // BGCUBESREPRATDEPTH_H
