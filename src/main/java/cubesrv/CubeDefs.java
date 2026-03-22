package cubesrv;
// The cube corner and edge numbers
//           ___________
//           | YELLOW  |
//           | 4  8  5 |
//           | 4     5 |
//           | 0  0  1 |
//  _________|_________|_________ _________
// |  ORANGE |   BLUE  |   RED   |  GREEN  |
// | 4  4  0 | 0  0  1 | 1  5  5 | 5  8  4 |
// | 9     1 | 1     2 | 2    10 | 10    9 |
// | 6  6  2 | 2  3  3 | 3  7  7 | 7 11  6 |
// |_________|_________|_________|_________|
//           |  WHITE  |
//           | 2  3  3 |
//           | 6     7 |
//           | 6 11  7 |
//           |_________|
//
// The cube corner and edge orientation referential squares
// if a referential square of a corner/edge being moved goes into a
// referential square, the corner/edge orientation is not changed
//           ___________
//           | YELLOW  |
//           |         |
//           | e     e |
//           |         |
//  _________|_________|_________ _________
// |  ORANGE |   BLUE  |   RED   |  GREEN  |
// |         | c  e  c |         | c  e  c |
// | e     e |         | e     e |         |
// |         | c  e  c |         | c  e  c |
// |_________|_________|_________|_________|
//           |  WHITE  |
//           |         |
//           | e     e |
//           |         |
//           |_________|

public class CubeDefs {

    public enum rotate_dir {
        ORANGECW, ORANGE180, ORANGECCW, REDCW, RED180, REDCCW,
        YELLOWCW, YELLOW180, YELLOWCCW, WHITECW, WHITE180, WHITECCW,
        GREENCW, GREEN180, GREENCCW, BLUECW, BLUE180, BLUECCW,
        RCOUNT
    }

    public enum cubecolor {
        CYELLOW, CORANGE, CBLUE, CRED, CGREEN, CWHITE, CCOUNT
    }

// The cube transformation (colors switch) through the cube rotation
    public enum transform_dir {
        TD_0,           // no rotation
        TD_C0_7_CW,     // rotation along axis from corner 0 to 7, 120 degree clockwise
        TD_C0_7_CCW,    // rotation along axis from corner 0 to 7, counterclockwise
        TD_C1_6_CW,
        TD_C1_6_CCW,
        TD_C2_5_CW,
        TD_C2_5_CCW,
        TD_C3_4_CW,
        TD_C3_4_CCW,
        TD_BG_CW,       // rotation along axis through the wall middle, from blue to green wall, clockwise
        TD_BG_180,
        TD_BG_CCW,
        TD_YW_CW,
        TD_YW_180,
        TD_YW_CCW,
        TD_OR_CW,
        TD_OR_180,
        TD_OR_CCW,
        TD_E0_11,       // rotation along axis from edge 0 to 11, 180 degree
        TD_E1_10,
        TD_E2_9,
        TD_E3_8,
        TD_E4_7,
        TD_E5_6,
        TCOUNT
    }

    public static final int[] R120 = {1, 2, 0};
    public static final int[] R240 = {2, 0, 1};

    public static final cubecolor[][] cubeCornerColors = {
        {cubecolor.CBLUE,   cubecolor.CORANGE, cubecolor.CYELLOW},
        {cubecolor.CBLUE,   cubecolor.CYELLOW, cubecolor.CRED},
        {cubecolor.CBLUE,   cubecolor.CWHITE,  cubecolor.CORANGE},
        {cubecolor.CBLUE,   cubecolor.CRED,    cubecolor.CWHITE},
        {cubecolor.CGREEN,  cubecolor.CYELLOW, cubecolor.CORANGE},
        {cubecolor.CGREEN,  cubecolor.CRED,    cubecolor.CYELLOW},
        {cubecolor.CGREEN,  cubecolor.CORANGE, cubecolor.CWHITE},
        {cubecolor.CGREEN,  cubecolor.CWHITE,  cubecolor.CRED}
    };

    public static final cubecolor[][] cubeEdgeColors = {
        {cubecolor.CBLUE,   cubecolor.CYELLOW},
        {cubecolor.CORANGE, cubecolor.CBLUE},
        {cubecolor.CRED,    cubecolor.CBLUE},
        {cubecolor.CBLUE,   cubecolor.CWHITE},
        {cubecolor.CYELLOW, cubecolor.CORANGE},
        {cubecolor.CYELLOW, cubecolor.CRED},
        {cubecolor.CWHITE,  cubecolor.CORANGE},
        {cubecolor.CWHITE,  cubecolor.CRED},
        {cubecolor.CGREEN,  cubecolor.CYELLOW},
        {cubecolor.CORANGE, cubecolor.CGREEN},
        {cubecolor.CRED,    cubecolor.CGREEN},
        {cubecolor.CGREEN,  cubecolor.CWHITE}
    };

    // -------------------------------------------------------------------------
    // cubecorners_perm
    // -------------------------------------------------------------------------
    public static class cubecorners_perm implements Comparable<cubecorners_perm> {
        public int perm;

        public cubecorners_perm() { perm = 0; }

        public cubecorners_perm(int c0, int c1, int c2, int c3,
                                int c4, int c5, int c6, int c7) {
            perm = c0 | (c1 << 3) | (c2 << 6) | (c3 << 9)
                 | (c4 << 12) | (c5 << 15) | (c6 << 18) | (c7 << 21);
        }

        public void setAt(int idx, int p) {
            perm &= ~(7 << 3*idx);
            perm |= (p << 3*idx);
        }

        public int getAt(int idx) { return (perm >>> 3*idx) & 7; }
        public int get() { return perm; }
        public void set(int p) { perm = p; }

        public static cubecorners_perm compose(cubecorners_perm ccp1, cubecorners_perm ccp2) {
            cubecorners_perm res = new cubecorners_perm();
            for (int i = 0; i < 8; i++)
                res.perm |= (ccp1.getAt(ccp2.getAt(i)) << 3*i);
            return res;
        }

        public static cubecorners_perm compose3(cubecorners_perm ccp1, cubecorners_perm ccp2, cubecorners_perm ccp3) {
            cubecorners_perm res = new cubecorners_perm();
            for (int i = 0; i < 8; i++) {
                int corner3perm = (ccp3.perm >>> 3*i) & 0x7;
                int corner2perm = (ccp2.perm >>> 3*corner3perm) & 0x7;
                int corner1perm = (ccp1.perm >>> 3*corner2perm) & 0x7;
                res.perm |= (corner1perm << 3*i);
            }
            return res;
        }

