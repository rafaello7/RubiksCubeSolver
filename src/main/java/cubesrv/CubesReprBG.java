package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.CPermReprBG.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class CubesReprBG {

    public static final int TWOPHASE_DEPTH2_MAX = 8;

    public static class BGCornerPermReprCubes {
        public long[] m_items = new long[0];

        public int addCubes(Collection<cubeedges> cearr) {
            List<Long> edgeList = new ArrayList<>();
            List<Integer> idxList = new ArrayList<>();
            for (cubeedges ce : cearr) {
                long edge = ce.get();
                if (!edgeList.contains(edge)) {
                    int idx = Arrays.binarySearch(m_items, edge);
                    if (idx < 0) {
                        edgeList.add(edge);
                        idxList.add(-1 - idx);
                    }
                }
            }
            idxList.sort(null);
            edgeList.sort(null);
            long[] itemsNew = new long[m_items.length + edgeList.size()];
            int srcPos = 0, idxsPos = 0, destPos = 0;
            while (destPos < itemsNew.length) {
                if (idxsPos < idxList.size() && idxList.get(idxsPos) == srcPos) {
                    itemsNew[destPos] = edgeList.get(idxsPos);
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

        public boolean containsCubeEdges(cubeedges ce) {
            return Arrays.binarySearch(m_items, ce.get()) >= 0;
        }

        public boolean empty() { return m_items.length == 0; }
        public int size() { return m_items.length; }
        public long[] edgeList() { return m_items; }
    }

    public static class BGCubesReprAtDepth {
        private final BGCubecornerReprPerms m_reprPerms;
        final BGCornerPermReprCubes[] m_cornerPermReprCubes;

        public BGCubesReprAtDepth(BGCubecornerReprPerms reprPerms) {
            this.m_reprPerms = reprPerms;
            m_cornerPermReprCubes = new BGCornerPermReprCubes[reprPerms.reprPermCount()];
            for (int i = 0; i < m_cornerPermReprCubes.length; i++)
                m_cornerPermReprCubes[i] = new BGCornerPermReprCubes();
        }

        public int size() { return m_cornerPermReprCubes.length; }

        public int cubeCount() {
            int res = 0;
            for (BGCornerPermReprCubes c : m_cornerPermReprCubes) res += c.size();
            return res;
        }

        public BGCornerPermReprCubes add(int idx) { return m_cornerPermReprCubes[idx]; }
        public BGCornerPermReprCubes getAt(int idx) { return m_cornerPermReprCubes[idx]; }
        public BGCornerPermReprCubes[] ccpCubesList() { return m_cornerPermReprCubes; }

        public cubecorners_perm getPermAt(int reprPermIdx) {
            return m_reprPerms.getPermForIdx(reprPermIdx);
        }

        public boolean containsCube(cube c) {
            int ccpReprSearchIdx = m_reprPerms.getReprPermIdx(c.ccp);
            BGCornerPermReprCubes ccpReprSearchCubes = m_cornerPermReprCubes[ccpReprSearchIdx];
            cubeedges ceSearchRepr = m_reprPerms.getReprCubeedges(c.ccp, c.ce);
            return ccpReprSearchCubes.containsCubeEdges(ceSearchRepr);
        }
    }

    public static class BGCubesReprByDepth {
        public final BGCubecornerReprPerms m_reprPerms;
        private final List<BGCubesReprAtDepth> m_cubesAtDepths = new ArrayList<>();
        private int m_availCount = 1;

        public BGCubesReprByDepth(boolean useReverse) {
            this.m_reprPerms = new BGCubecornerReprPerms(useReverse);
            m_cubesAtDepths.add(new BGCubesReprAtDepth(m_reprPerms));
            int cornerPermReprIdx = m_reprPerms.getReprPermIdx(csolved.ccp);
            BGCornerPermReprCubes ccpCubes = m_cubesAtDepths.get(0).add(cornerPermReprIdx);
            cubeedges ceRepr = m_reprPerms.getReprCubeedges(csolved.ccp, csolved.ce);
            ccpCubes.addCubes(java.util.Collections.singletonList(ceRepr));
        }

        public boolean isUseReverse() { return m_reprPerms.isUseReverse(); }
        public int availCount() { return m_availCount; }
        public void incAvailCount() { ++m_availCount; }

        public BGCubesReprAtDepth getAt(int idx) {
            if (idx > TWOPHASE_DEPTH2_MAX)
                System.out.println("BGCubesReprByDepth.getAt fatal: idx=" + idx + " exceeds TWOPHASE_DEPTH2_MAX=" + TWOPHASE_DEPTH2_MAX);
            while (idx >= m_cubesAtDepths.size())
                m_cubesAtDepths.add(new BGCubesReprAtDepth(m_reprPerms));
            return m_cubesAtDepths.get(idx);
        }

        public String getMoves(cube c, int searchTd, boolean movesRevp) {
            boolean movesRev = movesRevp;
            int[] reverseMoveIdxs = {0, 1, 2, 3, 6, 5, 4, 9, 8, 7};
            rotate_dir[][] transformedMoves = {
                { rotate_dir.BLUE180, rotate_dir.GREEN180, rotate_dir.ORANGE180, rotate_dir.RED180,
                  rotate_dir.WHITECW, rotate_dir.WHITE180, rotate_dir.WHITECCW, rotate_dir.YELLOWCW,
                  rotate_dir.YELLOW180, rotate_dir.YELLOWCCW },
                { rotate_dir.YELLOW180, rotate_dir.WHITE180, rotate_dir.BLUE180, rotate_dir.GREEN180,
                  rotate_dir.REDCW, rotate_dir.RED180, rotate_dir.REDCCW, rotate_dir.ORANGECW,
                  rotate_dir.ORANGE180, rotate_dir.ORANGECCW }
            };
            List<Integer> rotateIdxs = new ArrayList<>();
            int insertPos = 0;
            cube crepr = m_reprPerms.cubeRepresentative(c);
            int ccpReprIdx = m_reprPerms.getReprPermIdx(crepr.ccp);
            int depth = 0;
            while (true) {
                BGCornerPermReprCubes ccpReprCubes = m_cubesAtDepths.get(depth).getAt(ccpReprIdx);
                if (ccpReprCubes.containsCubeEdges(crepr.ce)) break;
                ++depth;
                if (depth >= m_availCount) {
                    System.out.println("bg getMoves: depth reached maximum, cube NOT FOUND");
                    System.exit(1);
                }
            }
            cube cc = c;
            while (depth-- > 0) {
                int cmidx = 0;
                cube ccRev = cc.reverse();
                cube cc1 = new cube();
                while (cmidx < RCOUNTBG) {
                    int cm = BGSpaceRotations[cmidx].ordinal();
                    cc1 = cube.compose(cc, crotated[cm]);
                    cube cc1repr = m_reprPerms.cubeRepresentative(cc1);
                    ccpReprIdx = m_reprPerms.getReprPermIdx(cc1repr.ccp);
                    BGCornerPermReprCubes ccpReprCubes = m_cubesAtDepths.get(depth).getAt(ccpReprIdx);
                    if (ccpReprCubes.containsCubeEdges(cc1repr.ce)) break;
                    cc1 = cube.compose(ccRev, crotated[cm]);
                    cc1repr = m_reprPerms.cubeRepresentative(cc1);
                    ccpReprIdx = m_reprPerms.getReprPermIdx(cc1repr.ccp);
                    BGCornerPermReprCubes ccpReprCubesRev = m_cubesAtDepths.get(depth).getAt(ccpReprIdx);
                    if (ccpReprCubesRev.containsCubeEdges(cc1repr.ce)) {
                        movesRev = !movesRev;
                        break;
                    }
                    ++cmidx;
                }
                if (cmidx == RCOUNTBG) {
                    System.out.println("bg getMoves: cube at depth " + depth + " NOT FOUND");
                    System.exit(1);
                }
                rotateIdxs.add(insertPos, movesRev ? cmidx : reverseMoveIdxs[cmidx]);
                if (movesRev) ++insertPos;
                cc = cc1;
            }
            StringBuilder res = new StringBuilder();
            for (int rotateIdx : rotateIdxs) {
                int rotateDir = (searchTd != 0)
                    ? transformedMoves[searchTd-1][rotateIdx].ordinal()
                    : BGSpaceRotations[rotateIdx].ordinal();
                res.append(' ');
                res.append(rotateDirName(rotateDir));
            }
            return res.toString();
        }

        public String getMoves(cube c, int searchTd) { return getMoves(c, searchTd, false); }

        public int addCubesForReprPerm(int reprPermIdx, int depth) {
            int cubeCount = 0;
            BGCubesReprAtDepth ccpReprCubesC = m_cubesAtDepths.get(depth-1);
            BGCornerPermReprCubes ccpReprCubesNewP = (depth == 1) ? null : m_cubesAtDepths.get(depth-2).getAt(reprPermIdx);
            BGCornerPermReprCubes ccpReprCubesNewC = ccpReprCubesC.getAt(reprPermIdx);
            BGCornerPermReprCubes ccpReprCubesNewN = m_cubesAtDepths.get(depth).add(reprPermIdx);
            cubecorners_perm ccpNewRepr = m_reprPerms.getPermForIdx(reprPermIdx);
            Set<cubecorners_perm> ccpChecked = new HashSet<>();
            for (int trrev = 0; trrev < (m_reprPerms.isUseReverse() ? 2 : 1); trrev++) {
                cubecorners_perm ccpNewReprRev = (trrev != 0) ? ccpNewRepr.reverse() : ccpNewRepr;
                for (int symmetric = 0; symmetric <= 1; symmetric++) {
                    cubecorners_perm ccpNewS = (symmetric != 0) ? ccpNewReprRev.symmetric() : ccpNewReprRev;
                    for (int tdidx = 0; tdidx < TCOUNTBG; tdidx++) {
                        int td = BGSpaceTransforms[tdidx].ordinal();
                        cubecorners_perm ccpNew = ccpNewS.transform(td);
                        if (!ccpChecked.contains(ccpNew)) {
                            ccpChecked.add(ccpNew);
                            for (int rdidx = 0; rdidx < RCOUNTBG; rdidx++) {
                                int rd    = BGSpaceRotations[rdidx].ordinal();
                                int rdRev = rotateDirReverse(rd);
                                for (int reversed = 0; reversed < (m_reprPerms.isUseReverse() ? 2 : 1); reversed++) {
                                    cubecorners_perm ccp = (reversed != 0)
                                        ? cubecorners_perm.compose(crotated[rdRev].ccp, ccpNew)
                                        : cubecorners_perm.compose(ccpNew, crotated[rdRev].ccp);
                                    int ccpReprIdx = m_reprPerms.getReprPermIdx(ccp);
                                    if (m_reprPerms.getPermForIdx(ccpReprIdx).equals(ccp)) {
                                        BGCornerPermReprCubes cpermReprCubesC = ccpReprCubesC.getAt(ccpReprIdx);
                                        List<cubeedges> ceNewArr = new ArrayList<>();
                                        for (long edges : cpermReprCubesC.edgeList()) {
                                            cubeedges ce = new cubeedges(edges);
                                            cubeedges cenew = (reversed != 0)
                                                ? cubeedges.compose(crotated[rd].ce, ce)
                                                : cubeedges.compose(ce, crotated[rd].ce);
                                            cubeedges cenewRepr = m_reprPerms.getReprCubeedges(ccpNew, cenew);
                                            if (ccpReprCubesNewP != null && ccpReprCubesNewP.containsCubeEdges(cenewRepr))
                                                continue;
                                            if (ccpReprCubesNewC.containsCubeEdges(cenewRepr))
                                                continue;
                                            ceNewArr.add(cenewRepr);
                                        }
                                        if (!ceNewArr.isEmpty())
                                            cubeCount += ccpReprCubesNewN.addCubes(ceNewArr);
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
            BGCornerPermReprCubes ccpReprCubes = m_cubesAtDepths.get(depth).getAt(reprPermIdx);
            cubecorners_perm ccp = m_reprPerms.getPermForIdx(reprPermIdx);
            if (!ccpReprCubes.empty()) {
                cubecorners_perm ccpSearch = reversed
                    ? cubecorners_perm.compose(cSearchT.ccp, ccp)
                    : cubecorners_perm.compose(ccp, cSearchT.ccp);
                int ccpReprSearchIdx = m_reprPerms.getReprPermIdx(ccpSearch);
                BGCornerPermReprCubes ccpReprSearchCubes = m_cubesAtDepths.get(depthMax).getAt(ccpReprSearchIdx);
                if (ccpReprCubes.size() <= ccpReprSearchCubes.size() || !m_reprPerms.isSingleTransform(ccpSearch)) {
                    for (long edges : ccpReprCubes.edgeList()) {
                        cubeedges ce = new cubeedges(edges);
                        cubeedges ceSearch = reversed
                            ? cubeedges.compose(cSearchT.ce, ce)
                            : cubeedges.compose(ce, cSearchT.ce);
                        cubeedges ceSearchRepr = m_reprPerms.getReprCubeedges(ccpSearch, ceSearch);
                        if (ccpReprSearchCubes.containsCubeEdges(ceSearchRepr)) {
                            cSearch[0] = new cube(ccpSearch, csolved.cco, ceSearch);
                            c[0] = new cube(ccp, csolved.cco, ce);
                            return true;
                        }
                    }
                } else {
                    cube cSearchTrev = cSearchT.reverse();
                    for (long edges : ccpReprSearchCubes.edgeList()) {
                        cubeedges ceSearchRepr = new cubeedges(edges);
                        cubeedges ceSearch = m_reprPerms.getCubeedgesForRepresentative(ccpSearch, ceSearchRepr);
                        cubeedges ce = reversed
                            ? cubeedges.compose(cSearchTrev.ce, ceSearch)
                            : cubeedges.compose(ceSearch, cSearchTrev.ce);
                        if (ccpReprCubes.containsCubeEdges(ce)) {
                            cSearch[0] = new cube(ccpSearch, csolved.cco, ceSearch);
                            c[0] = new cube(ccp, csolved.cco, ce);
                            return true;
                        }
                    }
                }
            }
            return false;
        }
    }
}

