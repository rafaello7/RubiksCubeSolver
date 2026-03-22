package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.CubesRepr.*;
import static cubesrv.CubesAdd.*;
import static cubesrv.ThreadPoolHelper.*;

public class SearchOptimal {

    private static class SearchIndexes {
        int permReprIdx = 0;
        int reversed    = 0;
        int symmetric   = 0;
        int td          = 0;
    }

    private static class SearchProgress extends ProgressBase {
        private final int m_depth;
        private final boolean m_useReverse;
        private final int m_itemCount;
        int m_nextItemIdx      = 0;
        int m_runningThreadCount = THREAD_COUNT;
        boolean m_isFinish     = false;

        SearchProgress(int depth, boolean useReverse) {
            m_depth = depth;
            m_useReverse = useReverse;
            m_itemCount = (useReverse ? 2 * 654 : 984) * 2 * transform_dir.TCOUNT.ordinal();
        }

        boolean isFinish() { return m_isFinish; }

        boolean inc(Responder responder, SearchIndexes indexesBuf) {
            boolean res;
            int itemIdx = -1;
            mutexLock();
            if (indexesBuf == null) m_isFinish = true;
            res = !m_isFinish && m_nextItemIdx < m_itemCount && !isStopRequested();
            if (res)
                itemIdx = m_nextItemIdx++;
            else
                --m_runningThreadCount;
            mutexUnlock();
            if (res && indexesBuf != null) {
                indexesBuf.td = itemIdx % transform_dir.TCOUNT.ordinal();
                int itemIdxDiv = itemIdx / transform_dir.TCOUNT.ordinal();
                indexesBuf.symmetric = itemIdxDiv & 1;
                itemIdxDiv = itemIdxDiv >>> 1;
                if (m_useReverse) {
                    indexesBuf.reversed = itemIdxDiv & 1;
                    itemIdxDiv = itemIdxDiv >>> 1;
                } else {
                    indexesBuf.reversed = 0;
                }
                indexesBuf.permReprIdx = itemIdxDiv;
                if (m_depth >= 17) {
                    int procCountNext = 100 * (itemIdx+1) / m_itemCount;
                    int procCountCur  = 100 * itemIdx / m_itemCount;
                    if (procCountNext != procCountCur && (m_depth >= 18 || procCountCur % 10 == 0))
                        responder.progress("depth " + m_depth + " search " + (100 * itemIdx / m_itemCount) + "%");
                }
            } else {
                if (m_depth >= 17)
                    responder.progress("depth " + m_depth + " search " + m_runningThreadCount + " threads still running");
            }
            return res;
        }

        String progressStr() {
            return (100 * m_nextItemIdx / m_itemCount) + "%";
        }
    }

    private static String getMovesForMatch(CubesReprByDepth cubesReprByDepth,
            cube cSearch, cube c, int searchRev, int searchTd,
            int reversed, int symmetric, int td) {
        cube cSearchT = cSearch.transform(transformReverse(td));
        cube cSearchTsymm = (symmetric != 0) ? cSearchT.symmetric() : cSearchT;
        cube cT = c.transform(transformReverse(td));
        cube cTsymm = (symmetric != 0) ? cT.symmetric() : cT;
        if (searchTd != 0) {
            cSearchTsymm = cSearchTsymm.transform(transformReverse(searchTd));
            cTsymm       = cTsymm.transform(transformReverse(searchTd));
        }
        String moves;
        if (searchRev != 0) {
            moves  = cubesReprByDepth.getMoves(cTsymm, reversed == 0);
            moves += cubesReprByDepth.getMoves(cSearchTsymm, reversed != 0);
        } else {
            moves  = cubesReprByDepth.getMoves(cSearchTsymm, reversed == 0);
            moves += cubesReprByDepth.getMoves(cTsymm, reversed != 0);
        }
        return moves;
    }

    private static void generateSearchTarr(cube csearch, boolean useReverse,
            cube[][][] cSearchTarr) {
        for (int rev = 0; rev < (useReverse ? 2 : 1); rev++) {
            cube csearchrev = (rev != 0) ? csearch.reverse() : csearch;
            for (int sym = 0; sym <= 1; sym++) {
                cube csearchrevsymm = (sym != 0) ? csearchrev.symmetric() : csearchrev;
                for (int td = 0; td < transform_dir.TCOUNT.ordinal(); td++)
                    cSearchTarr[rev][sym][td] = csearchrevsymm.transform(td);
            }
        }
    }