        public static cubecorners_perm fromPermIdx(int idxp) {
            int idx = idxp;
            int unused = 0x76543210;
            cubecorners_perm ccp = new cubecorners_perm();
            for (int cornerIdx = 8; cornerIdx >= 1; cornerIdx--) {
                int p = (idx % cornerIdx) * 4;
                ccp.setAt(8 - cornerIdx, (unused >>> p) & 0xf);
                int m = -1 << p;
                unused = (unused & ~m) | ((unused >>> 4) & m);
                idx /= cornerIdx;
            }
            return ccp;
        }

        public cubecorners_perm symmetric() {
            int permRes = perm ^ 0x924924;
            cubecorners_perm ccpres = new cubecorners_perm();
            ccpres.perm = ((permRes >>> 12) | (permRes << 12)) & 0xffffff;
            return ccpres;
        }

        public cubecorners_perm reverse() {
            cubecorners_perm res = new cubecorners_perm();
            for (int i = 0; i < 8; i++)
                res.perm |= (i << 3*getAt(i));
            return res;
        }

        public cubecorners_perm transform(int transformDir) {
            cubecorners_perm cctr  = ctransformed[transformDir].ccp;
            cubecorners_perm ccrtr = ctransformed[transformReverse(transformDir)].ccp;
            return compose3(cctr, this, ccrtr);
        }

        @Override public boolean equals(Object o) {
            if (!(o instanceof cubecorners_perm)) return false;
            return perm == ((cubecorners_perm)o).perm;
        }
        @Override public int hashCode() { return perm; }
        @Override public int compareTo(cubecorners_perm o) { return Integer.compare(perm, o.perm); }

