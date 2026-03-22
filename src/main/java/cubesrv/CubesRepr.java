package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.CPermRepr.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.TreeMap;

public class CubesRepr {

    public static class CornerOrientReprCubes {
        private final cubecorner_orients m_orients;
        public long[] m_items = new long[0];
        private int[] m_orientOccur = null;

        public CornerOrientReprCubes(cubecorner_orients orients) {
            this.m_orients = orients;
        }

        public cubecorner_orients getOrients() { return m_orients; }

        public int addCubes(Collection<cubeedges> cearr) {
            TreeMap<Long, Integer> edgeMap = new TreeMap<>();
            for (cubeedges ce : cearr) {
                long edge = ce.get();
                if (!edgeMap.containsKey(edge)) {
                    int idx = Arrays.binarySearch(m_items, edge);
                    if (idx < 0) {
                        edgeMap.put(edge, -1 - idx);
                    }
                }
            }
            List<java.util.Map.Entry<Long,Integer>> edgeList = new ArrayList<>(edgeMap.entrySet());
            long[] itemsNew = new long[m_items.length + edgeList.size()];
            int srcPos = 0, idxsPos = 0, destPos = 0;
            while (destPos < itemsNew.length) {
                if (idxsPos < edgeList.size() && edgeList.get(idxsPos).getValue() == srcPos) {
                    itemsNew[destPos] = edgeList.get(idxsPos).getKey();
                    ++idxsPos;
                } else {
                    itemsNew[destPos] = m_items[srcPos];
                    ++srcPos;
                }
                ++destPos;
            }
            m_items = itemsNew;
            return edgeList.size();
        }

        public void initOccur() {
            int[] orientOcc = new int[64];
            for (long edges : m_items) {
                cubeedges ce = new cubeedges(edges);
                int orientIdx = ce.getOrientIdx();
                orientOcc[orientIdx >>> 5] |= (1 << (orientIdx & 0x1f));
            }
            m_orientOccur = orientOcc;
        }

        public boolean containsCubeEdges(cubeedges ce) {
            if (m_orientOccur != null) {
                int orientIdx = ce.getOrientIdx();
                int orientOcc = m_orientOccur[orientIdx >>> 5];
                if ((orientOcc & (1 << (orientIdx & 0x1f))) == 0)
                    return false;
            }
            return Arrays.binarySearch(m_items, ce.get()) >= 0;
        }

        public boolean empty() { return m_items.length == 0; }
        public int size() { return m_items.length; }
        public long[] edgeList() { return m_items; }

        private static cubeedges findSolutionEdgeMulti(CornerOrientReprCubes ccoReprCubes,
                CornerOrientReprCubes ccoReprSearchCubes,
                List<EdgeReprCandidateTransform> otransform, boolean reversed) {
            for (long edges : ccoReprCubes.edgeList()) {
                cubeedges ce = new cubeedges(edges);
                cubeedges ceSearchRepr = CubecornerReprPerms.getComposedReprCubeedges(ce, reversed, otransform);
                if (ccoReprSearchCubes.containsCubeEdges(ceSearchRepr))
                    return ce;
            }
            return new cubeedges();
        }

        private static cubeedges findSolutionEdgeSingle(CornerOrientReprCubes ccoReprCubes,
                CornerOrientReprCubes ccoReprSearchCubes,
                EdgeReprCandidateTransform erct, boolean reversed) {
            cubeedges cetrans    = ctransformed[erct.transformedIdx].ce;
            cubeedges cetransRev = ctransformed[transformReverse(erct.transformedIdx)].ce;
            if (ccoReprCubes.size() <= ccoReprSearchCubes.size()) {
                for (long edges : ccoReprCubes.edgeList()) {
                    cubeedges ce = new cubeedges(edges);
                    cubeedges cesymm = erct.symmetric ? ce.symmetric() : ce;
                    cubeedges ceSearchRepr;
                    if (reversed) {
                        if (erct.reversed) ceSearchRepr = cubeedges.compose3revmid(cetrans, cesymm, erct.ceTrans);
                        else               ceSearchRepr = cubeedges.compose3(erct.ceTrans, cesymm, cetransRev);
                    } else {
                        if (erct.reversed) ceSearchRepr = cubeedges.compose3revmid(erct.ceTrans, cesymm, cetransRev);
                        else               ceSearchRepr = cubeedges.compose3(cetrans, cesymm, erct.ceTrans);
                    }
                    if (ccoReprSearchCubes.containsCubeEdges(ceSearchRepr))
                        return ce;
                }
            } else {
                cubeedges erctCeTransRev = erct.ceTrans.reverse();
                for (long edges : ccoReprSearchCubes.edgeList()) {
                    cubeedges ce = new cubeedges(edges);
                    cubeedges ceSearch;
                    if (reversed) {
                        if (erct.reversed) ceSearch = cubeedges.compose3revmid(erct.ceTrans, ce, cetrans);
                        else               ceSearch = cubeedges.compose3(erctCeTransRev, ce, cetrans);
                    } else {
                        if (erct.reversed) ceSearch = cubeedges.compose3revmid(cetransRev, ce, erct.ceTrans);
                        else               ceSearch = cubeedges.compose3(cetransRev, ce, erctCeTransRev);
                    }
                    cubeedges cesymmSearch = erct.symmetric ? ceSearch.symmetric() : ceSearch;
                    if (ccoReprCubes.containsCubeEdges(cesymmSearch))
                        return cesymmSearch;
                }
            }
            return new cubeedges();
        }

