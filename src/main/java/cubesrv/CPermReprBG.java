package cubesrv;

import static cubesrv.CubeDefs.*;
import java.util.ArrayList;
import java.util.List;

public class CPermReprBG {

    public static final int RCOUNTBG = 10;
    public static final int TCOUNTBG = 8;

    public static final rotate_dir[] BGSpaceRotations = {
        rotate_dir.ORANGE180, rotate_dir.RED180, rotate_dir.YELLOW180, rotate_dir.WHITE180,
        rotate_dir.GREENCW, rotate_dir.GREEN180, rotate_dir.GREENCCW,
        rotate_dir.BLUECW, rotate_dir.BLUE180, rotate_dir.BLUECCW
    };

    public static final transform_dir[] BGSpaceTransforms = {
        transform_dir.TD_0, transform_dir.TD_BG_CW, transform_dir.TD_BG_180, transform_dir.TD_BG_CCW,
        transform_dir.TD_YW_180, transform_dir.TD_OR_180, transform_dir.TD_E4_7, transform_dir.TD_E5_6
    };

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

    public static class BGCubecornerReprPerms {
        private final boolean m_useReverse;
        private final List<cubecorners_perm> m_reprPerms = new ArrayList<>();
        private final CubecornerPermToRepr[] m_permToRepr;

        public BGCubecornerReprPerms(boolean useReverse) {
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
                        for (int tdidx = 0; tdidx < TCOUNTBG; tdidx++) {
                            int td = BGSpaceTransforms[tdidx].ordinal();
                            cubecorners_perm cand = permchk.transform(td);
                            if (tdidx == 0 && rev == 0 && sym == 0 || cand.compareTo(permRepr) < 0) {
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
            System.out.println("bg repr size=" + m_reprPerms.size());
        }

        public boolean isUseReverse() { return m_useReverse; }
        public int reprPermCount() { return m_reprPerms.size(); }

        public cubecorners_perm getReprPerm(cubecorners_perm ccp) {
            return m_reprPerms.get(m_permToRepr[ccp.getPermIdx()].reprIdx);
        }

        public int getReprPermIdx(cubecorners_perm ccp) {
            return m_permToRepr[ccp.getPermIdx()].reprIdx;
        }

        public cubecorners_perm getPermForIdx(int reprPermIdx) {
            return m_reprPerms.get(reprPermIdx);
        }

        public boolean isSingleTransform(cubecorners_perm ccp) {
            return m_permToRepr[ccp.getPermIdx()].transform.size() == 1;
        }

        public cubeedges getReprCubeedges(cubecorners_perm ccp, cubeedges ce) {
            CubecornerPermToRepr permToRepr = m_permToRepr[ccp.getPermIdx()];
            cubeedges cesymm    = ce.symmetric();
            cubeedges cerev     = ce.reverse();
            cubeedges cerevsymm = cerev.symmetric();
            cubeedges erepr     = new cubeedges();
            boolean isInit = false;
            for (ReprCandidateTransform rct : permToRepr.transform) {
                cubeedges ecand;
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

        public cube cubeRepresentative(cube c) {
            cubecorners_perm ccpRepr = getReprPerm(c.ccp);
            cubeedges ceRepr = getReprCubeedges(c.ccp, c.ce);
            return new cube(ccpRepr, csolved.cco, ceRepr);
        }

        public cubeedges getCubeedgesForRepresentative(cubecorners_perm ccpSearch, cubeedges ceSearchRepr) {
            CubecornerPermToRepr permToRepr = m_permToRepr[ccpSearch.getPermIdx()];
            ReprCandidateTransform rct = permToRepr.transform.get(0);
            int transformIdxRev = transformReverse(rct.transformIdx);
            cubeedges ceSearchRevSymm = ceSearchRepr.transform(transformIdxRev);
            cubeedges ceSearchRev = rct.symmetric ? ceSearchRevSymm.symmetric() : ceSearchRevSymm;
            return rct.reversed ? ceSearchRev.reverse() : ceSearchRev;
        }
    }
}