        public int getPermIdx() {
            int res = 0;
            int indexes = 0;
            for (int i = 7; i >= 0; i--) {
                int p = getAt(i) * 4;
                res = (8-i) * res + ((indexes >>> p) & 0xf);
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

    // -------------------------------------------------------------------------
    // cubecorner_orients
    // -------------------------------------------------------------------------
    public static class cubecorner_orients implements Comparable<cubecorner_orients> {
        public int orients;

        public cubecorner_orients() { orients = 0; }

        public cubecorner_orients(int c0, int c1, int c2, int c3,
                                   int c4, int c5, int c6, int c7) {
            orients = c0 | (c1 << 2) | (c2 << 4) | (c3 << 6)
                    | (c4 << 8) | (c5 << 10) | (c6 << 12) | (c7 << 14);
        }

        public void setAt(int idx, int orient) {
            orients &= ~(3 << 2*idx);
            orients |= (orient << 2*idx);
        }

        public int getAt(int idx) { return (orients >>> 2*idx) & 3; }
        public int get() { return orients; }
        public void set(int orts) { orients = orts; }

        public static cubecorner_orients compose(cubecorner_orients cco1,
                cubecorners_perm ccp2, cubecorner_orients cco2) {
            cubecorner_orients res = new cubecorner_orients();
            int[] MOD3 = {0, 1, 2, 0, 1};
            for (int i = 0; i < 8; i++) {
                int cc2Perm   = (ccp2.get() >>> 3*i) & 7;
                int cc2Orient = (cco2.orients >>> 2*i) & 3;
                int cco1Orient = (cco1.orients >>> 2*cc2Perm) & 3;
                int resOrient = MOD3[cco1Orient + cc2Orient];
                res.orients |= (resOrient << 2*i);
            }
            return res;
        }

        public static cubecorner_orients compose3(cubecorner_orients cco1,
                cubecorners_perm ccp2, cubecorner_orients cco2,
                cubecorners_perm ccp3, cubecorner_orients cco3) {
            cubecorner_orients res = new cubecorner_orients();
            int[] MOD3 = {0, 1, 2, 0, 1, 2, 0};
            for (int i = 0; i < 8; i++) {
                int ccp3Perm   = (ccp3.get() >>> 3*i) & 7;
                int cco3orient = (cco3.orients >>> 2*i) & 3;
                int midperm    = (ccp2.get() >>> 3*ccp3Perm) & 7;
                int cco2orient = (cco2.orients >>> 2*ccp3Perm) & 3;
                int cco1orient = (cco1.orients >>> 2*midperm) & 3;
                int resorient  = MOD3[cco1orient + cco2orient + cco3orient];
                res.orients |= (resorient << 2*i);
            }
            return res;
        }

        public static cubecorner_orients fromOrientIdx(int idxp) {
            int idx = idxp;
            cubecorner_orients res = new cubecorner_orients();
            int sum = 0;
            for (int i = 0; i < 7; i++) {
                int value = idx % 3;
                idx /= 3;
                res.setAt(6-i, value);
                sum += value;
            }
            res.setAt(7, (15 - sum) % 3);
            return res;
        }

        public cubecorner_orients symmetric() {
            cubecorner_orients ccores = new cubecorner_orients();
            // set orient 2 -> 1, 1 -> 2, 0 unchanged
            int orie = ((orients & 0xaaaa) >>> 1) | ((orients & 0x5555) << 1);
            ccores.orients = ((orie >>> 8) | (orie << 8)) & 0xffff;
            return ccores;
        }

        public cubecorner_orients reverse(cubecorners_perm ccp) {
            int revOrients = ((orients & 0xaaaa) >>> 1) | ((orients & 0x5555) << 1);
            cubecorner_orients res = new cubecorner_orients();
            for (int i = 0; i < 8; i++)
                res.orients |= (((revOrients >>> 2*i) & 3) << 2*ccp.getAt(i));
            return res;
        }

        public cubecorner_orients transform(cubecorners_perm ccp, int transformDir) {
            cube ctrans    = ctransformed[transformDir];
            cube ctransRev = ctransformed[transformReverse(transformDir)];
            return compose3(ctrans.cco, ccp, this, ctransRev.ccp, ctransRev.cco);
        }

        @Override public boolean equals(Object o) {
            if (!(o instanceof cubecorner_orients)) return false;
            return orients == ((cubecorner_orients)o).orients;
        }
        @Override public int hashCode() { return orients; }
        @Override public int compareTo(cubecorner_orients o) { return Integer.compare(orients, o.orients); }

        public int getOrientIdx() {
            int res = 0;
            for (int i = 0; i < 7; i++)
                res = res * 3 + getAt(i);
            return res;
        }

        public boolean isBGspace() { return orients == 0; }

        public boolean isYWspace(cubecorners_perm ccp) {
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

        public boolean isORspace(cubecorners_perm ccp) {
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

        public cubecorner_orients representativeBG(cubecorners_perm ccp) {
            cubecorner_orients orepr = new cubecorner_orients();
            for (int i = 0; i < 8; i++)
                orepr.orients |= (((orients >>> 2*i) & 3) << 2*ccp.getAt(i));
            return orepr;
        }

        public cubecorner_orients representativeYW(cubecorners_perm ccp) {
            int[] oadd = {2, 1, 1, 2, 1, 2, 2, 1};
            cubecorner_orients orepr = new cubecorner_orients();
            cubecorners_perm ccpRev = ccp.reverse();
            for (int i = 0; i < 8; i++) {
                int toAdd = (oadd[i] == oadd[ccpRev.getAt(i)]) ? 0 : oadd[i];
                orepr.setAt(i, (getAt(ccpRev.getAt(i)) + toAdd) % 3);
            }
            return orepr;
        }

        public cubecorner_orients representativeOR(cubecorners_perm ccp) {
            int[] oadd = {1, 2, 2, 1, 2, 1, 1, 2};
            cubecorner_orients orepr = new cubecorner_orients();
            cubecorners_perm ccpRev = ccp.reverse();
            for (int i = 0; i < 8; i++) {
                int toAdd = (oadd[i] == oadd[ccpRev.getAt(i)]) ? 0 : oadd[i];
                orepr.setAt(i, (getAt(ccpRev.getAt(i)) + toAdd) % 3);
            }
            return orepr;
        }
    }

    // -------------------------------------------------------------------------
    // cubeedges
    // -------------------------------------------------------------------------
    public static class cubeedges implements Comparable<cubeedges> {
        public long edges;

        public cubeedges() { edges = 0L; }
        public cubeedges(long e) { edges = e; }

        public cubeedges(int e0p, int e1p, int e2p, int e3p, int e4p, int e5p,
                         int e6p, int e7p, int e8p, int e9p, int e10p, int e11p,
                         int e0o, int e1o, int e2o, int e3o, int e4o, int e5o,
                         int e6o, int e7o, int e8o, int e9o, int e10o, int e11o) {
            edges = (long)e0p  | ((long)e0o  << 4)
                 | ((long)e1p  << 5)  | ((long)e1o  << 9)
                 | ((long)e2p  << 10) | ((long)e2o  << 14)
                 | ((long)e3p  << 15) | ((long)e3o  << 19)
                 | ((long)e4p  << 20) | ((long)e4o  << 24)
                 | ((long)e5p  << 25) | ((long)e5o  << 29)
                 | ((long)e6p  << 30) | ((long)e6o  << 34)
                 | ((long)e7p  << 35) | ((long)e7o  << 39)
                 | ((long)e8p  << 40) | ((long)e8o  << 44)
                 | ((long)e9p  << 45) | ((long)e9o  << 49)
                 | ((long)e10p << 50) | ((long)e10o << 54)
                 | ((long)e11p << 55) | ((long)e11o << 59);
        }

        public void setAt(int idx, int perm, int orient) {
            edges &= ~(0x1FL << 5*idx);
            edges |= ((long)((orient << 4) | perm)) << 5*idx;
        }

        public void setPermAt(int idx, int perm) {
            edges &= ~(0xFL << 5*idx);
            edges |= ((long)perm) << 5*idx;
        }

        public void setOrientAt(int idx, int orient) {
            edges &= ~(0x1L << (5*idx+4));
            edges |= ((long)orient) << (5*idx+4);
        }

        public static cubeedges compose(cubeedges ce1, cubeedges ce2) {
            cubeedges res = new cubeedges();
            for (int i = 0; i < 12; i++) {
                int edge2perm  = (int)((ce2.edges >>> 5*i) & 0xfL);
                long edge1item = (ce1.edges >>> 5*edge2perm) & 0x1fL;
                res.edges |= edge1item << 5*i;
            }
            long edge2orients = ce2.edges & 0x842108421084210L;
            res.edges ^= edge2orients;
            return res;
        }

        public static cubeedges compose3(cubeedges ce1, cubeedges ce2, cubeedges ce3) {
            cubeedges res = new cubeedges();
            //res = compose(compose(ce1, ce2), ce3);
            for (int i = 0; i < 12; i++) {
                int  edge3perm   = (int)((ce3.edges >>> 5*i) & 0xfL);
                long edge2item   = ce2.edges >>> 5*edge3perm;
                int  edge2perm   = (int)(edge2item & 0xfL);
                long edge2orient = edge2item & 0x10L;
                long edge1item   = (ce1.edges >>> 5*edge2perm) & 0x1fL;
                long edgemitem   = edge1item ^ edge2orient;
                res.edges |= edgemitem << 5*i;
            }
            long edge3orients = ce3.edges & 0x842108421084210L;
            res.edges ^= edge3orients;
            return res;
        }

        // the middle cubeedges should be reversed before compose
        public static cubeedges compose3revmid(cubeedges ce1, cubeedges ce2, cubeedges ce3) {
            cubeedges res = new cubeedges();
            int[]  ce2rperm   = new int[12];
            long[] ce2rorient = new long[12];
            for (int i = 0; i < 12; i++) {
                long edge2item  = ce2.edges >>> 5*i;
                int  edge2perm  = (int)(edge2item & 0xfL);
                ce2rperm[edge2perm]   = i;
                ce2rorient[edge2perm] = edge2item & 0x10L;
            }
            for (int i = 0; i < 12; i++) {
                int  edge3perm  = (int)((ce3.edges >>> 5*i) & 0xfL);
                long edge1item  = (ce1.edges >>> 5*ce2rperm[edge3perm]) & 0x1fL;
                long edgemitem  = edge1item ^ ce2rorient[edge3perm];
                res.edges |= edgemitem << 5*i;
            }
            long edge3orients = ce3.edges & 0x842108421084210L;
            res.edges ^= edge3orients;
            return res;
        }

        public static boolean isCeReprSolvable(cubeedges ce) {
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

        public static cubeedges fromPermAndOrientIdx(int permIdxp, int orientIdxp) {
            int permIdx   = permIdxp;
            int orientIdx = orientIdxp;
            long unused   = 0xba9876543210L;
            cubeedges ce  = new cubeedges();
            int orient11  = 0;
            for (int edgeIdx = 12; edgeIdx >= 1; edgeIdx--) {
                int p = (permIdx % edgeIdx) * 4;
                ce.setPermAt(12 - edgeIdx, (int)(unused >>> p) & 0xf);
                ce.setOrientAt(12 - edgeIdx, orientIdx & 1);
                long m = -1L << p;
                unused = (unused & ~m) | ((unused >>> 4) & m);
                permIdx  /= edgeIdx;
                orient11  ^= orientIdx;
                orientIdx /= 2;
            }
            ce.setOrientAt(11, orient11 & 1);
            return ce;
        }

        public cubeedges symmetric() {
            cubeedges ceres = new cubeedges();
            // map perm: 0 1 2 3 4 5 6 7 8 9 10 11 -> 8 9 10 11 4 5 6 7 0 1 2 3
            long edg = edges ^ ((~edges & 0x210842108421084L) << 1);
            ceres.edges = (edg >>> 40) | (edg & 0xfffff00000L) | ((edg & 0xfffffL) << 40);
            return ceres;
        }

        public cubeedges reverse() {
            cubeedges res = new cubeedges();
            for (int i = 0; i < 12; i++) {
                long edgeItem = edges >>> 5*i;
                res.edges |= ((edgeItem & 0x10L) | (long)i) << 5*((int)(edgeItem) & 0xf);
            }
            return res;
        }

        public cubeedges transform(int idx) {
            cubeedges cetrans = ctransformed[idx].ce;
            cubeedges res = new cubeedges();
            for (int i = 0; i < 12; i++) {
                int ceItem    = (int)(this.edges >>> 5*i);
                int cePerm    = ceItem & 0xf;
                int ceOrient  = ceItem & 0x10;
                int ctransPerm  = (int)(cetrans.edges >>> 5*cePerm) & 0xf;
                int ctransPerm2 = (int)(cetrans.edges >>> 5*i) & 0xf;
                res.edges |= ((long)(ctransPerm | ceOrient)) << 5*ctransPerm2;
            }
            return res;
        }

        public int  getPermAt(int idx)   { return (int)(edges >>> 5*idx) & 0xf; }
        public int  getOrientAt(int idx) { return (int)(edges >>> (5*idx+4)) & 1; }

        @Override public boolean equals(Object o) {
            if (!(o instanceof cubeedges)) return false;
            return edges == ((cubeedges)o).edges;
        }
        @Override public int hashCode() { return Long.hashCode(edges); }
        @Override public int compareTo(cubeedges o) { return Long.compare(edges, o.edges); }

        public int getPermIdx() {
            int res = 0;
            long indexes = 0L;
            long e = edges << 2;
            int shift = 60;
            for (int i = 1; i <= 12; i++) {
                shift -= 5;
                int p = (int)(e >>> shift) & 0x3c;
                res = i * res + (int)(indexes >>> p) & 0xf;
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
            return (int)res & 0xffff;
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

        public long get() { return edges; }
        public void set(long e) { edges = e; }
        public boolean isNil() { return edges == 0L; }

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
            int[] orients4567  = {1, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 1};
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
            int[] orients4567  = {2, 1, 1, 2, 0, 0, 0, 0, 2, 1, 1, 2};
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

        public cubeedges representativeBG() {
            cubeedges cerepr = new cubeedges();
            long orientsIn = 0x6060L;
            cubeedges ceRev = reverse();
            int destIn  = 0;
            int destOut = 4;
            int permOutSum = 0;
            for (int i = 0; i < 12; i++) {
                int ipos = ceRev.getPermAt(i);
                long item = (edges >>> 5*ipos) & 0x1fL;
                if (ipos < 4 || ipos >= 8) {
                    item ^= (((orientsIn >>> ipos) ^ (orientsIn >>> destIn)) & 0x10L);
                    cerepr.edges |= item << 5*destIn;
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
                    cerepr.edges |= item << 5*destOut;
                    ++destOut;
                }
            }
            return cerepr;
        }

        public cubeedges representativeYW() {
            cubeedges cerepr = new cubeedges();
            int[] orientsIn = {1, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 1};
            cubeedges ceRev = reverse();
            int destIn  = 0;
            int destOut = 0;
            for (int i = 0; i < 12; i++) {
                int ipos = ceRev.getPermAt(i);
                if (orientsIn[ipos] != 2) {
                    while (orientsIn[destIn] == 2) ++destIn;
                    cerepr.setPermAt(destIn, i);
                    cerepr.setOrientAt(destIn, (orientsIn[ipos] == orientsIn[destIn]) ? getOrientAt(ipos) : 1-getOrientAt(ipos));
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

        public cubeedges representativeOR() {
            cubeedges cerepr = new cubeedges();
            int[] orientsIn = {2, 0, 0, 2, 1, 1, 1, 1, 2, 0, 0, 2};
            cubeedges ceRev = reverse();
            int destIn  = 0;
            int destOut = 0;
            for (int i = 0; i < 12; i++) {
                int ipos = ceRev.getPermAt(i);
                if (orientsIn[ipos] != 2) {
                    while (orientsIn[destIn] == 2) ++destIn;
                    cerepr.setPermAt(destIn, i);
                    cerepr.setOrientAt(destIn, (orientsIn[ipos] == orientsIn[destIn]) ? getOrientAt(ipos) : 1-getOrientAt(ipos));
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

    // -------------------------------------------------------------------------
    // cube
    // -------------------------------------------------------------------------
    public static class cube implements Comparable<cube> {
        public cubecorners_perm ccp;
        public cubecorner_orients cco;
        public cubeedges ce;

        public cube() {
            ccp = new cubecorners_perm();
            cco = new cubecorner_orients();
            ce  = new cubeedges();
        }

        public cube(cubecorners_perm ccp, cubecorner_orients cco, cubeedges ce) {
            this.ccp = ccp;
            this.cco = cco;
            this.ce  = ce;
        }

        public static cube compose(cube c1, cube c2) {
            return new cube(
                cubecorners_perm.compose(c1.ccp, c2.ccp),
                cubecorner_orients.compose(c1.cco, c2.ccp, c2.cco),
                cubeedges.compose(c1.ce, c2.ce));
        }

        public cube symmetric() {
            return new cube(ccp.symmetric(), cco.symmetric(), ce.symmetric());
        }

        public cube reverse() {
            return new cube(ccp.reverse(), cco.reverse(ccp), ce.reverse());
        }

        public cube transform(int transformDir) {
            cube ctrans    = ctransformed[transformDir];
            cubecorners_perm ccp1 = cubecorners_perm.compose(ctrans.ccp, this.ccp);
            cubecorner_orients cco1 = cubecorner_orients.compose(ctrans.cco, this.ccp, this.cco);
            cubeedges ce1 = cubeedges.compose(ctrans.ce, this.ce);

            cube ctransRev = ctransformed[transformReverse(transformDir)];
            cubecorners_perm ccp2 = cubecorners_perm.compose(ccp1, ctransRev.ccp);
            cubecorner_orients cco2 = cubecorner_orients.compose(cco1, ctransRev.ccp, ctransRev.cco);
            cubeedges ce2 = cubeedges.compose(ce1, ctransRev.ce);
            return new cube(ccp2, cco2, ce2);
        }

        @Override public boolean equals(Object o) {
            if (!(o instanceof cube)) return false;
            cube c = (cube)o;
            return ccp.equals(c.ccp) && cco.equals(c.cco) && ce.equals(c.ce);
        }
        @Override public int hashCode() {
            return ccp.hashCode() * 31 * 31 + cco.hashCode() * 31 + ce.hashCode();
        }
        @Override public int compareTo(cube o) {
            if (!ccp.equals(o.ccp)) return ccp.compareTo(o.ccp);
            if (!cco.equals(o.cco)) return cco.compareTo(o.cco);
            return ce.compareTo(o.ce);
        }

        public boolean isBGspace() { return cco.isBGspace() && ce.isBGspace(); }
        public boolean isYWspace() { return cco.isYWspace(ccp) && ce.isYWspace(); }
        public boolean isORspace() { return cco.isORspace(ccp) && ce.isORspace(); }

        public cube representativeBG() {
            return new cube(csolved.ccp, cco.representativeBG(ccp), ce.representativeBG());
        }
        public cube representativeYW() {
            return new cube(csolved.ccp, cco.representativeYW(ccp), ce.representativeYW());
        }
        public cube representativeOR() {
            return new cube(csolved.ccp, cco.representativeOR(ccp), ce.representativeOR());
        }

        public String toParamText() {
            String[] colorChars = {"Y","O","B","R","G","W"};
            StringBuilder res = new StringBuilder();
            res.append(colorChars[cubeCornerColors[ccp.getAt(4)][R120[cco.getAt(4)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(8)][1-ce.getOrientAt(8)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(5)][R240[cco.getAt(5)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(4)][ce.getOrientAt(4)].ordinal()]);
            res.append('Y');
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(5)][ce.getOrientAt(5)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(0)][R240[cco.getAt(0)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(0)][1-ce.getOrientAt(0)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(1)][R120[cco.getAt(1)]].ordinal()]);

            res.append(colorChars[cubeCornerColors[ccp.getAt(4)][R240[cco.getAt(4)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(4)][1-ce.getOrientAt(4)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(0)][R120[cco.getAt(0)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(9)][ce.getOrientAt(9)].ordinal()]);
            res.append('O');
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(1)][ce.getOrientAt(1)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(6)][R120[cco.getAt(6)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(6)][1-ce.getOrientAt(6)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(2)][R240[cco.getAt(2)]].ordinal()]);

            res.append(colorChars[cubeCornerColors[ccp.getAt(0)][cco.getAt(0)].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(0)][ce.getOrientAt(0)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(1)][cco.getAt(1)].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(1)][1-ce.getOrientAt(1)].ordinal()]);
            res.append('B');
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(2)][1-ce.getOrientAt(2)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(2)][cco.getAt(2)].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(3)][ce.getOrientAt(3)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(3)][cco.getAt(3)].ordinal()]);

            res.append(colorChars[cubeCornerColors[ccp.getAt(1)][R240[cco.getAt(1)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(5)][1-ce.getOrientAt(5)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(5)][R120[cco.getAt(5)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(2)][ce.getOrientAt(2)].ordinal()]);
            res.append('R');
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(10)][ce.getOrientAt(10)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(3)][R120[cco.getAt(3)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(7)][1-ce.getOrientAt(7)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(7)][R240[cco.getAt(7)]].ordinal()]);

            res.append(colorChars[cubeCornerColors[ccp.getAt(5)][cco.getAt(5)].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(8)][ce.getOrientAt(8)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(4)][cco.getAt(4)].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(10)][1-ce.getOrientAt(10)].ordinal()]);
            res.append('G');
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(9)][1-ce.getOrientAt(9)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(7)][cco.getAt(7)].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(11)][ce.getOrientAt(11)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(6)][cco.getAt(6)].ordinal()]);

            res.append(colorChars[cubeCornerColors[ccp.getAt(2)][R120[cco.getAt(2)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(3)][1-ce.getOrientAt(3)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(3)][R240[cco.getAt(3)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(6)][ce.getOrientAt(6)].ordinal()]);
            res.append('W');
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(7)][ce.getOrientAt(7)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(6)][R240[cco.getAt(6)]].ordinal()]);
            res.append(colorChars[cubeEdgeColors[ce.getPermAt(11)][1-ce.getOrientAt(11)].ordinal()]);
            res.append(colorChars[cubeCornerColors[ccp.getAt(7)][R120[cco.getAt(7)]].ordinal()]);
            return res.toString();
        }
    }

    // -------------------------------------------------------------------------
    // cubecorners helper
    // -------------------------------------------------------------------------
    public static class cubecorners {
        public cubecorners_perm perm;
        public cubecorner_orients orients;
        public cubecorners(int c0p, int c0o, int c1p, int c1o, int c2p, int c2o,
                           int c3p, int c3o, int c4p, int c4o, int c5p, int c5o,
                           int c6p, int c6o, int c7p, int c7o) {
            perm    = new cubecorners_perm(c0p, c1p, c2p, c3p, c4p, c5p, c6p, c7p);
            orients = new cubecorner_orients(c0o, c1o, c2o, c3o, c4o, c5o, c6o, c7o);
        }
    }

    // -------------------------------------------------------------------------
    // Static cube data
    // -------------------------------------------------------------------------
    public static final cube csolved = new cube(
        new cubecorners_perm(0,1,2,3,4,5,6,7),
        new cubecorner_orients(0,0,0,0,0,0,0,0),
        new cubeedges(0,1,2,3,4,5,6,7,8,9,10,11, 0,0,0,0,0,0,0,0,0,0,0,0));

    public static final cube[] crotated = {
        new cube( // ORANGECW
            new cubecorners_perm(4,1,0,3,6,5,2,7),
            new cubecorner_orients(1,0,2,0,2,0,1,0),
            new cubeedges(0,4,2,3,9,5,1,7,8,6,10,11, 0,1,0,0,1,0,1,0,0,1,0,0)),
        new cube( // ORANGE180
            new cubecorners_perm(6,1,4,3,2,5,0,7),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(0,9,2,3,6,5,4,7,8,1,10,11, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // ORANGECCW
            new cubecorners_perm(2,1,6,3,0,5,4,7),
            new cubecorner_orients(1,0,2,0,2,0,1,0),
            new cubeedges(0,6,2,3,1,5,9,7,8,4,10,11, 0,1,0,0,1,0,1,0,0,1,0,0)),
        new cube( // REDCW
            new cubecorners_perm(0,3,2,7,4,1,6,5),
            new cubecorner_orients(0,2,0,1,0,1,0,2),
            new cubeedges(0,1,7,3,4,2,6,10,8,9,5,11, 0,0,1,0,0,1,0,1,0,0,1,0)),
        new cube( // RED180
            new cubecorners_perm(0,7,2,5,4,3,6,1),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(0,1,10,3,4,7,6,5,8,9,2,11, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // REDCCW
            new cubecorners_perm(0,5,2,1,4,7,6,3),
            new cubecorner_orients(0,2,0,1,0,1,0,2),
            new cubeedges(0,1,5,3,4,10,6,2,8,9,7,11, 0,0,1,0,0,1,0,1,0,0,1,0)),
        new cube( // YELLOWCW
            new cubecorners_perm(1,5,2,3,0,4,6,7),
            new cubecorner_orients(2,1,0,0,1,2,0,0),
            new cubeedges(5,1,2,3,0,8,6,7,4,9,10,11, 1,0,0,0,1,1,0,0,1,0,0,0)),
        new cube( // YELLOW180
            new cubecorners_perm(5,4,2,3,1,0,6,7),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(8,1,2,3,5,4,6,7,0,9,10,11, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // YELLOWCCW
            new cubecorners_perm(4,0,2,3,5,1,6,7),
            new cubecorner_orients(2,1,0,0,1,2,0,0),
            new cubeedges(4,1,2,3,8,0,6,7,5,9,10,11, 1,0,0,0,1,1,0,0,1,0,0,0)),
        new cube( // WHITECW
            new cubecorners_perm(0,1,6,2,4,5,7,3),
            new cubecorner_orients(0,0,1,2,0,0,2,1),
            new cubeedges(0,1,2,6,4,5,11,3,8,9,10,7, 0,0,0,1,0,0,1,1,0,0,0,1)),
        new cube( // WHITE180
            new cubecorners_perm(0,1,7,6,4,5,3,2),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(0,1,2,11,4,5,7,6,8,9,10,3, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // WHITECCW
            new cubecorners_perm(0,1,3,7,4,5,2,6),
            new cubecorner_orients(0,0,1,2,0,0,2,1),
            new cubeedges(0,1,2,7,4,5,3,11,8,9,10,6, 0,0,0,1,0,0,1,1,0,0,0,1)),
        new cube( // GREENCW
            new cubecorners_perm(0,1,2,3,5,7,4,6),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(0,1,2,3,4,5,6,7,10,8,11,9, 0,0,0,0,0,0,0,0,1,1,1,1)),
        new cube( // GREEN180
            new cubecorners_perm(0,1,2,3,7,6,5,4),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(0,1,2,3,4,5,6,7,11,10,9,8, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // GREENCCW
            new cubecorners_perm(0,1,2,3,6,4,7,5),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(0,1,2,3,4,5,6,7,9,11,8,10, 0,0,0,0,0,0,0,0,1,1,1,1)),
        new cube( // BLUECW
            new cubecorners_perm(2,0,3,1,4,5,6,7),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(1,3,0,2,4,5,6,7,8,9,10,11, 1,1,1,1,0,0,0,0,0,0,0,0)),
        new cube( // BLUE180
            new cubecorners_perm(3,2,1,0,4,5,6,7),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(3,2,1,0,4,5,6,7,8,9,10,11, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // BLUECCW
            new cubecorners_perm(1,3,0,2,4,5,6,7),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(2,0,3,1,4,5,6,7,8,9,10,11, 1,1,1,1,0,0,0,0,0,0,0,0))
    };

    public static final cube[] ctransformed = {
        csolved,
        new cube( // TD_C0_7_CW
            new cubecorners_perm(0,2,4,6,1,3,5,7),
            new cubecorner_orients(1,2,2,1,2,1,1,2),
            new cubeedges(1,4,6,9,0,3,8,11,2,5,7,10, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_C0_7_CCW
            new cubecorners_perm(0,4,1,5,2,6,3,7),
            new cubecorner_orients(2,1,1,2,1,2,2,1),
            new cubeedges(4,0,8,5,1,9,2,10,6,3,11,7, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_C1_6_CW
            new cubecorners_perm(5,1,4,0,7,3,6,2),
            new cubecorner_orients(2,1,1,2,1,2,2,1),
            new cubeedges(5,8,0,4,10,2,9,1,7,11,3,6, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_C1_6_CCW
            new cubecorners_perm(3,1,7,5,2,0,6,4),
            new cubecorner_orients(1,2,2,1,2,1,1,2),
            new cubeedges(2,7,5,10,3,0,11,8,1,6,4,9, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_C2_5_CW
            new cubecorners_perm(3,7,2,6,1,5,0,4),
            new cubecorner_orients(2,1,1,2,1,2,2,1),
            new cubeedges(7,3,11,6,2,10,1,9,5,0,8,4, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_C2_5_CCW
            new cubecorners_perm(6,4,2,0,7,5,3,1),
            new cubecorner_orients(1,2,2,1,2,1,1,2),
            new cubeedges(9,6,4,1,11,8,3,0,10,7,5,2, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_C3_4_CW
            new cubecorners_perm(5,7,1,3,4,6,0,2),
            new cubecorner_orients(1,2,2,1,2,1,1,2),
            new cubeedges(10,5,7,2,8,11,0,3,9,4,6,1, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_C3_4_CCW
            new cubecorners_perm(6,2,7,3,4,0,5,1),
            new cubecorner_orients(2,1,1,2,1,2,2,1),
            new cubeedges(6,11,3,7,9,1,10,2,4,8,0,5, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_BG_CW
            new cubecorners_perm(1,3,0,2,5,7,4,6),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(2,0,3,1,5,7,4,6,10,8,11,9, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_BG_180
            new cubecorners_perm(3,2,1,0,7,6,5,4),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(3,2,1,0,7,6,5,4,11,10,9,8, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_BG_CCW
            new cubecorners_perm(2,0,3,1,6,4,7,5),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(1,3,0,2,6,4,7,5,9,11,8,10, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_YW_CW
            new cubecorners_perm(4,0,6,2,5,1,7,3),
            new cubecorner_orients(2,1,1,2,1,2,2,1),
            new cubeedges(4,9,1,6,8,0,11,3,5,10,2,7, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_YW_180
            new cubecorners_perm(5,4,7,6,1,0,3,2),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(8,10,9,11,5,4,7,6,0,2,1,3, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_YW_CCW
            new cubecorners_perm(1,5,3,7,0,4,2,6),
            new cubecorner_orients(2,1,1,2,1,2,2,1),
            new cubeedges(5,2,10,7,0,8,3,11,4,1,9,6, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_OR_CW
            new cubecorners_perm(4,5,0,1,6,7,2,3),
            new cubecorner_orients(1,2,2,1,2,1,1,2),
            new cubeedges(8,4,5,0,9,10,1,2,11,6,7,3, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_OR_180
            new cubecorners_perm(6,7,4,5,2,3,0,1),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(11,9,10,8,6,7,4,5,3,1,2,0, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new cube( // TD_OR_CCW
            new cubecorners_perm(2,3,6,7,0,1,4,5),
            new cubecorner_orients(1,2,2,1,2,1,1,2),
            new cubeedges(3,6,7,11,1,2,9,10,0,4,5,8, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_E0_11
            new cubecorners_perm(1,0,5,4,3,2,7,6),
            new cubecorner_orients(1,2,2,1,2,1,1,2),
            new cubeedges(0,5,4,8,2,1,10,9,3,7,6,11, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_E1_10
            new cubecorners_perm(2,6,0,4,3,7,1,5),
            new cubecorner_orients(2,1,1,2,1,2,2,1),
            new cubeedges(6,1,9,4,3,11,0,8,7,2,10,5, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_E2_9
            new cubecorners_perm(7,3,5,1,6,2,4,0),
            new cubecorner_orients(2,1,1,2,1,2,2,1),
            new cubeedges(7,10,2,5,11,3,8,0,6,9,1,4, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_E3_8
            new cubecorners_perm(7,6,3,2,5,4,1,0),
            new cubecorner_orients(1,2,2,1,2,1,1,2),
            new cubeedges(11,7,6,3,10,9,2,1,8,5,4,0, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_E4_7
            new cubecorners_perm(4,6,5,7,0,2,1,3),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(9,8,11,10,4,6,5,7,1,0,3,2, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new cube( // TD_E5_6
            new cubecorners_perm(7,5,6,4,3,1,2,0),
            new cubecorner_orients(0,0,0,0,0,0,0,0),
            new cubeedges(10,11,8,9,7,5,6,4,2,3,0,1, 1,1,1,1,1,1,1,1,1,1,1,1))
    };

    // -------------------------------------------------------------------------
    // Helper functions
    // -------------------------------------------------------------------------
    public static String rotateDirName(int rd) {
        return switch (rotate_dir.values()[rd]) {
            case ORANGECW  -> "orange-cw";
            case ORANGE180 -> "orange-180";
            case ORANGECCW -> "orange-ccw";
            case REDCW     -> "red-cw";
            case RED180    -> "red-180";
            case REDCCW    -> "red-ccw";
            case YELLOWCW  -> "yellow-cw";
            case YELLOW180 -> "yellow-180";
            case YELLOWCCW -> "yellow-ccw";
            case WHITECW   -> "white-cw";
            case WHITE180  -> "white-180";
            case WHITECCW  -> "white-ccw";
            case GREENCW   -> "green-cw";
            case GREEN180  -> "green-180";
            case GREENCCW  -> "green-ccw";
            case BLUECW    -> "blue-cw";
            case BLUE180   -> "blue-180";
            case BLUECCW   -> "blue-ccw";
            default        -> String.valueOf(rd);
        };
    }

    public static int rotateNameToDir(String rotateName) {
        for (int rd = 0; rd < rotate_dir.RCOUNT.ordinal(); rd++) {
            String dirName = rotateDirName(rd);
            if (rotateName.startsWith(dirName))
                return rd;
        }
        return rotate_dir.RCOUNT.ordinal();
    }

    public static int rotateDirReverse(int rd) {
        return switch (rotate_dir.values()[rd]) {
            case ORANGECW  -> rotate_dir.ORANGECCW.ordinal();
            case ORANGE180 -> rotate_dir.ORANGE180.ordinal();
            case ORANGECCW -> rotate_dir.ORANGECW.ordinal();
            case REDCW     -> rotate_dir.REDCCW.ordinal();
            case RED180    -> rotate_dir.RED180.ordinal();
            case REDCCW    -> rotate_dir.REDCW.ordinal();
            case YELLOWCW  -> rotate_dir.YELLOWCCW.ordinal();
            case YELLOW180 -> rotate_dir.YELLOW180.ordinal();
            case YELLOWCCW -> rotate_dir.YELLOWCW.ordinal();
            case WHITECW   -> rotate_dir.WHITECCW.ordinal();
            case WHITE180  -> rotate_dir.WHITE180.ordinal();
            case WHITECCW  -> rotate_dir.WHITECW.ordinal();
            case GREENCW   -> rotate_dir.GREENCCW.ordinal();
            case GREEN180  -> rotate_dir.GREEN180.ordinal();
            case GREENCCW  -> rotate_dir.GREENCW.ordinal();
            case BLUECW    -> rotate_dir.BLUECCW.ordinal();
            case BLUE180   -> rotate_dir.BLUE180.ordinal();
            case BLUECCW   -> rotate_dir.BLUECW.ordinal();
            default        -> rotate_dir.RCOUNT.ordinal();
        };
    }

    public static int transformReverse(int idx) {
        transform_dir td = transform_dir.values()[idx];
        return switch (td) {
            case TD_C0_7_CW  -> transform_dir.TD_C0_7_CCW.ordinal();
            case TD_C0_7_CCW -> transform_dir.TD_C0_7_CW.ordinal();
            case TD_C1_6_CW  -> transform_dir.TD_C1_6_CCW.ordinal();
            case TD_C1_6_CCW -> transform_dir.TD_C1_6_CW.ordinal();
            case TD_C2_5_CW  -> transform_dir.TD_C2_5_CCW.ordinal();
            case TD_C2_5_CCW -> transform_dir.TD_C2_5_CW.ordinal();
            case TD_C3_4_CW  -> transform_dir.TD_C3_4_CCW.ordinal();
            case TD_C3_4_CCW -> transform_dir.TD_C3_4_CW.ordinal();
            case TD_BG_CW    -> transform_dir.TD_BG_CCW.ordinal();
            case TD_BG_CCW   -> transform_dir.TD_BG_CW.ordinal();
            case TD_YW_CW    -> transform_dir.TD_YW_CCW.ordinal();
            case TD_YW_CCW   -> transform_dir.TD_YW_CW.ordinal();
            case TD_OR_CW    -> transform_dir.TD_OR_CCW.ordinal();
            case TD_OR_CCW   -> transform_dir.TD_OR_CW.ordinal();
            default          -> idx;
        };
    }

    public static String transformName(int td) {
        return switch (transform_dir.values()[td]) {
            case TD_0       -> "0";
            case TD_C0_7_CW -> "c0-7.cw";
            case TD_C0_7_CCW-> "c0-7.ccw";
            case TD_C1_6_CW -> "c1-6.cw";
            case TD_C1_6_CCW-> "c1-6.ccw";
            case TD_C2_5_CW -> "c2-5.cw";
            case TD_C2_5_CCW-> "c2-5.ccw";
            case TD_C3_4_CW -> "c3-4.cw";
            case TD_C3_4_CCW-> "c3-4.ccw";
            case TD_BG_CW   -> "bg.cw";
            case TD_BG_180  -> "bg.180";
            case TD_BG_CCW  -> "bg.ccw";
            case TD_YW_CW   -> "yw.cw";
            case TD_YW_180  -> "yw.180";
            case TD_YW_CCW  -> "yw.ccw";
            case TD_OR_CW   -> "or.cw";
            case TD_OR_180  -> "or.180";
            case TD_OR_CCW  -> "or.ccw";
            case TD_E0_11   -> "e0-11";
            case TD_E1_10   -> "e1-10";
            case TD_E2_9    -> "e2-9";
            case TD_E3_8    -> "e3-8";
            case TD_E4_7    -> "e4-7";
            case TD_E5_6    -> "e5-6";
            default         -> "unknown";
        };
    }

    public static void cubePrint(cube c) {
        String[] colorPrint = {
            "\u001B[48;2;230;230;0m  \u001B[m",
            "\u001B[48;2;230;148;0m  \u001B[m",
            "\u001B[48;2;0;0;230m  \u001B[m",
            "\u001B[48;2;230;0;0m  \u001B[m",
            "\u001B[48;2;0;230;0m  \u001B[m",
            "\u001B[48;2;230;230;230m  \u001B[m"
        };
        System.out.println();
        System.out.println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(4)][R120[c.cco.getAt(4)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(8)][1-c.ce.getOrientAt(8)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(5)][R240[c.cco.getAt(5)]].ordinal()]);
        System.out.println("        " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(4)][c.ce.getOrientAt(4)].ordinal()] +
            colorPrint[cubecolor.CYELLOW.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(5)][c.ce.getOrientAt(5)].ordinal()]);
        System.out.println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(0)][R240[c.cco.getAt(0)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(0)][1-c.ce.getOrientAt(0)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(1)][R120[c.cco.getAt(1)]].ordinal()]);
        System.out.println();
        System.out.println(" " +
            colorPrint[cubeCornerColors[c.ccp.getAt(4)][R240[c.cco.getAt(4)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(4)][1-c.ce.getOrientAt(4)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(0)][R120[c.cco.getAt(0)]].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(0)][c.cco.getAt(0)].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(0)][c.ce.getOrientAt(0)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(1)][c.cco.getAt(1)].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(1)][R240[c.cco.getAt(1)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(5)][1-c.ce.getOrientAt(5)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(5)][R120[c.cco.getAt(5)]].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(5)][c.cco.getAt(5)].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(8)][c.ce.getOrientAt(8)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(4)][c.cco.getAt(4)].ordinal()]);
        System.out.println(" " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(9)][c.ce.getOrientAt(9)].ordinal()] +
            colorPrint[cubecolor.CORANGE.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(1)][c.ce.getOrientAt(1)].ordinal()] + " " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(1)][1-c.ce.getOrientAt(1)].ordinal()] +
            colorPrint[cubecolor.CBLUE.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(2)][1-c.ce.getOrientAt(2)].ordinal()] + " " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(2)][c.ce.getOrientAt(2)].ordinal()] +
            colorPrint[cubecolor.CRED.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(10)][c.ce.getOrientAt(10)].ordinal()] + " " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(10)][1-c.ce.getOrientAt(10)].ordinal()] +
            colorPrint[cubecolor.CGREEN.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(9)][1-c.ce.getOrientAt(9)].ordinal()]);
        System.out.println(" " +
            colorPrint[cubeCornerColors[c.ccp.getAt(6)][R120[c.cco.getAt(6)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(6)][1-c.ce.getOrientAt(6)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(2)][R240[c.cco.getAt(2)]].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(2)][c.cco.getAt(2)].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(3)][c.ce.getOrientAt(3)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(3)][c.cco.getAt(3)].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(3)][R120[c.cco.getAt(3)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(7)][1-c.ce.getOrientAt(7)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(7)][R240[c.cco.getAt(7)]].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(7)][c.cco.getAt(7)].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(11)][c.ce.getOrientAt(11)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(6)][c.cco.getAt(6)].ordinal()]);
        System.out.println();
        System.out.println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(2)][R120[c.cco.getAt(2)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(3)][1-c.ce.getOrientAt(3)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(3)][R240[c.cco.getAt(3)]].ordinal()]);
        System.out.println("        " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(6)][c.ce.getOrientAt(6)].ordinal()] +
            colorPrint[cubecolor.CWHITE.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(7)][c.ce.getOrientAt(7)].ordinal()]);
        System.out.println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(6)][R240[c.cco.getAt(6)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(11)][1-c.ce.getOrientAt(11)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(7)][R120[c.cco.getAt(7)]].ordinal()]);
        System.out.println();
    }
}