        public static cubeedges findSolutionEdge(CornerOrientReprCubes ccoReprCubes,
                CornerOrientReprCubes ccoReprSearchCubes,
                List<EdgeReprCandidateTransform> otransform, boolean reversed) {
            if (otransform.size() == 1)
                return findSolutionEdgeSingle(ccoReprCubes, ccoReprSearchCubes, otransform.get(0), reversed);
            else
                return findSolutionEdgeMulti(ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
        }
    }

    // Binary search by key extractor on a sorted list
    static int binarySearchByOrients(List<CornerOrientReprCubes> list, cubecorner_orients key) {
        int lo = 0, hi = list.size() - 1;
        while (lo <= hi) {
            int mid = (lo + hi) >>> 1;
            int cmp = list.get(mid).getOrients().compareTo(key);
            if (cmp < 0) lo = mid + 1;
            else if (cmp > 0) hi = mid - 1;
            else return mid;
        }
        return -(lo + 1);
    }

    public static class CornerPermReprCubes {
        private static final CornerOrientReprCubes m_coreprCubesEmpty =
            new CornerOrientReprCubes(new cubecorner_orients());
        private final List<CornerOrientReprCubes> m_coreprCubes = new ArrayList<>();

        public boolean empty() { return m_coreprCubes.isEmpty(); }
        public int size() { return m_coreprCubes.size(); }

        public int cubeCount() {
            int res = 0;
            for (CornerOrientReprCubes it : m_coreprCubes) res += it.size();
            return res;
        }

        public void initOccur() {
            for (CornerOrientReprCubes cerepr : m_coreprCubes) cerepr.initOccur();
        }

        public CornerOrientReprCubes cornerOrientCubesAt(cubecorner_orients cco) {
            int idx = binarySearchByOrients(m_coreprCubes, cco);
            return idx >= 0 ? m_coreprCubes.get(idx) : m_coreprCubesEmpty;
        }

        public CornerOrientReprCubes cornerOrientCubesAdd(cubecorner_orients cco) {
            int idx = binarySearchByOrients(m_coreprCubes, cco);
            if (idx < 0) {
                idx = -1 - idx;
                m_coreprCubes.add(idx, new CornerOrientReprCubes(cco));
            }
            return m_coreprCubes.get(idx);
        }

        public Collection<CornerOrientReprCubes> ccoCubesList() { return m_coreprCubes; }
    }

    public static class CubesReprAtDepth {
        private final CubecornerReprPerms m_reprPerms;
        private final CornerPermReprCubes[] m_cornerPermReprCubes;

        public CubesReprAtDepth(CubecornerReprPerms reprPerms) {
            this.m_reprPerms = reprPerms;
            m_cornerPermReprCubes = new CornerPermReprCubes[reprPerms.reprPermCount()];
            for (int i = 0; i < m_cornerPermReprCubes.length; i++)
                m_cornerPermReprCubes[i] = new CornerPermReprCubes();
        }

        public int size() { return m_cornerPermReprCubes.length; }

        public int cubeCount() {
            int res = 0;
            for (CornerPermReprCubes c : m_cornerPermReprCubes) res += c.cubeCount();
            return res;
        }

        public CornerPermReprCubes add(int idx) { return m_cornerPermReprCubes[idx]; }
        public void initOccur(int idx) { m_cornerPermReprCubes[idx].initOccur(); }
        public CornerPermReprCubes getAt(int idx) { return m_cornerPermReprCubes[idx]; }