    private static boolean searchMovesForIdxs(CubesReprByDepth cubesReprByDepth,
            int depth, int depthMax, cube[][][] cSearchTarr,
            SearchIndexes indexes, String[] moves, int searchRev, int searchTd) {
        cube cSearchT = cSearchTarr[indexes.reversed][indexes.symmetric][indexes.td];
        cube[] c = {new cube()};
        cube[] cSearch = {new cube()};
        if (cubesReprByDepth.searchMovesForReprPerm(indexes.permReprIdx,
                depth, depthMax, cSearchT, indexes.reversed != 0, c, cSearch)) {
            moves[0] = getMovesForMatch(cubesReprByDepth, cSearch[0], c[0],
                    searchRev, searchTd, indexes.reversed, indexes.symmetric, indexes.td);
            return true;
        }
        return false;
    }

    private static void searchMovesTa(int threadNo, CubesReprByDepth cubesReprByDepth,
            int depth, int depthMax, cube[][][] cSearchTarr,
            Responder responder, SearchProgress searchProgress) {
        SearchIndexes indexes = new SearchIndexes();
        while (searchProgress.inc(responder, indexes)) {
            String[] moves = {""};
            if (searchMovesForIdxs(cubesReprByDepth, depth, depthMax, cSearchTarr, indexes, moves, 0, 0)) {
                responder.solution(moves[0]);
                searchProgress.inc(responder, null);
                return;
            }
        }
    }

