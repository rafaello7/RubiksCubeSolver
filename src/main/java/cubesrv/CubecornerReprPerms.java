package cubesrv;

import static cubesrv.CubeDefs.*;
import java.util.ArrayList;
import java.util.List;

public class CubecornerReprPerms {
    private static class ReprCandidateTransform {
        final boolean reversed;
        final boolean symmetric;
        final int transformIdx;

        ReprCandidateTransform(boolean reversed, boolean symmetric, int transformIdx) {
            this.reversed = reversed;
            this.symmetric = symmetric;
            this.transformIdx = transformIdx;
        }
    }

    private static class CubecornerPermToRepr {
        int reprIdx = -1;
        final List<ReprCandidateTransform> transform = new ArrayList<>();
    }

    private final boolean m_useReverse;
    private final List<CubecornersPerm> m_reprPerms = new ArrayList<>();
    private final CubecornerPermToRepr[] m_permToRepr;

    public CubecornerReprPerms(boolean useReverse) {
        this.m_useReverse = useReverse;
        m_permToRepr = new CubecornerPermToRepr[40320];
        for (int i = 0; i < 40320; i++) m_permToRepr[i] = new CubecornerPermToRepr();

        for (int pidx = 0; pidx < 40320; pidx++) {
            CubecornersPerm perm = CubecornersPerm.fromPermIdx(pidx);
            CubecornersPerm permRepr = new CubecornersPerm();
            List<ReprCandidateTransform> transform = new ArrayList<>();
            for (int rev = 0; rev < (m_useReverse ? 2 : 1); rev++) {
                CubecornersPerm permr = (rev != 0) ? perm.reverse() : perm;
                for (int sym = 0; sym <= 1; sym++) {
                    CubecornersPerm permchk = (sym != 0) ? permr.symmetric() : permr;
                    for (int td = 0; td < TransformDir.TCOUNT.ordinal(); td++) {
                        CubecornersPerm cand = permchk.transform(td);
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

    public boolean isUseReverse() {
        return m_useReverse;
    }

    public int reprPermCount() {
        return m_reprPerms.size();
    }

    public CubecornersPerm getReprPerm(CubecornersPerm ccp) {
        int reprPermIdx = m_permToRepr[ccp.getPermIdx()].reprIdx;
        return m_reprPerms.get(reprPermIdx);
    }

    /* Returns a number in range 0..653 or 0..983, depend on use of moves
     * reversing.
     */
    public int getReprPermIdx(CubecornersPerm ccp) {
        return m_permToRepr[ccp.getPermIdx()].reprIdx;
    }

    public CubecornersPerm getPermForIdx(int reprPermIdx) {
        return m_reprPerms.get(reprPermIdx);
    }

    /* Returns true when the corners permutation determines uniquely
     * the transformation needed to convert a cube having the permutation to
     * representative one.
     */
    public boolean isSingleTransform(CubecornersPerm ccp) {
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
    public CubecornerOrients getReprOrients(CubecornersPerm ccp, CubecornerOrients cco,
                                            List<EdgeReprCandidateTransform> transform) {
        CubecornerPermToRepr permToRepr = m_permToRepr[ccp.getPermIdx()];
        CubecornersPerm ccpsymm = ccp.symmetric();
        CubecornersPerm ccprev = ccp.reverse();
        CubecornersPerm ccprevsymm = ccprev.symmetric();
        CubecornerOrients orepr = new CubecornerOrients();
        CubecornerOrients ccosymm = cco.symmetric();
        CubecornerOrients ccorev = cco.reverse(ccp);
        CubecornerOrients ccorevsymm = ccorev.symmetric();
        boolean isInit = false;
        transform.clear();
        for (ReprCandidateTransform rct : permToRepr.transform) {
            CubecornerOrients ocand;
            if (rct.reversed) {
                if (rct.symmetric) ocand = ccorevsymm.transform(ccprevsymm, rct.transformIdx);
                else ocand = ccorev.transform(ccprev, rct.transformIdx);
            } else {
                if (rct.symmetric) ocand = ccosymm.transform(ccpsymm, rct.transformIdx);
                else ocand = cco.transform(ccp, rct.transformIdx);
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
    public CubecornerOrients getComposedReprOrients(CubecornersPerm ccp, CubecornerOrients cco,
                                                    boolean reverse, CubeEdges ce2, List<EdgeReprCandidateTransform> transform) {
        CubecornerPermToRepr permToRepr = m_permToRepr[ccp.getPermIdx()];
        CubecornersPerm ccpsymm = ccp.symmetric();
        CubecornersPerm ccprev = ccp.reverse();
        CubecornersPerm ccprevsymm = ccprev.symmetric();
        CubecornerOrients orepr = new CubecornerOrients();
        CubecornerOrients ccosymm = cco.symmetric();
        CubecornerOrients ccorev = cco.reverse(ccp);
        CubecornerOrients ccorevsymm = ccorev.symmetric();
        boolean isInit = false;
        transform.clear();
        for (ReprCandidateTransform rct : permToRepr.transform) {
            CubecornerOrients ocand;
            if (rct.reversed) {
                if (rct.symmetric) ocand = ccorevsymm.transform(ccprevsymm, rct.transformIdx);
                else ocand = ccorev.transform(ccprev, rct.transformIdx);
            } else {
                if (rct.symmetric) ocand = ccosymm.transform(ccpsymm, rct.transformIdx);
                else ocand = cco.transform(ccp, rct.transformIdx);
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
            CubeEdges ce2symm = erct.symmetric ? ce2.symmetric() : ce2;
            if (reverse) {
                if (erct.reversed) {
                    // transform((ce1 rev) ⊙  (ce2 rev)) = cetrans ⊙  (ce1 rev) ⊙  (ce2 rev) ⊙  cetransRev
                    // ceTrans = (ce2 rev) ⊙  cetransRev
                    erct.ceTrans = CubeEdges.compose(ce2symm.reverse(),
                            ctransformed[transformReverse(erct.transformedIdx)].ce);
                } else {
                    // transform(ce2 ⊙  ce1) = cetrans ⊙  ce2 ⊙  ce1 ⊙  cetransRev
                    // ceTrans = cetrans ⊙  ce2
                    erct.ceTrans = CubeEdges.compose(
                            ctransformed[erct.transformedIdx].ce, ce2symm);
                }
            } else {
                if (erct.reversed) {
                    // transform((ce2 rev) ⊙  (ce1 rev)) = cetrans ⊙  (ce2 rev) ⊙  (ce1 rev) ⊙  cetransRev
                    // ceTrans = cetrans ⊙  (ce2 rev)
                    erct.ceTrans = CubeEdges.compose(
                            ctransformed[erct.transformedIdx].ce, ce2symm.reverse());
                } else {
                    // transform(ce1 ⊙  ce2) = cetrans ⊙  ce1 ⊙  ce2 ⊙  cetransRev
                    // ceTrans = ce2 ⊙  cetransRev
                    erct.ceTrans = CubeEdges.compose(ce2symm,
                            ctransformed[transformReverse(erct.transformedIdx)].ce);
                }
            }
        }
        return orepr;
    }

    public CubecornerOrients getOrientsForComposedRepr(CubecornersPerm ccpSearch,
                                                       CubecornerOrients ccoSearchRepr, boolean reversed, Cube cSearchT,
                                                       List<EdgeReprCandidateTransform> transform) {
        CubecornerPermToRepr permToRepr = m_permToRepr[ccpSearch.getPermIdx()];
        CubecornersPerm ccpSearchRepr = m_reprPerms.get(permToRepr.reprIdx);
        Cube cSearchTrev = cSearchT.reverse();
        transform.clear();
        ReprCandidateTransform rct = permToRepr.transform.get(0);
        int transformIdxRev = transformReverse(rct.transformIdx);
        CubecornerOrients ccoSearchRevSymm = ccoSearchRepr.transform(ccpSearchRepr, transformIdxRev);
        CubecornerOrients ccoSearchRev = rct.symmetric ? ccoSearchRevSymm.symmetric() : ccoSearchRevSymm;
        CubecornerOrients ccoSearch = rct.reversed ? ccoSearchRev.reverse(ccpSearch.reverse()) : ccoSearchRev;
        CubecornerOrients cco = reversed
                ? CubecornerOrients.compose(cSearchTrev.cco, ccpSearch, ccoSearch)
                : CubecornerOrients.compose(ccoSearch, cSearchTrev.ccp, cSearchTrev.cco);
        transform.add(new EdgeReprCandidateTransform(rct.transformIdx, rct.reversed, rct.symmetric));
        EdgeReprCandidateTransform erct = transform.get(0);
        CubeEdges ce2symm = erct.symmetric ? cSearchT.ce.symmetric() : cSearchT.ce;
        if (reversed) {
            if (erct.reversed) {
                erct.ceTrans = CubeEdges.compose(ce2symm.reverse(),
                        ctransformed[transformReverse(erct.transformedIdx)].ce);
            } else {
                erct.ceTrans = CubeEdges.compose(
                        ctransformed[erct.transformedIdx].ce, ce2symm);
            }
        } else {
            if (erct.reversed) {
                erct.ceTrans = CubeEdges.compose(
                        ctransformed[erct.transformedIdx].ce, ce2symm.reverse());
            } else {
                erct.ceTrans = CubeEdges.compose(ce2symm,
                        ctransformed[transformReverse(erct.transformedIdx)].ce);
            }
        }
        return cco;
    }

    public CubeEdges getReprCubeedges(CubeEdges ce, List<EdgeReprCandidateTransform> transform) {
        CubeEdges cerepr = new CubeEdges();
        if (transform.size() == 1) {
            EdgeReprCandidateTransform erct = transform.get(0);
            CubeEdges cechk = erct.reversed ? ce.reverse() : ce;
            if (erct.symmetric) cechk = cechk.symmetric();
            cerepr = cechk.transform(erct.transformedIdx);
        } else {
            CubeEdges cesymm = ce.symmetric();
            CubeEdges cerev = ce.reverse();
            CubeEdges cerevsymm = cerev.symmetric();
            boolean isInit = false;
            for (EdgeReprCandidateTransform erct : transform) {
                CubeEdges cand;
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

    public static CubeEdges getComposedReprCubeedges(CubeEdges ce, boolean reverse,
                                                     List<EdgeReprCandidateTransform> transform) {
        CubeEdges cerepr = new CubeEdges();
        if (transform.size() == 1) {
            EdgeReprCandidateTransform erct = transform.get(0);
            CubeEdges cesymm = erct.symmetric ? ce.symmetric() : ce;
            if (reverse) {
                if (erct.reversed) {
                    cerepr = CubeEdges.compose3revmid(ctransformed[erct.transformedIdx].ce, cesymm, erct.ceTrans);
                } else {
                    cerepr = CubeEdges.compose3(erct.ceTrans, cesymm, ctransformed[transformReverse(erct.transformedIdx)].ce);
                }
            } else {
                if (erct.reversed) {
                    cerepr = CubeEdges.compose3revmid(erct.ceTrans, cesymm, ctransformed[transformReverse(erct.transformedIdx)].ce);
                } else {
                    cerepr = CubeEdges.compose3(ctransformed[erct.transformedIdx].ce, cesymm, erct.ceTrans);
                }
            }
        } else {
            boolean isInit = false;
            for (EdgeReprCandidateTransform erct : transform) {
                CubeEdges cesymm = erct.symmetric ? ce.symmetric() : ce;
                CubeEdges cand;
                if (reverse) {
                    if (erct.reversed) {
                        cand = CubeEdges.compose3revmid(ctransformed[erct.transformedIdx].ce, cesymm, erct.ceTrans);
                    } else {
                        cand = CubeEdges.compose3(erct.ceTrans, cesymm, ctransformed[transformReverse(erct.transformedIdx)].ce);
                    }
                } else {
                    if (erct.reversed) {
                        cand = CubeEdges.compose3revmid(erct.ceTrans, cesymm, ctransformed[transformReverse(erct.transformedIdx)].ce);
                    } else {
                        cand = CubeEdges.compose3(ctransformed[erct.transformedIdx].ce, cesymm, erct.ceTrans);
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

    public Cube cubeRepresentative(Cube c) {
        List<EdgeReprCandidateTransform> transform = new ArrayList<>();
        CubecornersPerm ccpRepr = getReprPerm(c.ccp);
        CubecornerOrients ccoRepr = getReprOrients(c.ccp, c.cco, transform);
        CubeEdges ceRepr = getReprCubeedges(c.ce, transform);
        return new Cube(ccpRepr, ccoRepr, ceRepr);
    }
}
