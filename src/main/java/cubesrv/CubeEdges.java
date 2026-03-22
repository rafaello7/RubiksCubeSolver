package cubesrv;

public class CubeEdges implements Comparable<CubeEdges> {
    public long edges;

    public CubeEdges() {
        edges = 0L;
    }

    public CubeEdges(long e) {
        edges = e;
    }

    public CubeEdges(int e0p, int e1p, int e2p, int e3p, int e4p, int e5p,
                     int e6p, int e7p, int e8p, int e9p, int e10p, int e11p,
                     int e0o, int e1o, int e2o, int e3o, int e4o, int e5o,
                     int e6o, int e7o, int e8o, int e9o, int e10o, int e11o) {
        edges = (long) e0p | ((long) e0o << 4)
                | ((long) e1p << 5) | ((long) e1o << 9)
                | ((long) e2p << 10) | ((long) e2o << 14)
                | ((long) e3p << 15) | ((long) e3o << 19)
                | ((long) e4p << 20) | ((long) e4o << 24)
                | ((long) e5p << 25) | ((long) e5o << 29)
                | ((long) e6p << 30) | ((long) e6o << 34)
                | ((long) e7p << 35) | ((long) e7o << 39)
                | ((long) e8p << 40) | ((long) e8o << 44)
                | ((long) e9p << 45) | ((long) e9o << 49)
                | ((long) e10p << 50) | ((long) e10o << 54)
                | ((long) e11p << 55) | ((long) e11o << 59);
    }

    public void setAt(int idx, int perm, int orient) {
        edges &= ~(0x1FL << 5 * idx);
        edges |= ((long) ((orient << 4) | perm)) << 5 * idx;
    }

    public void setPermAt(int idx, int perm) {
        edges &= ~(0xFL << 5 * idx);
        edges |= ((long) perm) << 5 * idx;
    }

    public void setOrientAt(int idx, int orient) {
        edges &= ~(0x1L << (5 * idx + 4));
        edges |= ((long) orient) << (5 * idx + 4);
    }

    public static CubeEdges compose(CubeEdges ce1, CubeEdges ce2) {
        CubeEdges res = new CubeEdges();
        for (int i = 0; i < 12; i++) {
            int edge2perm = (int) ((ce2.edges >>> 5 * i) & 0xfL);
            long edge1item = (ce1.edges >>> 5 * edge2perm) & 0x1fL;
            res.edges |= edge1item << 5 * i;
        }
        long edge2orients = ce2.edges & 0x842108421084210L;
        res.edges ^= edge2orients;
        return res;
    }

    public static CubeEdges compose3(CubeEdges ce1, CubeEdges ce2, CubeEdges ce3) {
        CubeEdges res = new CubeEdges();
        //res = compose(compose(ce1, ce2), ce3);
        for (int i = 0; i < 12; i++) {
            int edge3perm = (int) ((ce3.edges >>> 5 * i) & 0xfL);
            long edge2item = ce2.edges >>> 5 * edge3perm;
            int edge2perm = (int) (edge2item & 0xfL);
            long edge2orient = edge2item & 0x10L;
            long edge1item = (ce1.edges >>> 5 * edge2perm) & 0x1fL;
            long edgemitem = edge1item ^ edge2orient;
            res.edges |= edgemitem << 5 * i;
        }
        long edge3orients = ce3.edges & 0x842108421084210L;
        res.edges ^= edge3orients;
        return res;
    }

    // the middle cubeedges should be reversed before compose
    public static CubeEdges compose3revmid(CubeEdges ce1, CubeEdges ce2, CubeEdges ce3) {
        CubeEdges res = new CubeEdges();
        int[] ce2rperm = new int[12];
        long[] ce2rorient = new long[12];
        for (int i = 0; i < 12; i++) {
            long edge2item = ce2.edges >>> 5 * i;
            int edge2perm = (int) (edge2item & 0xfL);
            ce2rperm[edge2perm] = i;
            ce2rorient[edge2perm] = edge2item & 0x10L;
        }
        for (int i = 0; i < 12; i++) {
            int edge3perm = (int) ((ce3.edges >>> 5 * i) & 0xfL);
            long edge1item = (ce1.edges >>> 5 * ce2rperm[edge3perm]) & 0x1fL;
            long edgemitem = edge1item ^ ce2rorient[edge3perm];
            res.edges |= edgemitem << 5 * i;
        }
        long edge3orients = ce3.edges & 0x842108421084210L;
        res.edges ^= edge3orients;
        return res;
    }

