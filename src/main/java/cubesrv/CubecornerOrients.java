package cubesrv;

public class CubecornerOrients implements Comparable<CubecornerOrients> {
    public int orients;

    public CubecornerOrients() {
        orients = 0;
    }

    public CubecornerOrients(int c0, int c1, int c2, int c3,
                             int c4, int c5, int c6, int c7) {
        orients = c0 | (c1 << 2) | (c2 << 4) | (c3 << 6)
                | (c4 << 8) | (c5 << 10) | (c6 << 12) | (c7 << 14);
    }

    public void setAt(int idx, int orient) {
        orients &= ~(3 << 2 * idx);
        orients |= (orient << 2 * idx);
    }

    public int getAt(int idx) {
        return (orients >>> 2 * idx) & 3;
    }

    public int get() {
        return orients;
    }

    public void set(int orts) {
        orients = orts;
    }

    public static CubecornerOrients compose(CubecornerOrients cco1,
                                            CubecornersPerm ccp2, CubecornerOrients cco2) {
        CubecornerOrients res = new CubecornerOrients();
        int[] MOD3 = {0, 1, 2, 0, 1};
        for (int i = 0; i < 8; i++) {
            int cc2Perm = (ccp2.get() >>> 3 * i) & 7;
            int cc2Orient = (cco2.orients >>> 2 * i) & 3;
            int cco1Orient = (cco1.orients >>> 2 * cc2Perm) & 3;
            int resOrient = MOD3[cco1Orient + cc2Orient];
            res.orients |= (resOrient << 2 * i);
        }
        return res;
    }

    public static CubecornerOrients compose3(CubecornerOrients cco1,
                                             CubecornersPerm ccp2, CubecornerOrients cco2,
                                             CubecornersPerm ccp3, CubecornerOrients cco3) {
        CubecornerOrients res = new CubecornerOrients();
        int[] MOD3 = {0, 1, 2, 0, 1, 2, 0};
        for (int i = 0; i < 8; i++) {
            int ccp3Perm = (ccp3.get() >>> 3 * i) & 7;
            int cco3orient = (cco3.orients >>> 2 * i) & 3;
            int midperm = (ccp2.get() >>> 3 * ccp3Perm) & 7;
            int cco2orient = (cco2.orients >>> 2 * ccp3Perm) & 3;
            int cco1orient = (cco1.orients >>> 2 * midperm) & 3;
            int resorient = MOD3[cco1orient + cco2orient + cco3orient];
            res.orients |= (resorient << 2 * i);
        }
        return res;
    }

    public static CubecornerOrients fromOrientIdx(int idxp) {
        int idx = idxp;
        CubecornerOrients res = new CubecornerOrients();
        int sum = 0;
        for (int i = 0; i < 7; i++) {
            int value = idx % 3;
            idx /= 3;
            res.setAt(6 - i, value);
            sum += value;
        }
        res.setAt(7, (15 - sum) % 3);
        return res;
    }

    public CubecornerOrients symmetric() {
        CubecornerOrients ccores = new CubecornerOrients();
        // set orient 2 -> 1, 1 -> 2, 0 unchanged
        int orie = ((orients & 0xaaaa) >>> 1) | ((orients & 0x5555) << 1);
        ccores.orients = ((orie >>> 8) | (orie << 8)) & 0xffff;
        return ccores;
    }

    public CubecornerOrients reverse(CubecornersPerm ccp) {
        int revOrients = ((orients & 0xaaaa) >>> 1) | ((orients & 0x5555) << 1);
        CubecornerOrients res = new CubecornerOrients();
        for (int i = 0; i < 8; i++)
            res.orients |= (((revOrients >>> 2 * i) & 3) << 2 * ccp.getAt(i));
        return res;
    }

    public CubecornerOrients transform(CubecornersPerm ccp, int transformDir) {
        Cube ctrans = CubeDefs.ctransformed[transformDir];
        Cube ctransRev = CubeDefs.ctransformed[CubeDefs.transformReverse(transformDir)];
        return compose3(ctrans.cco, ccp, this, ctransRev.ccp, ctransRev.cco);
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof CubecornerOrients)) return false;
        return orients == ((CubecornerOrients) o).orients;
    }

    @Override
    public int hashCode() {
        return orients;
    }

    @Override
    public int compareTo(CubecornerOrients o) {
        return Integer.compare(orients, o.orients);
    }

    public int getOrientIdx() {
        int res = 0;
        for (int i = 0; i < 7; i++)
            res = res * 3 + getAt(i);
        return res;
    }

    public boolean isBGspace() {
        return orients == 0;
    }

    public boolean isYWspace(CubecornersPerm ccp) {
        int[] orients0356 = {0, 1, 1, 0, 1, 0, 0, 1};
        int[] orients1247 = {2, 0, 0, 2, 0, 2, 2, 0};
        for (int i = 0; i < 8; i++) {
            int cp = ccp.getAt(i);
            if (cp == 0 || cp == 3 || cp == 5 || cp == 6) {
                if (getAt(i) != orients0356[i]) return false;
            } else if (cp == 1 || cp == 2 || cp == 4 || cp == 7) {
                if (getAt(i) != orients1247[i]) return false;
            }
        }
        return true;
    }

    public boolean isORspace(CubecornersPerm ccp) {
        int[] orients0356 = {0, 2, 2, 0, 2, 0, 0, 2};
        int[] orients1247 = {1, 0, 0, 1, 0, 1, 1, 0};
        for (int i = 0; i < 8; i++) {
            int cp = ccp.getAt(i);
            if (cp == 0 || cp == 3 || cp == 5 || cp == 6) {
                if (getAt(i) != orients0356[i]) return false;
            } else if (cp == 1 || cp == 2 || cp == 4 || cp == 7) {
                if (getAt(i) != orients1247[i]) return false;
            }
        }
        return true;
    }

    public CubecornerOrients representativeBG(CubecornersPerm ccp) {
        CubecornerOrients orepr = new CubecornerOrients();
        for (int i = 0; i < 8; i++)
            orepr.orients |= (((orients >>> 2 * i) & 3) << 2 * ccp.getAt(i));
        return orepr;
    }

    public CubecornerOrients representativeYW(CubecornersPerm ccp) {
        int[] oadd = {2, 1, 1, 2, 1, 2, 2, 1};
        CubecornerOrients orepr = new CubecornerOrients();
        CubecornersPerm ccpRev = ccp.reverse();
        for (int i = 0; i < 8; i++) {
            int toAdd = (oadd[i] == oadd[ccpRev.getAt(i)]) ? 0 : oadd[i];
            orepr.setAt(i, (getAt(ccpRev.getAt(i)) + toAdd) % 3);
        }
        return orepr;
    }

    public CubecornerOrients representativeOR(CubecornersPerm ccp) {
        int[] oadd = {1, 2, 2, 1, 2, 1, 1, 2};
        CubecornerOrients orepr = new CubecornerOrients();
        CubecornersPerm ccpRev = ccp.reverse();
        for (int i = 0; i < 8; i++) {
            int toAdd = (oadd[i] == oadd[ccpRev.getAt(i)]) ? 0 : oadd[i];
            orepr.setAt(i, (getAt(ccpRev.getAt(i)) + toAdd) % 3);
        }
        return orepr;
    }
}
