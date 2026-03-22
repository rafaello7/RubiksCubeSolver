package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.ThreadPoolHelper.*;

public class SearchOptimal {
    private static String getMovesForMatch(CubesReprByDepth cubesReprByDepth,
                                           Cube cSearch, Cube c, int searchRev, int searchTd,
                                           int reversed, int symmetric, int td) {
        Cube cSearchT = cSearch.transform(transformReverse(td));
        Cube cSearchTsymm = (symmetric != 0) ? cSearchT.symmetric() : cSearchT;
        Cube cT = c.transform(transformReverse(td));
        Cube cTsymm = (symmetric != 0) ? cT.symmetric() : cT;
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

    private static void generateSearchTarr(Cube csearch, boolean useReverse,
                                           Cube[][][] cSearchTarr) {
        for (int rev = 0; rev < (useReverse ? 2 : 1); rev++) {
            Cube csearchrev = (rev != 0) ? csearch.reverse() : csearch;
            for (int sym = 0; sym <= 1; sym++) {
                Cube csearchrevsymm = (sym != 0) ? csearchrev.symmetric() : csearchrev;
                for (int td = 0; td < TransformDir.TCOUNT.ordinal(); td++)
                    cSearchTarr[rev][sym][td] = csearchrevsymm.transform(td);
            }
        }
    }

    private static boolean searchMovesForIdxs(CubesReprByDepth cubesReprByDepth,
                                              int depth, int depthMax, Cube[][][] cSearchTarr,
                                              OptimalSearchIndexes indexes, String[] moves, int searchRev, int searchTd) {
        Cube cSearchT = cSearchTarr[indexes.reversed][indexes.symmetric][indexes.td];
        Cube[] c = {new Cube()};
        Cube[] cSearch = {new Cube()};
        if (cubesReprByDepth.searchMovesForReprPerm(indexes.permReprIdx,
                depth, depthMax, cSearchT, indexes.reversed != 0, c, cSearch)) {
            moves[0] = getMovesForMatch(cubesReprByDepth, cSearch[0], c[0],
                    searchRev, searchTd, indexes.reversed, indexes.symmetric, indexes.td);
            return true;
        }
        return false;
    }

    private static void searchMovesTa(int threadNo, CubesReprByDepth cubesReprByDepth,
            int depth, int depthMax, Cube[][][] cSearchTarr,
            Responder responder, OptimalSearchProgress optimalSearchProgress) {
        OptimalSearchIndexes indexes = new OptimalSearchIndexes();
        while (optimalSearchProgress.inc(responder, indexes)) {
            String[] moves = {""};
            if (searchMovesForIdxs(cubesReprByDepth, depth, depthMax, cSearchTarr, indexes, moves, 0, 0)) {
                responder.solution(moves[0]);
                optimalSearchProgress.inc(responder, null);
                return;
            }
        }
    }

    private static void searchMovesTb(int threadNo, CubesReprByDepth cubesReprByDepth,
            int depth, int depthMax, Cube csearch,
            Responder responder, OptimalSearchProgress optimalSearchProgress) {
        CubesReprAtDepth ccReprCubesC = cubesReprByDepth.getAt(depth);
        java.util.List<java.util.AbstractMap.SimpleEntry<CubecornersPerm, CornerPermReprCubes>> ccp1FilledIters
            = new java.util.ArrayList<>();
        CornerPermReprCubes[] list = ccReprCubesC.ccpCubesList();
        for (int idx = 0; idx < list.length; idx++) {
            CornerPermReprCubes ccpCubes1 = list[idx];
            if (!ccpCubes1.empty()) {
                CubecornersPerm ccp = ccReprCubesC.getPermAt(idx);
                ccp1FilledIters.add(new java.util.AbstractMap.SimpleEntry<>(ccp, ccpCubes1));
            }
        }
        OptimalSearchIndexes indexes2 = new OptimalSearchIndexes();
        while (optimalSearchProgress.inc(responder, indexes2)) {
            CornerPermReprCubes ccpReprCubes2 = cubesReprByDepth.getAt(depthMax).getAt(indexes2.permReprIdx);
            if (ccpReprCubes2.empty()) continue;
            for (var pair : ccp1FilledIters) {
                CubecornersPerm ccp1 = pair.getKey();
                CornerPermReprCubes ccpCubes1 = pair.getValue();
                for (CornerOrientReprCubes ccoCubes1 : ccpCubes1.ccoCubesList()) {
                    CubecornerOrients cco1 = ccoCubes1.getOrients();
                    for (long edges1 : ccoCubes1.edgeList()) {
                        CubeEdges ce1 = new CubeEdges(edges1);
                        Cube c1 = new Cube(ccp1, cco1, ce1);
                        java.util.Set<Cube> cubesChecked = new java.util.HashSet<>();
                        for (int rev1 = 0; rev1 < (cubesReprByDepth.isUseReverse() ? 2 : 1); rev1++) {
                            Cube c1r = (rev1 != 0) ? c1.reverse() : c1;
                            for (int sym1 = 0; sym1 <= 1; sym1++) {
                                Cube c1rs = (sym1 != 0) ? c1r.symmetric() : c1r;
                                for (int td1 = 0; td1 < TransformDir.TCOUNT.ordinal(); td1++) {
                                    Cube c1T = c1rs.transform(td1);
                                    if (!cubesChecked.add(c1T)) continue;
                                    Cube cSearch1 = Cube.compose(c1T, csearch);
                                    Cube[][][] cSearchTarr = new Cube[2][2][TransformDir.TCOUNT.ordinal()];
                                    for (Cube[][] a : cSearchTarr) for (Cube[] b : a) java.util.Arrays.fill(b, new Cube());
                                    generateSearchTarr(cSearch1, cubesReprByDepth.isUseReverse(), cSearchTarr);
                                    String[] moves2 = {""};
                                    if (searchMovesForIdxs(cubesReprByDepth, depthMax, depthMax,
                                            cSearchTarr, indexes2, moves2, 0, 0)) {
                                        String moves = moves2[0] + cubesReprByDepth.getMoves(c1T);
                                        responder.solution(moves);
                                        optimalSearchProgress.inc(responder, null);
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
                                               Cube csearch, int depthSearch, Responder responder) {
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
        Cube[][][] cSearchTarr = new Cube[2][2][TransformDir.TCOUNT.ordinal()];
        for (Cube[][] a : cSearchTarr) for (Cube[] b : a) java.util.Arrays.fill(b, new Cube());
        generateSearchTarr(csearch, cubesReprByDepth.isUseReverse(), cSearchTarr);
        OptimalSearchProgress optimalSearchProgress = new OptimalSearchProgress(depthSearch, cubesReprByDepth.isUseReverse());
        final CubesReprByDepth crbd = cubesReprByDepth;
        final int d = depth, dm = depthMax;
        runInThreadPool(threadNo -> searchMovesTa(threadNo, crbd, d, dm, cSearchTarr, responder, optimalSearchProgress));
        if (optimalSearchProgress.isFinish()) {
            responder.movecount(String.valueOf(depthSearch));
            responder.message("finished at " + optimalSearchProgress.progressStr());
            return true;
        }
        boolean isStopRequested = ProgressBase.isStopRequested();
        if (isStopRequested) responder.message("canceled");
        else responder.message("depth " + depthSearch + " end");
        return isStopRequested;
    }

    private static boolean searchMovesOptimalB(CubesReprByDepthAdd cubesReprByDepthAdd,
                                               Cube csearch, int depth, int depthMax, Responder responder) {
        CubesReprByDepth cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depthMax, responder);
        if (cubesReprByDepth == null) return true;
        OptimalSearchProgress optimalSearchProgress = new OptimalSearchProgress(2*depthMax+depth, cubesReprByDepth.isUseReverse());
        final CubesReprByDepth crbd = cubesReprByDepth;
        runInThreadPool(threadNo -> searchMovesTb(threadNo, crbd, depth, depthMax, csearch, responder, optimalSearchProgress));
        if (optimalSearchProgress.isFinish()) {
            responder.movecount(String.valueOf(2*depthMax+depth));
            responder.message("finished at " + optimalSearchProgress.progressStr());
            return true;
        }
        boolean isStopRequested = ProgressBase.isStopRequested();
        if (isStopRequested) responder.message("canceled");
        else responder.message("depth " + (2*depthMax+depth) + " end");
        return isStopRequested;
    }

    public static void searchMovesOptimal(CubesReprByDepthAdd cubesReprByDepthAdd,
                                          Cube csearch, int depthMax, Responder responder) {
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

