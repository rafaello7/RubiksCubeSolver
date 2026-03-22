package cubesrv;

public class CubecornersPerm implements Comparable<CubecornersPerm> {
    public int perm;

    public CubecornersPerm() {
        perm = 0;
    }

    public CubecornersPerm(int c0, int c1, int c2, int c3,
                           int c4, int c5, int c6, int c7) {
        perm = c0 | (c1 << 3) | (c2 << 6) | (c3 << 9)
                | (c4 << 12) | (c5 << 15) | (c6 << 18) | (c7 << 21);
    }

    public void setAt(int idx, int p) {
        perm &= ~(7 << 3 * idx);
        perm |= (p << 3 * idx);
    }

    public int getAt(int idx) {
        return (perm >>> 3 * idx) & 7;
    }

    public int get() {
        return perm;
    }

    public void set(int p) {
        perm = p;
    }

    public static CubecornersPerm compose(CubecornersPerm ccp1, CubecornersPerm ccp2) {
        CubecornersPerm res = new CubecornersPerm();
        for (int i = 0; i < 8; i++)
            res.perm |= (ccp1.getAt(ccp2.getAt(i)) << 3 * i);
        return res;
    }

    public static CubecornersPerm compose3(CubecornersPerm ccp1, CubecornersPerm ccp2, CubecornersPerm ccp3) {
        CubecornersPerm res = new CubecornersPerm();
        for (int i = 0; i < 8; i++) {
            int corner3perm = (ccp3.perm >>> 3 * i) & 0x7;
            int corner2perm = (ccp2.perm >>> 3 * corner3perm) & 0x7;
            int corner1perm = (ccp1.perm >>> 3 * corner2perm) & 0x7;
            res.perm |= (corner1perm << 3 * i);
        }
        return res;
    }

    public static CubecornersPerm fromPermIdx(int idxp) {
        int idx = idxp;
        int unused = 0x76543210;
        CubecornersPerm ccp = new CubecornersPerm();
        for (int cornerIdx = 8; cornerIdx >= 1; cornerIdx--) {
            int p = (idx % cornerIdx) * 4;
            ccp.setAt(8 - cornerIdx, (unused >>> p) & 0xf);
            int m = -1 << p;
            unused = (unused & ~m) | ((unused >>> 4) & m);
            idx /= cornerIdx;
        }
        return ccp;
    }

    public CubecornersPerm symmetric() {
        int permRes = perm ^ 0x924924;
        CubecornersPerm ccpres = new CubecornersPerm();
        ccpres.perm = ((permRes >>> 12) | (permRes << 12)) & 0xffffff;
        return ccpres;
    }

    public CubecornersPerm reverse() {
        CubecornersPerm res = new CubecornersPerm();
        for (int i = 0; i < 8; i++)
            res.perm |= (i << 3 * getAt(i));
        return res;
    }

    public CubecornersPerm transform(int transformDir) {
        CubecornersPerm cctr = CubeDefs.ctransformed[transformDir].ccp;
        CubecornersPerm ccrtr = CubeDefs.ctransformed[CubeDefs.transformReverse(transformDir)].ccp;
        return compose3(cctr, this, ccrtr);
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof CubecornersPerm)) return false;
        return perm == ((CubecornersPerm) o).perm;
    }

    @Override
    public int hashCode() {
        return perm;
    }

    @Override
    public int compareTo(CubecornersPerm o) {
        return Integer.compare(perm, o.perm);
    }

    public int getPermIdx() {
        int res = 0;
        int indexes = 0;
        for (int i = 7; i >= 0; i--) {
            int p = getAt(i) * 4;
            res = (8 - i) * res + ((indexes >>> p) & 0xf);
            indexes += 0x11111111 << p;
        }
        return res;
    }

    public boolean isPermParityOdd() {
        boolean isSwapsOdd = false;
        int permScanned = 0;
        for (int i = 0; i < 8; i++) {
            if ((permScanned & (1 << i)) != 0) continue;
            permScanned |= (1 << i);
            int p = i;
            while (true) {
                p = getAt(p);
                if (p == i) break;
                permScanned |= (1 << p);
                isSwapsOdd = !isSwapsOdd;
            }
        }
        return isSwapsOdd;
    }
}
