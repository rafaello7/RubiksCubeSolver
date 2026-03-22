package cubesrv;

import static cubesrv.CubeDefs.csolved;
import static cubesrv.CubeDefs.transformReverse;
import java.util.ArrayList;
import java.util.List;

public class BGCubecornerReprPerms {
    private static class BGReprCandidateTransform {
        final boolean reversed;
        final boolean symmetric;
        final int transformIdx;

        BGReprCandidateTransform(boolean reversed, boolean symmetric, int transformIdx) {
            this.reversed = reversed;
            this.symmetric = symmetric;
            this.transformIdx = transformIdx;
        }
    }

    private static class CubecornerPermToRepr {
        int reprIdx = -1;
        final List<BGReprCandidateTransform> transform = new ArrayList<>();
    }

    private final boolean m_useReverse;
    private final List<CubecornersPerm> m_reprPerms = new ArrayList<>();
    private final CubecornerPermToRepr[] m_permToRepr;

    public BGCubecornerReprPerms(boolean useReverse) {
        this.m_useReverse = useReverse;
        m_permToRepr = new CubecornerPermToRepr[40320];
        for (int i = 0; i < 40320; i++) m_permToRepr[i] = new CubecornerPermToRepr();

        for (int pidx = 0; pidx < 40320; pidx++) {
            CubecornersPerm perm = CubecornersPerm.fromPermIdx(pidx);
            CubecornersPerm permRepr = new CubecornersPerm();
            List<BGReprCandidateTransform> transform = new ArrayList<>();
            for (int rev = 0; rev < (m_useReverse ? 2 : 1); rev++) {
                CubecornersPerm permr = (rev != 0) ? perm.reverse() : perm;
                for (int sym = 0; sym <= 1; sym++) {
                    CubecornersPerm permchk = (sym != 0) ? permr.symmetric() : permr;
                    for (int tdidx = 0; tdidx < CPermReprBG.TCOUNTBG; tdidx++) {
                        int td = CPermReprBG.BGSpaceTransforms[tdidx].ordinal();
                        CubecornersPerm cand = permchk.transform(td);
                        if (tdidx == 0 && rev == 0 && sym == 0 || cand.compareTo(permRepr) < 0) {
                            permRepr = cand;
                            transform.clear();
                            transform.add(new BGReprCandidateTransform(rev != 0, sym != 0, td));
                        } else if (cand.equals(permRepr)) {
                            transform.add(new BGReprCandidateTransform(rev != 0, sym != 0, td));
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
        System.out.println("bg repr size=" + m_reprPerms.size());
    }

    public boolean isUseReverse() {
        return m_useReverse;
    }

    public int reprPermCount() {
        return m_reprPerms.size();
    }

    public CubecornersPerm getReprPerm(CubecornersPerm ccp) {
        return m_reprPerms.get(m_permToRepr[ccp.getPermIdx()].reprIdx);
    }

    public int getReprPermIdx(CubecornersPerm ccp) {
        return m_permToRepr[ccp.getPermIdx()].reprIdx;
    }

    public CubecornersPerm getPermForIdx(int reprPermIdx) {
        return m_reprPerms.get(reprPermIdx);
    }

    public boolean isSingleTransform(CubecornersPerm ccp) {
        return m_permToRepr[ccp.getPermIdx()].transform.size() == 1;
    }

    public CubeEdges getReprCubeedges(CubecornersPerm ccp, CubeEdges ce) {
        CubecornerPermToRepr permToRepr = m_permToRepr[ccp.getPermIdx()];
        CubeEdges cesymm = ce.symmetric();
        CubeEdges cerev = ce.reverse();
        CubeEdges cerevsymm = cerev.symmetric();
        CubeEdges erepr = new CubeEdges();
        boolean isInit = false;
        for (BGReprCandidateTransform rct : permToRepr.transform) {
            CubeEdges ecand;
            if (rct.reversed) {
                ecand = rct.symmetric ? cerevsymm.transform(rct.transformIdx) : cerev.transform(rct.transformIdx);
            } else {
                ecand = rct.symmetric ? cesymm.transform(rct.transformIdx) : ce.transform(rct.transformIdx);
            }
            if (!isInit || ecand.compareTo(erepr) < 0) {
                erepr = ecand;
                isInit = true;
            }
        }
        return erepr;
    }

    public Cube cubeRepresentative(Cube c) {
        CubecornersPerm ccpRepr = getReprPerm(c.ccp);
        CubeEdges ceRepr = getReprCubeedges(c.ccp, c.ce);
        return new Cube(ccpRepr, csolved.cco, ceRepr);
    }

    public CubeEdges getCubeedgesForRepresentative(CubecornersPerm ccpSearch, CubeEdges ceSearchRepr) {
        CubecornerPermToRepr permToRepr = m_permToRepr[ccpSearch.getPermIdx()];
        BGReprCandidateTransform rct = permToRepr.transform.get(0);
        int transformIdxRev = transformReverse(rct.transformIdx);
        CubeEdges ceSearchRevSymm = ceSearchRepr.transform(transformIdxRev);
        CubeEdges ceSearchRev = rct.symmetric ? ceSearchRevSymm.symmetric() : ceSearchRevSymm;
        return rct.reversed ? ceSearchRev.reverse() : ceSearchRev;
    }
}
