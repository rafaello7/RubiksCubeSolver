package cubesrv;

import static cubesrv.CubeDefs.*;
import java.util.ArrayList;
import java.util.List;

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

public class CPermRepr {

    public static class EdgeReprCandidateTransform {
        public int transformedIdx;
        public boolean reversed;
        public boolean symmetric;
        public cubeedges ceTrans;

        public EdgeReprCandidateTransform(int transformedIdx, boolean reversed, boolean symmetric) {
            this.transformedIdx = transformedIdx;
            this.reversed = reversed;
            this.symmetric = symmetric;
            this.ceTrans = csolved.ce;
        }

        public EdgeReprCandidateTransform(int transformedIdx, boolean reversed, boolean symmetric, cubeedges ceTrans) {
            this.transformedIdx = transformedIdx;
            this.reversed = reversed;
            this.symmetric = symmetric;
            this.ceTrans = ceTrans;
        }
    }

    private static class ReprCandidateTransform {
        final boolean reversed;
        final boolean symmetric;
        final int transformIdx;
        ReprCandidateTransform(boolean reversed, boolean symmetric, int transformIdx) {
            this.reversed = reversed; this.symmetric = symmetric; this.transformIdx = transformIdx;
        }
    }

    private static class CubecornerPermToRepr {
        int reprIdx = -1;
        final List<ReprCandidateTransform> transform = new ArrayList<>();
    }

    public static class CubecornerReprPerms {
        private final boolean m_useReverse;
        private final List<cubecorners_perm> m_reprPerms = new ArrayList<>();
        private final CubecornerPermToRepr[] m_permToRepr;

        public CubecornerReprPerms(boolean useReverse) {
            this.m_useReverse = useReverse;
            m_permToRepr = new CubecornerPermToRepr[40320];
            for (int i = 0; i < 40320; i++) m_permToRepr[i] = new CubecornerPermToRepr();

            for (int pidx = 0; pidx < 40320; pidx++) {
                cubecorners_perm perm = cubecorners_perm.fromPermIdx(pidx);
                cubecorners_perm permRepr = new cubecorners_perm();
                List<ReprCandidateTransform> transform = new ArrayList<>();
                for (int rev = 0; rev < (m_useReverse ? 2 : 1); rev++) {
                    cubecorners_perm permr = (rev != 0) ? perm.reverse() : perm;
                    for (int sym = 0; sym <= 1; sym++) {
                        cubecorners_perm permchk = (sym != 0) ? permr.symmetric() : permr;
                        for (int td = 0; td < transform_dir.TCOUNT.ordinal(); td++) {
                            cubecorners_perm cand = permchk.transform(td);
                            if (td + rev + sym == 0 || cand.compareTo(permRepr) < 0) {
                                permRepr = cand;
                                transform.clear();
                                transform.add(new ReprCandidateTransform(rev != 0, sym != 0, td));
                            } else if (cand.equals(permRepr)) {
                                transform.add(new ReprCandidateTransform(rev != 0, sym != 0, td));
                            }
                        }
                    }
                }
                CubecornerPermToRepr permToReprRepr = m_permToRepr[permRepr.getPermIdx()];
                if (permToReprRepr.reprIdx < 0) {
                    permToReprRepr.reprIdx = m_reprPerms.size();
                    m_reprPerms.add(permRepr);
                }
                CubecornerPermToRepr permToRepr = m_permToRepr[perm.getPermIdx()];
                permToRepr.reprIdx = permToReprRepr.reprIdx;
                permToRepr.transform.clear();
                permToRepr.transform.addAll(transform);
            }
            System.out.println("repr size=" + m_reprPerms.size());
        }

        public boolean isUseReverse() { return m_useReverse; }
        public int reprPermCount() { return m_reprPerms.size(); }

        public cubecorners_perm getReprPerm(cubecorners_perm ccp) {
            int reprPermIdx = m_permToRepr[ccp.getPermIdx()].reprIdx;
            return m_reprPerms.get(reprPermIdx);
        }

        /* Returns a number in range 0..653 or 0..983, depend on use of moves
         * reversing.
         */
        public int getReprPermIdx(cubecorners_perm ccp) {
            return m_permToRepr[ccp.getPermIdx()].reprIdx;
        }