    public static boolean isCeReprSolvable(CubeEdges ce) {
        boolean isSwapsOdd = false;
        int permScanned = 0;
        for (int i = 0; i < 12; i++) {
            if ((permScanned & (1 << i)) != 0) continue;
            permScanned |= (1 << i);
            int p = i;
            while (true) {
                p = ce.getPermAt(p);
                if (p == i) break;
                if ((permScanned & (1 << p)) != 0) {
                    System.out.println("edge perm " + p + " is twice");
                    return false;
                }
                permScanned |= (1 << p);
                isSwapsOdd = !isSwapsOdd;
            }
        }
        if (isSwapsOdd) {
            System.out.println("cubeedges unsolvable due to permutation parity");
            return false;
        }
        int sumOrient = 0;
        for (int i = 0; i < 12; i++) sumOrient += ce.getOrientAt(i);
        if (sumOrient % 2 != 0) {
            System.out.println("cubeedges unsolvable due to edge orientations");
            return false;
        }
        return true;
    }

    public static CubeEdges fromPermAndOrientIdx(int permIdxp, int orientIdxp) {
        int permIdx = permIdxp;
        int orientIdx = orientIdxp;
        long unused = 0xba9876543210L;
        CubeEdges ce = new CubeEdges();
        int orient11 = 0;
        for (int edgeIdx = 12; edgeIdx >= 1; edgeIdx--) {
            int p = (permIdx % edgeIdx) * 4;
            ce.setPermAt(12 - edgeIdx, (int) (unused >>> p) & 0xf);
            ce.setOrientAt(12 - edgeIdx, orientIdx & 1);
            long m = -1L << p;
            unused = (unused & ~m) | ((unused >>> 4) & m);
            permIdx /= edgeIdx;
            orient11 ^= orientIdx;
            orientIdx /= 2;
        }
        ce.setOrientAt(11, orient11 & 1);
        return ce;
    }

    public CubeEdges symmetric() {
        CubeEdges ceres = new CubeEdges();
        // map perm: 0 1 2 3 4 5 6 7 8 9 10 11 -> 8 9 10 11 4 5 6 7 0 1 2 3
        long edg = edges ^ ((~edges & 0x210842108421084L) << 1);
        ceres.edges = (edg >>> 40) | (edg & 0xfffff00000L) | ((edg & 0xfffffL) << 40);
        return ceres;
    }

    public CubeEdges reverse() {
        CubeEdges res = new CubeEdges();
        for (int i = 0; i < 12; i++) {
            long edgeItem = edges >>> 5 * i;
            res.edges |= ((edgeItem & 0x10L) | (long) i) << 5 * ((int) (edgeItem) & 0xf);
        }
        return res;
    }

    public CubeEdges transform(int idx) {
        CubeEdges cetrans = CubeDefs.ctransformed[idx].ce;
        CubeEdges res = new CubeEdges();
        for (int i = 0; i < 12; i++) {
            int ceItem = (int) (this.edges >>> 5 * i);
            int cePerm = ceItem & 0xf;
            int ceOrient = ceItem & 0x10;
            int ctransPerm = (int) (cetrans.edges >>> 5 * cePerm) & 0xf;
            int ctransPerm2 = (int) (cetrans.edges >>> 5 * i) & 0xf;
            res.edges |= ((long) (ctransPerm | ceOrient)) << 5 * ctransPerm2;
        }
        return res;
    }

    public int getPermAt(int idx) {
        return (int) (edges >>> 5 * idx) & 0xf;
    }