        public CornerPermReprCubes getFor(cubecorners_perm ccp) {
            int reprPermIdx = m_reprPerms.getReprPermIdx(ccp);
            return m_cornerPermReprCubes[reprPermIdx];
        }

        public CornerPermReprCubes[] ccpCubesList() { return m_cornerPermReprCubes; }

        public cubecorners_perm getPermAt(int reprPermIdx) {
            return m_reprPerms.getPermForIdx(reprPermIdx);
        }
    }

    public static class CubesReprByDepth {
        public final CubecornerReprPerms m_reprPerms;
        private final List<CubesReprAtDepth> m_cubesAtDepths = new ArrayList<>();
        private int m_availCount = 1;

        public CubesReprByDepth(boolean useReverse) {
            this.m_reprPerms = new CubecornerReprPerms(useReverse);
            m_cubesAtDepths.add(new CubesReprAtDepth(m_reprPerms));
            int cornerPermReprIdx = m_reprPerms.getReprPermIdx(csolved.ccp);
            CornerPermReprCubes ccpCubes = m_cubesAtDepths.get(0).add(cornerPermReprIdx);
            List<EdgeReprCandidateTransform> otransform = new ArrayList<>();
            cubecorner_orients ccoRepr = m_reprPerms.getReprOrients(csolved.ccp, csolved.cco, otransform);
            CornerOrientReprCubes ccoCubes = ccpCubes.cornerOrientCubesAdd(ccoRepr);
            cubeedges ceRepr = m_reprPerms.getReprCubeedges(csolved.ce, otransform);
            ccoCubes.addCubes(java.util.Collections.singletonList(ceRepr));
        }

        public boolean isUseReverse() { return m_reprPerms.isUseReverse(); }
        public int availCount() { return m_availCount; }
        public void incAvailCount() { ++m_availCount; }

        public CubesReprAtDepth getAt(int idx) {
            while (idx >= m_cubesAtDepths.size())
                m_cubesAtDepths.add(new CubesReprAtDepth(m_reprPerms));
            return m_cubesAtDepths.get(idx);
        }

        public cubecorners_perm getReprPermForIdx(int reprPermIdx) {
            return m_reprPerms.getPermForIdx(reprPermIdx);
        }

        public String getMoves(cube c, boolean movesRevp) {
            boolean movesRev = movesRevp;
            List<Integer> rotateDirs = new ArrayList<>();
            int insertPos = 0;
            cube crepr = m_reprPerms.cubeRepresentative(c);
            int ccpReprIdx = m_reprPerms.getReprPermIdx(crepr.ccp);
            int depth = 0;
            while (true) {
                CornerPermReprCubes ccpReprCubes = m_cubesAtDepths.get(depth).getAt(ccpReprIdx);
                CornerOrientReprCubes ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(crepr.cco);
                if (ccoReprCubes.containsCubeEdges(crepr.ce)) break;
                ++depth;
                if (depth >= m_availCount) {
                    System.out.println("getMoves: depth reached maximum, cube NOT FOUND");
                    System.exit(1);
                }
            }
            cube cc = c;
            while (depth-- > 0) {
                int cm = 0;
                cube ccRev = cc.reverse();
                cube cc1 = cc;
                while (cm < rotate_dir.RCOUNT.ordinal()) {
                    cc1 = cube.compose(cc, crotated[cm]);
                    cube cc1repr = m_reprPerms.cubeRepresentative(cc1);
                    int ccpReprIdx2 = m_reprPerms.getReprPermIdx(cc1repr.ccp);
                    CornerPermReprCubes ccpReprCubes = m_cubesAtDepths.get(depth).getAt(ccpReprIdx2);
                    CornerOrientReprCubes ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(cc1repr.cco);
                    if (ccoReprCubes.containsCubeEdges(cc1repr.ce)) break;
                    cc1 = cube.compose(ccRev, crotated[cm]);
                    cc1repr = m_reprPerms.cubeRepresentative(cc1);
                    ccpReprIdx2 = m_reprPerms.getReprPermIdx(cc1repr.ccp);
                    CornerPermReprCubes ccpReprCubesRev = m_cubesAtDepths.get(depth).getAt(ccpReprIdx2);
                    CornerOrientReprCubes ccoReprCubesRev = ccpReprCubesRev.cornerOrientCubesAt(cc1repr.cco);
                    if (ccoReprCubesRev.containsCubeEdges(cc1repr.ce)) {
                        movesRev = !movesRev;
                        break;
                    }
                    ++cm;
                }
                if (cm == rotate_dir.RCOUNT.ordinal()) {
                    System.out.println("getMoves: cube at depth " + depth + " NOT FOUND");
                    System.exit(1);
                }
                rotateDirs.add(insertPos, movesRev ? cm : rotateDirReverse(cm));
                if (movesRev) ++insertPos;
                cc = cc1;
            }
            StringBuilder res = new StringBuilder();
            for (int rotateDir : rotateDirs) {
                res.append(' ');
                res.append(rotateDirName(rotateDir));
            }
            return res.toString();
        }