        public cubecorners_perm getPermForIdx(int reprPermIdx) {
            return m_reprPerms.get(reprPermIdx);
        }

        /* Returns true when the corners permutation determines uniquely
         * the transformation needed to convert a cube having the permutation to
         * representative one.
         */
        public boolean isSingleTransform(cubecorners_perm ccp) {
            return m_permToRepr[ccp.getPermIdx()].transform.size() == 1;
        }

        /* Returns corner orientations of representative cube for a cube given
         * by corners permutation and orientations. The transform is an output
         * parameter: provides a list of transformations converting the given
         * corner permutation and orientations to the representative corner
         * orientations. All the transformations give also identical
         * representative corners permutation.
         * The transform can be passed later to cubeedgesRepresentative
         * to get the edges of representative cube.
         */
        public cubecorner_orients getReprOrients(cubecorners_perm ccp, cubecorner_orients cco,
                List<EdgeReprCandidateTransform> transform) {
            CubecornerPermToRepr permToRepr = m_permToRepr[ccp.getPermIdx()];
            cubecorners_perm ccpsymm     = ccp.symmetric();
            cubecorners_perm ccprev      = ccp.reverse();
            cubecorners_perm ccprevsymm  = ccprev.symmetric();
            cubecorner_orients orepr     = new cubecorner_orients();
            cubecorner_orients ccosymm   = cco.symmetric();
            cubecorner_orients ccorev    = cco.reverse(ccp);
            cubecorner_orients ccorevsymm = ccorev.symmetric();
            boolean isInit = false;
            transform.clear();
            for (ReprCandidateTransform rct : permToRepr.transform) {
                cubecorner_orients ocand;
                if (rct.reversed) {
                    if (rct.symmetric) ocand = ccorevsymm.transform(ccprevsymm, rct.transformIdx);
                    else               ocand = ccorev.transform(ccprev, rct.transformIdx);
                } else {
                    if (rct.symmetric) ocand = ccosymm.transform(ccpsymm, rct.transformIdx);
                    else               ocand = cco.transform(ccp, rct.transformIdx);
                }
                if (!isInit || ocand.compareTo(orepr) < 0) {
                    orepr = ocand;
                    transform.clear();
                    transform.add(new EdgeReprCandidateTransform(rct.transformIdx, rct.reversed, rct.symmetric, csolved.ce));
                    isInit = true;
                } else if (ocand.equals(orepr)) {
                    transform.add(new EdgeReprCandidateTransform(rct.transformIdx, rct.reversed, rct.symmetric, csolved.ce));
                }
            }
            return orepr;
        }