    private static void searchMovesTb(int threadNo, CubesReprByDepth cubesReprByDepth,
            int depth, int depthMax, cube csearch,
            Responder responder, SearchProgress searchProgress) {
        CubesReprAtDepth ccReprCubesC = cubesReprByDepth.getAt(depth);
        java.util.List<java.util.AbstractMap.SimpleEntry<cubecorners_perm, CornerPermReprCubes>> ccp1FilledIters
            = new java.util.ArrayList<>();
        CornerPermReprCubes[] list = ccReprCubesC.ccpCubesList();
        for (int idx = 0; idx < list.length; idx++) {
            CornerPermReprCubes ccpCubes1 = list[idx];
            if (!ccpCubes1.empty()) {
                cubecorners_perm ccp = ccReprCubesC.getPermAt(idx);
                ccp1FilledIters.add(new java.util.AbstractMap.SimpleEntry<>(ccp, ccpCubes1));
            }
        }
        SearchIndexes indexes2 = new SearchIndexes();
        while (searchProgress.inc(responder, indexes2)) {
            CornerPermReprCubes ccpReprCubes2 = cubesReprByDepth.getAt(depthMax).getAt(indexes2.permReprIdx);
            if (ccpReprCubes2.empty()) continue;
            for (var pair : ccp1FilledIters) {
                cubecorners_perm ccp1 = pair.getKey();
                CornerPermReprCubes ccpCubes1 = pair.getValue();
                for (CornerOrientReprCubes ccoCubes1 : ccpCubes1.ccoCubesList()) {
                    cubecorner_orients cco1 = ccoCubes1.getOrients();
                    for (long edges1 : ccoCubes1.edgeList()) {
                        cubeedges ce1 = new cubeedges(edges1);
                        cube c1 = new cube(ccp1, cco1, ce1);
                        java.util.Set<cube> cubesChecked = new java.util.HashSet<>();
                        for (int rev1 = 0; rev1 < (cubesReprByDepth.isUseReverse() ? 2 : 1); rev1++) {
                            cube c1r = (rev1 != 0) ? c1.reverse() : c1;
                            for (int sym1 = 0; sym1 <= 1; sym1++) {
                                cube c1rs = (sym1 != 0) ? c1r.symmetric() : c1r;
                                for (int td1 = 0; td1 < transform_dir.TCOUNT.ordinal(); td1++) {
                                    cube c1T = c1rs.transform(td1);
                                    if (!cubesChecked.add(c1T)) continue;
                                    cube cSearch1 = cube.compose(c1T, csearch);
                                    cube[][][] cSearchTarr = new cube[2][2][transform_dir.TCOUNT.ordinal()];
                                    for (cube[][] a : cSearchTarr) for (cube[] b : a) java.util.Arrays.fill(b, new cube());
                                    generateSearchTarr(cSearch1, cubesReprByDepth.isUseReverse(), cSearchTarr);
                                    String[] moves2 = {""};
                                    if (searchMovesForIdxs(cubesReprByDepth, depthMax, depthMax,
                                            cSearchTarr, indexes2, moves2, 0, 0)) {
                                        String moves = moves2[0] + cubesReprByDepth.getMoves(c1T);
                                        responder.solution(moves);
                                        searchProgress.inc(responder, null);
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    private static boolean searchMovesOptimalA(CubesReprByDepthAdd cubesReprByDepthAdd,
            cube csearch, int depthSearch, Responder responder) {
        CubesReprByDepth cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(0, responder);
        if (cubesReprByDepth == null) return true;
        int depthsAvail = cubesReprByDepth.availCount() - 1;
        int depth, depthMax;
        if (depthSearch <= depthsAvail) {
            depth = 0; depthMax = depthSearch;
        } else if (depthSearch <= 2 * depthsAvail) {
            depth = depthSearch - depthsAvail; depthMax = depthsAvail;
        } else {
            depth = depthSearch / 2; depthMax = depthSearch - depth;
            cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depthMax, responder);
            if (cubesReprByDepth == null) return true;
        }
        cube[][][] cSearchTarr = new cube[2][2][transform_dir.TCOUNT.ordinal()];
        for (cube[][] a : cSearchTarr) for (cube[] b : a) java.util.Arrays.fill(b, new cube());
        generateSearchTarr(csearch, cubesReprByDepth.isUseReverse(), cSearchTarr);
        SearchProgress searchProgress = new SearchProgress(depthSearch, cubesReprByDepth.isUseReverse());
        final CubesReprByDepth crbd = cubesReprByDepth;
        final int d = depth, dm = depthMax;
        runInThreadPool(threadNo -> searchMovesTa(threadNo, crbd, d, dm, cSearchTarr, responder, searchProgress));
        if (searchProgress.isFinish()) {
            responder.movecount(String.valueOf(depthSearch));
            responder.message("finished at " + searchProgress.progressStr());
            return true;
        }
        boolean isStopRequested = ProgressBase.isStopRequested();
        if (isStopRequested) responder.message("canceled");
        else responder.message("depth " + depthSearch + " end");
        return isStopRequested;
    }

    private static boolean searchMovesOptimalB(CubesReprByDepthAdd cubesReprByDepthAdd,
            cube csearch, int depth, int depthMax, Responder responder) {
        CubesReprByDepth cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depthMax, responder);
        if (cubesReprByDepth == null) return true;
        SearchProgress searchProgress = new SearchProgress(2*depthMax+depth, cubesReprByDepth.isUseReverse());
        final CubesReprByDepth crbd = cubesReprByDepth;
        runInThreadPool(threadNo -> searchMovesTb(threadNo, crbd, depth, depthMax, csearch, responder, searchProgress));
        if (searchProgress.isFinish()) {
            responder.movecount(String.valueOf(2*depthMax+depth));
            responder.message("finished at " + searchProgress.progressStr());
            return true;
        }
        boolean isStopRequested = ProgressBase.isStopRequested();
        if (isStopRequested) responder.message("canceled");
        else responder.message("depth " + (2*depthMax+depth) + " end");
        return isStopRequested;
    }

    public static void searchMovesOptimal(CubesReprByDepthAdd cubesReprByDepthAdd,
            cube csearch, int depthMax, Responder responder) {
        for (int depthSearch = 0; depthSearch <= 2*depthMax; depthSearch++) {
            if (searchMovesOptimalA(cubesReprByDepthAdd, csearch, depthSearch, responder))
                return;
        }
        for (int depth = 1; depth <= depthMax; depth++) {
            if (searchMovesOptimalB(cubesReprByDepthAdd, csearch, depth, depthMax, responder))
                return;
        }
        responder.message("not found");
    }
}

