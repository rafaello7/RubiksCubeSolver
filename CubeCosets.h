#ifndef CUBECOSETS_H
#define CUBECOSETS_H

#include "cube.h"
#include "CubeCosetsAtDepth.h"
#include <vector>

class CubeCosets {
    std::vector<CubeCosetsAtDepth> m_cubesAtDepths;
    unsigned m_availCount;
    CubeCosets(const CubeCosets&) = delete;
    CubeCosets &operator=(const CubeCosets&) = delete;
public:
    CubeCosets(unsigned size)
        : m_cubesAtDepths(size), m_availCount(0)
    {
    }

    unsigned availCount() const { return m_availCount; }
    void incAvailCount() { ++m_availCount; }
    unsigned availMaxCount() const { return m_cubesAtDepths.size(); }
    const CubeCosetsAtDepth &operator[](unsigned idx) const { return m_cubesAtDepths[idx]; }
    CubeCosetsAtDepth &operator[](unsigned idx) { return m_cubesAtDepths[idx]; }
};

#endif // CUBECOSETS_H
