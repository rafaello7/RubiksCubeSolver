#include "BGCubesReprAtDepth.h"

BGCubesReprAtDepth::BGCubesReprAtDepth(const BGReprCornerPerms &reprPerms)
    : m_reprPerms(reprPerms), m_cornerPermReprCubes(reprPerms.reprPermCount())
{
}

BGCubesReprAtDepth::~BGCubesReprAtDepth() {
}

size_t BGCubesReprAtDepth::cubeCount() const {
    size_t res = 0;
    for(const BGCornerPermReprCubes &reprCube : m_cornerPermReprCubes)
        res += reprCube.size();
    return res;
}

BGCornerPermReprCubes &BGCubesReprAtDepth::add(unsigned idx) {
    return m_cornerPermReprCubes[idx];
}

CornersPerm BGCubesReprAtDepth::getPermAt(ccpcubes_iter it) const
{
    unsigned reprPermIdx = std::distance(m_cornerPermReprCubes.begin(), it);
    return m_reprPerms.getPermForIdx(reprPermIdx);
}

bool BGCubesReprAtDepth::containsCube(const cube &c) const {
    unsigned ccpReprSearchIdx = m_reprPerms.getReprPermIdx(c.ccp);
    const BGCornerPermReprCubes &ccpReprSearchCubes =
        m_cornerPermReprCubes[ccpReprSearchIdx];
    cubeedges ceSearchRepr = m_reprPerms.getReprCubeedges(c.ccp, c.ce);
    return ccpReprSearchCubes.containsCubeEdges(ceSearchRepr);
}