        public String getMoves(cube c) { return getMoves(c, false); }

        public int addCubesForReprPerm(int reprPermIdx, int depth) {
            List<EdgeReprCandidateTransform> otransformNew = new ArrayList<>();
            int cubeCount = 0;
            CubesReprAtDepth ccpReprCubesC    = m_cubesAtDepths.get(depth-1);
            CornerPermReprCubes ccpReprCubesNewP = (depth == 1) ? null : m_cubesAtDepths.get(depth-2).getAt(reprPermIdx);
            CornerPermReprCubes ccpReprCubesNewC = ccpReprCubesC.getAt(reprPermIdx);
            CornerPermReprCubes ccpReprCubesNewN = m_cubesAtDepths.get(depth).add(reprPermIdx);
            cubecorners_perm ccpNewRepr = m_reprPerms.getPermForIdx(reprPermIdx);
            java.util.Set<cubecorners_perm> ccpChecked = new java.util.HashSet<>();
            for (int trrev = 0; trrev < (m_reprPerms.isUseReverse() ? 2 : 1); trrev++) {
                cubecorners_perm ccpNewReprRev = (trrev != 0) ? ccpNewRepr.reverse() : ccpNewRepr;
                for (int symmetric = 0; symmetric <= 1; symmetric++) {
                    cubecorners_perm ccpNewS = (symmetric != 0) ? ccpNewReprRev.symmetric() : ccpNewReprRev;
                    for (int td = 0; td < transform_dir.TCOUNT.ordinal(); td++) {
                        cubecorners_perm ccpNew = ccpNewS.transform(td);
                        if (!ccpChecked.contains(ccpNew)) {
                            ccpChecked.add(ccpNew);
                            for (int rd = 0; rd < rotate_dir.RCOUNT.ordinal(); rd++) {
                                int rdRev = rotateDirReverse(rd);
                                for (int reversed = 0; reversed < (m_reprPerms.isUseReverse() ? 2 : 1); reversed++) {
                                    cubecorners_perm ccp = (reversed != 0)
                                        ? cubecorners_perm.compose(crotated[rdRev].ccp, ccpNew)
                                        : cubecorners_perm.compose(ccpNew, crotated[rdRev].ccp);
                                    int ccpReprIdx = m_reprPerms.getReprPermIdx(ccp);
                                    if (m_reprPerms.getPermForIdx(ccpReprIdx).equals(ccp)) {
                                        CornerPermReprCubes cpermReprCubesC = ccpReprCubesC.getAt(ccpReprIdx);
                                        for (CornerOrientReprCubes corientReprCubesC : cpermReprCubesC.ccoCubesList()) {
                                            cubecorner_orients cco = corientReprCubesC.getOrients();
                                            cubecorner_orients ccoNew = (reversed != 0)
                                                ? cubecorner_orients.compose(crotated[rd].cco, ccp, cco)
                                                : cubecorner_orients.compose(cco, crotated[rd].ccp, crotated[rd].cco);
                                            cubecorner_orients ccoReprNew = m_reprPerms.getComposedReprOrients(
                                                    ccpNew, ccoNew, reversed != 0, crotated[rd].ce, otransformNew);
                                            CornerOrientReprCubes corientReprCubesNewP =
                                                (ccpReprCubesNewP != null) ? ccpReprCubesNewP.cornerOrientCubesAt(ccoReprNew) : null;
                                            CornerOrientReprCubes corientReprCubesNewC =
                                                ccpReprCubesNewC.cornerOrientCubesAt(ccoReprNew);
                                            List<cubeedges> ceNewArr = new ArrayList<>();
                                            for (long edges : corientReprCubesC.edgeList()) {
                                                cubeedges ce = new cubeedges(edges);
                                                cubeedges cenewRepr = CubecornerReprPerms.getComposedReprCubeedges(
                                                        ce, reversed != 0, otransformNew);
                                                if (corientReprCubesNewP != null && corientReprCubesNewP.containsCubeEdges(cenewRepr))
                                                    continue;
                                                if (corientReprCubesNewC.containsCubeEdges(cenewRepr))
                                                    continue;
                                                ceNewArr.add(cenewRepr);
                                            }
                                            if (!ceNewArr.isEmpty()) {
                                                CornerOrientReprCubes corientReprCubesNewN =
                                                    ccpReprCubesNewN.cornerOrientCubesAdd(ccoReprNew);
                                                cubeCount += corientReprCubesNewN.addCubes(ceNewArr);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return cubeCount;
        }

        public boolean searchMovesForReprPerm(int reprPermIdx, int depth, int depthMax,
                cube cSearchT, boolean reversed, cube[] c, cube[] cSearch) {
            List<EdgeReprCandidateTransform> otransform = new ArrayList<>();
            CornerPermReprCubes ccpReprCubes = m_cubesAtDepths.get(depth).getAt(reprPermIdx);
            cubecorners_perm ccp = m_reprPerms.getPermForIdx(reprPermIdx);
            if (!ccpReprCubes.empty()) {
                cubecorners_perm ccpSearch = reversed
                    ? cubecorners_perm.compose(cSearchT.ccp, ccp)
                    : cubecorners_perm.compose(ccp, cSearchT.ccp);
                CornerPermReprCubes ccpReprSearchCubes = m_cubesAtDepths.get(depthMax).getFor(ccpSearch);
                if (ccpReprCubes.size() <= ccpReprSearchCubes.size() || !m_reprPerms.isSingleTransform(ccpSearch)) {
                    for (CornerOrientReprCubes ccoReprCubes : ccpReprCubes.ccoCubesList()) {
                        cubecorner_orients cco = ccoReprCubes.getOrients();
                        cubecorner_orients ccoSearch = reversed
                            ? cubecorner_orients.compose(cSearchT.cco, ccp, cco)
                            : cubecorner_orients.compose(cco, cSearchT.ccp, cSearchT.cco);
                        cubecorner_orients ccoSearchRepr = m_reprPerms.getComposedReprOrients(
                                ccpSearch, ccoSearch, reversed, cSearchT.ce, otransform);
                        CornerOrientReprCubes ccoReprSearchCubes = ccpReprSearchCubes.cornerOrientCubesAt(ccoSearchRepr);
                        if (ccoReprSearchCubes.empty()) continue;
                        cubeedges ce = CornerOrientReprCubes.findSolutionEdge(
                                ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
                        if (!ce.isNil()) {
                            cubeedges ceSearch = reversed
                                ? cubeedges.compose(cSearchT.ce, ce)
                                : cubeedges.compose(ce, cSearchT.ce);
                            cSearch[0] = new cube(ccpSearch, ccoSearch, ceSearch);
                            c[0] = new cube(ccp, cco, ce);
                            return true;
                        }
                    }
                } else {
                    for (CornerOrientReprCubes ccoReprSearchCubes : ccpReprSearchCubes.ccoCubesList()) {
                        cubecorner_orients ccoSearchRepr = ccoReprSearchCubes.getOrients();
                        cubecorner_orients cco = m_reprPerms.getOrientsForComposedRepr(
                                ccpSearch, ccoSearchRepr, reversed, cSearchT, otransform);
                        CornerOrientReprCubes ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(cco);
                        if (ccoReprCubes.empty()) continue;
                        cubeedges ce = CornerOrientReprCubes.findSolutionEdge(
                                ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
                        if (!ce.isNil()) {
                            cubecorner_orients ccoSearch = reversed
                                ? cubecorner_orients.compose(cSearchT.cco, ccp, cco)
                                : cubecorner_orients.compose(cco, cSearchT.ccp, cSearchT.cco);
                            cubeedges ceSearch = reversed
                                ? cubeedges.compose(cSearchT.ce, ce)
                                : cubeedges.compose(ce, cSearchT.ce);
                            cSearch[0] = new cube(ccpSearch, ccoSearch, ceSearch);
                            c[0] = new cube(ccp, cco, ce);
                            return true;
                        }
                    }
                }
            }
            return false;
        }
    }
}