        /* Gets the representative orients of cco.
         * Assumes the ccp,cco are cubecorners of a cube compose of two cubes, c1 ⊙  c2
         *
         * Parameters:
         *    ccp, cco      - cubecorners of the c1.cc composed with c2.cc
         *    reverse       - whether the ccp,cco is reversed
         *    ce2           - c2.ce
         *    transform     - output list to pass later to cubeedgesComposedRepresentative()
         *                    along with c1.ce, to get representative cubeedges of
         *                    c1.ce ⊙  ce2
         */
        public cubecorner_orients getComposedReprOrients(cubecorners_perm ccp, cubecorner_orients cco,
                boolean reverse, cubeedges ce2, List<EdgeReprCandidateTransform> transform) {
            CubecornerPermToRepr permToRepr = m_permToRepr[ccp.getPermIdx()];
            cubecorners_perm ccpsymm     = ccp.symmetric();
            cubecorners_perm ccprev      = ccp.reverse();
            cubecorners_perm ccprevsymm  = ccprev.symmetric();
            cubecorner_orients orepr     = new cubecorner_orients();
            cubecorner_orients ccosymm   = cco.symmetric();
            cubecorner_orients ccorev    = cco.reverse(ccp);
            cubecorner_orients ccorevsymm = ccorev.symmetric();
            boolean isInit = false;
            transform.clear();
            for (ReprCandidateTransform rct : permToRepr.transform) {
                cubecorner_orients ocand;
                if (rct.reversed) {
                    if (rct.symmetric) ocand = ccorevsymm.transform(ccprevsymm, rct.transformIdx);
                    else               ocand = ccorev.transform(ccprev, rct.transformIdx);
                } else {
                    if (rct.symmetric) ocand = ccosymm.transform(ccpsymm, rct.transformIdx);
                    else               ocand = cco.transform(ccp, rct.transformIdx);
                }
                if (!isInit || ocand.compareTo(orepr) < 0) {
                    orepr = ocand;
                    transform.clear();
                    transform.add(new EdgeReprCandidateTransform(rct.transformIdx, rct.reversed, rct.symmetric));
                    isInit = true;
                } else if (ocand.equals(orepr)) {
                    transform.add(new EdgeReprCandidateTransform(rct.transformIdx, rct.reversed, rct.symmetric));
                }
            }
            for (EdgeReprCandidateTransform erct : transform) {
                cubeedges ce2symm = erct.symmetric ? ce2.symmetric() : ce2;
                if (reverse) {
                    if (erct.reversed) {
                        // transform((ce1 rev) ⊙  (ce2 rev)) = cetrans ⊙  (ce1 rev) ⊙  (ce2 rev) ⊙  cetransRev
                        // ceTrans = (ce2 rev) ⊙  cetransRev
                        erct.ceTrans = cubeedges.compose(ce2symm.reverse(),
                                ctransformed[transformReverse(erct.transformedIdx)].ce);
                    } else {
                        // transform(ce2 ⊙  ce1) = cetrans ⊙  ce2 ⊙  ce1 ⊙  cetransRev
                        // ceTrans = cetrans ⊙  ce2
                        erct.ceTrans = cubeedges.compose(
                                ctransformed[erct.transformedIdx].ce, ce2symm);
                    }
                } else {
                    if (erct.reversed) {
                        // transform((ce2 rev) ⊙  (ce1 rev)) = cetrans ⊙  (ce2 rev) ⊙  (ce1 rev) ⊙  cetransRev
                        // ceTrans = cetrans ⊙  (ce2 rev)
                        erct.ceTrans = cubeedges.compose(
                                ctransformed[erct.transformedIdx].ce, ce2symm.reverse());
                    } else {
                        // transform(ce1 ⊙  ce2) = cetrans ⊙  ce1 ⊙  ce2 ⊙  cetransRev
                        // ceTrans = ce2 ⊙  cetransRev
                        erct.ceTrans = cubeedges.compose(ce2symm,
                                ctransformed[transformReverse(erct.transformedIdx)].ce);
                    }
                }
            }
            return orepr;
        }

        public cubecorner_orients getOrientsForComposedRepr(cubecorners_perm ccpSearch,
                cubecorner_orients ccoSearchRepr, boolean reversed, cube cSearchT,
                List<EdgeReprCandidateTransform> transform) {
            CubecornerPermToRepr permToRepr = m_permToRepr[ccpSearch.getPermIdx()];
            cubecorners_perm ccpSearchRepr  = m_reprPerms.get(permToRepr.reprIdx);
            cube cSearchTrev = cSearchT.reverse();
            transform.clear();
            ReprCandidateTransform rct = permToRepr.transform.get(0);
            int transformIdxRev = transformReverse(rct.transformIdx);
            cubecorner_orients ccoSearchRevSymm = ccoSearchRepr.transform(ccpSearchRepr, transformIdxRev);
            cubecorner_orients ccoSearchRev = rct.symmetric ? ccoSearchRevSymm.symmetric() : ccoSearchRevSymm;
            cubecorner_orients ccoSearch    = rct.reversed ? ccoSearchRev.reverse(ccpSearch.reverse()) : ccoSearchRev;
            cubecorner_orients cco = reversed
                ? cubecorner_orients.compose(cSearchTrev.cco, ccpSearch, ccoSearch)
                : cubecorner_orients.compose(ccoSearch, cSearchTrev.ccp, cSearchTrev.cco);
            transform.add(new EdgeReprCandidateTransform(rct.transformIdx, rct.reversed, rct.symmetric));
            EdgeReprCandidateTransform erct = transform.get(0);
            cubeedges ce2symm = erct.symmetric ? cSearchT.ce.symmetric() : cSearchT.ce;
            if (reversed) {
                if (erct.reversed) {
                    erct.ceTrans = cubeedges.compose(ce2symm.reverse(),
                            ctransformed[transformReverse(erct.transformedIdx)].ce);
                } else {
                    erct.ceTrans = cubeedges.compose(
                            ctransformed[erct.transformedIdx].ce, ce2symm);
                }
            } else {
                if (erct.reversed) {
                    erct.ceTrans = cubeedges.compose(
                            ctransformed[erct.transformedIdx].ce, ce2symm.reverse());
                } else {
                    erct.ceTrans = cubeedges.compose(ce2symm,
                            ctransformed[transformReverse(erct.transformedIdx)].ce);
                }
            }
            return cco;
        }

