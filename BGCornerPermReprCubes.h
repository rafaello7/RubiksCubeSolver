#ifndef BGCORNERPERMREPRCUBES_H
#define BGCORNERPERMREPRCUBES_H

#include "BGReprCornerPerms.h"
#include <vector>
#include <memory>

enum {
    TWOPHASE_DEPTH2_MAX = 8u
};

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

#endif // BGCORNERPERMREPRCUBES_H
