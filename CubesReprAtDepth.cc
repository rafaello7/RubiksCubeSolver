#include "CubesReprAtDepth.h"

CubesReprAtDepth::CubesReprAtDepth(const CubecornerReprPerms &reprPerms)
    : m_reprPerms(reprPerms), m_cornerPermReprCubes(reprPerms.reprPermCount())
{
}

CubesReprAtDepth::~CubesReprAtDepth() {
}

size_t CubesReprAtDepth::cubeCount() const {
    size_t res = 0;
    for(const CornerPermReprCubes &reprCube : m_cornerPermReprCubes)
        res += reprCube.cubeCount();
    return res;
}

void CubesReprAtDepth::initOccur(unsigned idx)
{
    m_cornerPermReprCubes[idx].initOccur();
}

CornerPermReprCubes &CubesReprAtDepth::add(unsigned idx) {
    return m_cornerPermReprCubes[idx];
}

CornersPerm CubesReprAtDepth::getPermAt(ccpcubes_iter it) const
{
    unsigned reprPermIdx = std::distance(m_cornerPermReprCubes.begin(), it);
    return m_reprPerms.getPermForIdx(reprPermIdx);
}

