#ifndef CUBESREPRATDEPTH_H
#define CUBESREPRATDEPTH_H

#include "CornerPermReprCubes.h"
#include "ReprCornerPerms.h"
#include <vector>

/* A set of representative cubes reachable at specific depth.
 */
class CubesReprAtDepth {
    const ReprCornerPerms &m_reprPerms;
    std::vector<CornerPermReprCubes> m_cornerPermReprCubes;
public:
    typedef std::vector<CornerPermReprCubes>::const_iterator ccpcubes_iter;
    explicit CubesReprAtDepth(const ReprCornerPerms&);
    CubesReprAtDepth(const CubesReprAtDepth&) = delete;
    ~CubesReprAtDepth();
    size_t size() const { return m_cornerPermReprCubes.size(); }
    size_t cubeCount() const;
    CornerPermReprCubes &add(unsigned idx);
    void initOccur(unsigned idx);
    const CornerPermReprCubes &getAt(unsigned idx) const {
        return m_cornerPermReprCubes[idx];
    }
    const CornerPermReprCubes &getFor(CornersPerm ccp) const {
        unsigned reprPermIdx = m_reprPerms.getReprPermIdx(ccp);
        return m_cornerPermReprCubes[reprPermIdx];
    }
    ccpcubes_iter ccpCubesBegin() const { return m_cornerPermReprCubes.begin(); }
    ccpcubes_iter ccpCubesEnd() const { return m_cornerPermReprCubes.end(); }
    CornersPerm getPermAt(ccpcubes_iter) const;
};

#endif // CUBESREPRATDEPTH_H
