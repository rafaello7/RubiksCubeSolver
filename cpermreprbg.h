#ifndef CPERMREPRBG_H
#define CPERMREPRBG_H

#include "cube.h"
#include <vector>

enum {
    RCOUNTBG = 10,
    TCOUNTBG = 8
};

extern const unsigned BGSpaceRotations[];
extern const unsigned BGSpaceTransforms[];

class BGCubecornerReprPerms {
    struct ReprCandidateTransform {
        bool reversed;
        bool symmetric;
        unsigned short transformIdx;
    };

    struct CubecornerPermToRepr {
        int reprIdx = -1;       // index in m_reprPerms
        std::vector<ReprCandidateTransform> transform;
    };

    std::vector<CornersPerm> m_reprPerms;
    std::vector<CubecornerPermToRepr> m_permToRepr;
    const bool m_useReverse;
public:
    explicit BGCubecornerReprPerms(bool useReverse);
    ~BGCubecornerReprPerms();

    bool isUseReverse() const { return m_useReverse; }
    unsigned reprPermCount() const { return m_reprPerms.size(); }

    /* Returns corners permutation of representative cube for a cube
     * given by the corners permutation.
     */
    CornersPerm getReprPerm(CornersPerm) const;

    /* Returns a number in range 0..1671 or 0..2767, depend on use of moves
     * reversing.
     */
    unsigned getReprPermIdx(CornersPerm) const;

    CornersPerm getPermForIdx(unsigned reprPermIdx) const;

    /* Returns true when the corners permutation determines uniquely
     * the transformation needed to convert a cube having the permutation to
     * representative one.
     */
    bool isSingleTransform(CornersPerm) const;

    cubeedges getReprCubeedges(CornersPerm, cubeedges) const;
    cube cubeRepresentative(const cube&) const;
    cubeedges getCubeedgesForRepresentative(
            CornersPerm ccpSearch, cubeedges ceSearchRepr) const;
};

#endif // CPERMREPRBG_H
