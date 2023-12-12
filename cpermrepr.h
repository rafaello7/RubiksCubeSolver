#ifndef CPERMREPR_H
#define CPERMREPR_H

#include "cubedefs.h"
#include <vector>

/* Representative cube is a cube chosen from set of similar cubes.
 * Two cubes are similar when one cube can be converted into another
 * one by a transformation (colors switch) after cube rotation, mirroring
 * (a symmetric cube), optionally combined with reversing of moves
 * sequence used to get the cube.
 *
 * To get a representative cube from the set of similar cubes, a subset
 * of cubes with specific corners permutation is chosen first. Further
 * choice is based on the cube corner orientations. When a few cubes
 * have the same corners permutation and orientations, the choice is
 * based on the cube edges.
 *
 * There are 984 or 654 distinct corner permutations, depend on use of moves
 * reversing.
 */

struct EdgeReprCandidateTransform {
    unsigned transformedIdx;
    bool reversed;
    bool symmetric;
    cubeedges ceTrans;
};

class CubecornerReprPerms {
    struct ReprCandidateTransform {
        bool reversed;
        bool symmetric;
        unsigned short transformIdx;
    };

    struct CubecornerPermToRepr {
        int reprIdx = -1;       // index in m_reprPerms
        std::vector<ReprCandidateTransform> transform;
    };

    std::vector<cubecorners_perm> m_reprPerms;
    std::vector<CubecornerPermToRepr> m_permToRepr;
    const bool m_useReverse;
public:
    explicit CubecornerReprPerms(bool useReverse);
    ~CubecornerReprPerms();

    bool isUseReverse() const { return m_useReverse; }
    unsigned reprPermCount() const { return m_reprPerms.size(); }

    /* Returns corners permutation of representative cube for a cube
     * given by the corners permutation.
     */
    cubecorners_perm getReprPerm(cubecorners_perm) const;

    /* Returns a number in range 0..653 or 0..983, depend on use of moves
     * reversing.
     */
    unsigned getReprPermIdx(cubecorners_perm) const;

    cubecorners_perm getPermForIdx(unsigned reprPermIdx) const;

    /* Returns true when the corners permutation determines uniquely
     * the transformation needed to convert a cube having the permutation to
     * representative one.
     */
    bool isSingleTransform(cubecorners_perm) const;

    /* Returns corner orientations of representative cube for a cube given
     * by corners permutation and orientations. The transform is an output
     * parameter: provides a list of transformations converting the given
     * corner permutation and orientations to the representative corner
     * orientations. All the transformations give also identical
     * representative corners permutation.
     * The transform can be passed later to cubeedgesRepresentative
     * to get the edges of representative cube.
     */
    cubecorner_orients getReprOrients(
            cubecorners_perm, cubecorner_orients,
             std::vector<EdgeReprCandidateTransform> &transform) const;

    cubecorner_orients getComposedReprOrients(
            cubecorners_perm, cubecorner_orients, bool reverse,
            cubeedges ce2, std::vector<EdgeReprCandidateTransform>&) const;
    cubecorner_orients getOrientsForComposedRepr(cubecorners_perm ccpSearch,
            cubecorner_orients ccoSearchRepr, bool reversed, const cube &cSearchT,
            std::vector<EdgeReprCandidateTransform>&) const;

    static cubeedges getReprCubeedges(cubeedges,
            const std::vector<EdgeReprCandidateTransform>&);

    static cubeedges getComposedReprCubeedges(cubeedges, bool reverse,
            const std::vector<EdgeReprCandidateTransform>&);

    cube cubeRepresentative(const cube&) const;
};

#endif // CPERMREPR_H