        public cubeedges getReprCubeedges(cubeedges ce, List<EdgeReprCandidateTransform> transform) {
            cubeedges cerepr = new cubeedges();
            if (transform.size() == 1) {
                EdgeReprCandidateTransform erct = transform.get(0);
                cubeedges cechk = erct.reversed ? ce.reverse() : ce;
                if (erct.symmetric) cechk = cechk.symmetric();
                cerepr = cechk.transform(erct.transformedIdx);
            } else {
                cubeedges cesymm   = ce.symmetric();
                cubeedges cerev    = ce.reverse();
                cubeedges cerevsymm = cerev.symmetric();
                boolean isInit = false;
                for (EdgeReprCandidateTransform erct : transform) {
                    cubeedges cand;
                    if (erct.reversed) {
                        cand = erct.symmetric ? cerevsymm.transform(erct.transformedIdx) : cerev.transform(erct.transformedIdx);
                    } else {
                        cand = erct.symmetric ? cesymm.transform(erct.transformedIdx) : ce.transform(erct.transformedIdx);
                    }
                    if (!isInit || cand.compareTo(cerepr) < 0) {
                        cerepr = cand;
                        isInit = true;
                    }
                }
            }
            return cerepr;
        }

        public static cubeedges getComposedReprCubeedges(cubeedges ce, boolean reverse,
                List<EdgeReprCandidateTransform> transform) {
            cubeedges cerepr = new cubeedges();
            if (transform.size() == 1) {
                EdgeReprCandidateTransform erct = transform.get(0);
                cubeedges cesymm = erct.symmetric ? ce.symmetric() : ce;
                if (reverse) {
                    if (erct.reversed) {
                        cerepr = cubeedges.compose3revmid(ctransformed[erct.transformedIdx].ce, cesymm, erct.ceTrans);
                    } else {
                        cerepr = cubeedges.compose3(erct.ceTrans, cesymm, ctransformed[transformReverse(erct.transformedIdx)].ce);
                    }
                } else {
                    if (erct.reversed) {
                        cerepr = cubeedges.compose3revmid(erct.ceTrans, cesymm, ctransformed[transformReverse(erct.transformedIdx)].ce);
                    } else {
                        cerepr = cubeedges.compose3(ctransformed[erct.transformedIdx].ce, cesymm, erct.ceTrans);
                    }
                }
            } else {
                boolean isInit = false;
                for (EdgeReprCandidateTransform erct : transform) {
                    cubeedges cesymm = erct.symmetric ? ce.symmetric() : ce;
                    cubeedges cand;
                    if (reverse) {
                        if (erct.reversed) {
                            cand = cubeedges.compose3revmid(ctransformed[erct.transformedIdx].ce, cesymm, erct.ceTrans);
                        } else {
                            cand = cubeedges.compose3(erct.ceTrans, cesymm, ctransformed[transformReverse(erct.transformedIdx)].ce);
                        }
                    } else {
                        if (erct.reversed) {
                            cand = cubeedges.compose3revmid(erct.ceTrans, cesymm, ctransformed[transformReverse(erct.transformedIdx)].ce);
                        } else {
                            cand = cubeedges.compose3(ctransformed[erct.transformedIdx].ce, cesymm, erct.ceTrans);
                        }
                    }
                    if (!isInit || cand.compareTo(cerepr) < 0) {
                        cerepr = cand;
                        isInit = true;
                    }
                }
            }
            return cerepr;
        }

        public cube cubeRepresentative(cube c) {
            List<EdgeReprCandidateTransform> transform = new ArrayList<>();
            cubecorners_perm ccpRepr = getReprPerm(c.ccp);
            cubecorner_orients ccoRepr = getReprOrients(c.ccp, c.cco, transform);
            cubeedges ceRepr = getReprCubeedges(c.ce, transform);
            return new cube(ccpRepr, ccoRepr, ceRepr);
        }
    }
}