    public int getOrientAt(int idx) {
        return (int) (edges >>> (5 * idx + 4)) & 1;
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof CubeEdges)) return false;
        return edges == ((CubeEdges) o).edges;
    }

    @Override
    public int hashCode() {
        return Long.hashCode(edges);
    }

    @Override
    public int compareTo(CubeEdges o) {
        return Long.compare(edges, o.edges);
    }

    public int getPermIdx() {
        int res = 0;
        long indexes = 0L;
        long e = edges << 2;
        int shift = 60;
        for (int i = 1; i <= 12; i++) {
            shift -= 5;
            int p = (int) (e >>> shift) & 0x3c;
            res = i * res + (int) (indexes >>> p) & 0xf;
            indexes += 0x111111111111L << p;
        }
        return res;
    }

    public int getOrientIdx() {
        long orients = (edges & 0x42108421084210L) >>> 4;
        orients = orients | (orients >>> 4);
        orients = orients | (orients >>> 8);
        orients = orients & 0x70000f0000fL;
        long res = orients | (orients >>> 16) | (orients >>> 32);
        return (int) res & 0xffff;
    }

    public boolean isPermParityOdd() {
        boolean isSwapsOdd = false;
        int permScanned = 0;
        for (int i = 0; i < 12; i++) {
            if ((permScanned & (1 << i)) != 0) continue;
            permScanned |= (1 << i);
            int p = i;
            while (true) {
                p = getPermAt(p);
                if (p == i) break;
                permScanned |= (1 << p);
                isSwapsOdd = !isSwapsOdd;
            }
        }
        return isSwapsOdd;
    }

    public long get() {
        return edges;
    }

    public void set(long e) {
        edges = e;
    }

    public boolean isNil() {
        return edges == 0L;
    }

    public boolean isBGspace() {
        int[] orients03811 = {0, 1, 1, 0, 2, 2, 2, 2, 0, 1, 1, 0};
        int[] orients12910 = {1, 0, 0, 1, 2, 2, 2, 2, 1, 0, 0, 1};
        for (int i = 0; i < 12; i++) {
            if (i < 4 || i >= 8) {
                int ep = getPermAt(i);
                if (ep == 0 || ep == 3 || ep == 8 || ep == 11) {
                    if (getOrientAt(i) != orients03811[i]) return false;
                } else if (ep == 1 || ep == 2 || ep == 9 || ep == 10) {
                    if (getOrientAt(i) != orients12910[i]) return false;
                } else {
                    return false;
                }
            } else {
                if (getOrientAt(i) != 0) return false;
            }
        }
        return true;
    }

    public boolean isYWspace() {
        int[] orients03811 = {0, 2, 2, 0, 1, 1, 1, 1, 0, 2, 2, 0};
        int[] orients4567 = {1, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 1};
        for (int i = 0; i < 12; i++) {
            if (orients03811[i] == 2) {
                if (getOrientAt(i) != 0) return false;
            } else {
                int ep = getPermAt(i);
                if (ep == 0 || ep == 3 || ep == 8 || ep == 11) {
                    if (getOrientAt(i) != orients03811[i]) return false;
                } else if (ep >= 4 && ep <= 7) {
                    if (getOrientAt(i) != orients4567[i]) return false;
                } else {
                    return false;
                }
            }
        }
        return true;
    }

    public boolean isORspace() {
        int[] orients12910 = {2, 0, 0, 2, 1, 1, 1, 1, 2, 0, 0, 2};
        int[] orients4567 = {2, 1, 1, 2, 0, 0, 0, 0, 2, 1, 1, 2};
        for (int i = 0; i < 12; i++) {
            if (orients12910[i] == 2) {
                if (getOrientAt(i) != 0) return false;
            } else {
                int ep = getPermAt(i);
                if (ep == 1 || ep == 2 || ep == 9 || ep == 10) {
                    if (getOrientAt(i) != orients12910[i]) return false;
                } else if (ep >= 4 && ep <= 7) {
                    if (getOrientAt(i) != orients4567[i]) return false;
                } else {
                    return false;
                }
            }
        }
        return true;
    }

    public CubeEdges representativeBG() {
        CubeEdges cerepr = new CubeEdges();
        long orientsIn = 0x6060L;
        CubeEdges ceRev = reverse();
        int destIn = 0;
        int destOut = 4;
        int permOutSum = 0;
        for (int i = 0; i < 12; i++) {
            int ipos = ceRev.getPermAt(i);
            long item = (edges >>> 5 * ipos) & 0x1fL;
            if (ipos < 4 || ipos >= 8) {
                item ^= (((orientsIn >>> ipos) ^ (orientsIn >>> destIn)) & 0x10L);
                cerepr.edges |= item << 5 * destIn;
                ++destIn;
                destIn += destIn & 0x4;
            } else {
                permOutSum += i;
                if (destOut == 7 && (permOutSum & 1) != 0) {
                    long item6 = (cerepr.edges >>> 30) & 0x1fL;
                    cerepr.edges &= ~(0x1fL << 30);
                    cerepr.edges |= item6 << 35;
                    --destOut;
                }
                cerepr.edges |= item << 5 * destOut;
                ++destOut;
            }
        }
        return cerepr;
    }

    public CubeEdges representativeYW() {
        CubeEdges cerepr = new CubeEdges();
        int[] orientsIn = {1, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 1};
        CubeEdges ceRev = reverse();
        int destIn = 0;
        int destOut = 0;
        for (int i = 0; i < 12; i++) {
            int ipos = ceRev.getPermAt(i);
            if (orientsIn[ipos] != 2) {
                while (orientsIn[destIn] == 2) ++destIn;
                cerepr.setPermAt(destIn, i);
                cerepr.setOrientAt(destIn, (orientsIn[ipos] == orientsIn[destIn]) ? getOrientAt(ipos) : 1 - getOrientAt(ipos));
                ++destIn;
            } else {
                while (orientsIn[destOut] != 2) ++destOut;
                cerepr.setPermAt(destOut, i);
                cerepr.setOrientAt(destOut, getOrientAt(ipos));
                ++destOut;
            }
        }
        boolean isSwapsOdd = false;
        int permScanned = 0;
        for (int i = 0; i < 12; i++) {
            if ((permScanned & (1 << i)) != 0) continue;
            permScanned |= (1 << i);
            int p = i;
            while (true) {
                p = cerepr.getPermAt(p);
                if (p == i) break;
                permScanned |= (1 << p);
                isSwapsOdd = !isSwapsOdd;
            }
        }
        if (isSwapsOdd) {
            int p = cerepr.getPermAt(10);
            int o = cerepr.getOrientAt(10);
            cerepr.setPermAt(10, cerepr.getPermAt(9));
            cerepr.setOrientAt(10, cerepr.getOrientAt(9));
            cerepr.setPermAt(9, p);
            cerepr.setOrientAt(9, o);
        }
        if (!isCeReprSolvable(this)) {
            System.out.println("fatal: YW cube representative is unsolvable");
            System.exit(1);
        }
        return cerepr;
    }

    public CubeEdges representativeOR() {
        CubeEdges cerepr = new CubeEdges();
        int[] orientsIn = {2, 0, 0, 2, 1, 1, 1, 1, 2, 0, 0, 2};
        CubeEdges ceRev = reverse();
        int destIn = 0;
        int destOut = 0;
        for (int i = 0; i < 12; i++) {
            int ipos = ceRev.getPermAt(i);
            if (orientsIn[ipos] != 2) {
                while (orientsIn[destIn] == 2) ++destIn;
                cerepr.setPermAt(destIn, i);
                cerepr.setOrientAt(destIn, (orientsIn[ipos] == orientsIn[destIn]) ? getOrientAt(ipos) : 1 - getOrientAt(ipos));
                ++destIn;
            } else {
                while (orientsIn[destOut] != 2) ++destOut;
                cerepr.setPermAt(destOut, i);
                cerepr.setOrientAt(destOut, getOrientAt(ipos));
                ++destOut;
            }
        }
        boolean isSwapsOdd = false;
        int permScanned = 0;
        for (int i = 0; i < 12; i++) {
            if ((permScanned & (1 << i)) != 0) continue;
            permScanned |= (1 << i);
            int p = i;
            while (true) {
                p = cerepr.getPermAt(p);
                if (p == i) break;
                permScanned |= (1 << p);
                isSwapsOdd = !isSwapsOdd;
            }
        }
        if (isSwapsOdd) {
            int p = cerepr.getPermAt(8);
            int o = cerepr.getOrientAt(8);
            cerepr.setPermAt(8, cerepr.getPermAt(11));
            cerepr.setOrientAt(8, cerepr.getOrientAt(11));
            cerepr.setPermAt(11, p);
            cerepr.setOrientAt(11, o);
        }
        if (!isCeReprSolvable(this)) {
            System.out.println("fatal: OR cube representative is unsolvable");
            System.exit(1);
        }
        return cerepr;
    }
}
