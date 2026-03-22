package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.CPermReprBG.*;
import static cubesrv.CubesRepr.*;
import static cubesrv.CubesAdd.*;
import static cubesrv.CubesAddBG.*;
import cubesrv.CubeCosets;
import cubesrv.CubeCosets.CubeCosetsAtDepth;
import cubesrv.CubeCosetsAdd;
import static cubesrv.CubeCosetsAdd.TWOPHASE_DEPTH1_CATCHFIRST_MAX;
import static cubesrv.CubeCosetsAdd.TWOPHASE_DEPTH1_MULTI_MAX;
import static cubesrv.SearchBG.*;
import static cubesrv.ThreadPoolHelper.*;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class SearchQuick {

    public static final int TWOPHASE_SEARCHREV = 2;

    // Pair of cube + moves string
    private static class CubeWithMoves {
        final cube c;
        final String moves;
        CubeWithMoves(cube c, String moves) { this.c = c; this.moves = moves; }
    }

    private static int searchPhase1Cube2(CubesReprByDepth cubesReprByDepth,
            BGCubesReprByDepthAdd bgcubesReprByDepthAdd,
            cube cSearchMid, List<cube> cubes,
            int searchRev, int searchTd, int cube2Depth, int movesMaxp,
            boolean catchFirst, Responder responder, String[] moves) {
        int bestMoveCount = -1;
        int movesMax = movesMaxp;
        for (cube cube2 : cubes) {
            cube cSpace = cube.compose(cube2.reverse(), cSearchMid);
            String[] movesInSpace = {""};
            int depthInSpace = searchInSpaceMoves(bgcubesReprByDepthAdd, cSpace,
                    searchRev != 0, searchTd, movesMax - cube2Depth, responder, movesInSpace);
            if (depthInSpace >= 0) {
                cube cube2T = cube2.transform(transformReverse(searchTd));
                String cube2Moves = cubesReprByDepth.getMoves(cube2T, searchRev == 0);
                if (searchRev != 0)
                    moves[0] = cube2Moves + movesInSpace[0];
                else
                    moves[0] = movesInSpace[0] + cube2Moves;
                bestMoveCount = cube2Depth + depthInSpace;
                if (catchFirst || depthInSpace == 0) return bestMoveCount;
                movesMax = bestMoveCount - 1;
            }
        }
        return bestMoveCount;
    }

    private static class QuickSearchProgress extends ProgressBase {
        private final int m_itemCount;
        private final int m_depthSearch;
        private final boolean m_catchFirst;
        private int m_movesMax;
        int m_nextItemIdx    = 0;
        int m_bestMoveCount  = -1;
        String m_bestMoves   = "";

        QuickSearchProgress(int itemCount, int depthSearch, boolean catchFirst, int movesMax) {
            m_itemCount   = itemCount;
            m_depthSearch = depthSearch;
            m_catchFirst  = catchFirst;
            m_movesMax    = movesMax;
        }

        boolean isCatchFirst() { return m_catchFirst; }

        int inc(Responder responder, int[] itemIdxBuf) {
            int movesMax;
            int itemIdx = -1;
            mutexLock();
            if (m_movesMax >= 0 && m_nextItemIdx < m_itemCount && !isStopRequested()) {
                itemIdx = m_nextItemIdx++;
                movesMax = m_movesMax;
            } else {
                movesMax = -1;
            }
            mutexUnlock();
            if (movesMax >= 0) {
                itemIdxBuf[0] = itemIdx;
                if (m_depthSearch >= 9) {
                    int procCountNext = 100 * (itemIdx+1) / m_itemCount;
                    int procCountCur  = 100 * itemIdx / m_itemCount;
                    if (procCountNext != procCountCur && (m_depthSearch >= 10 || procCountCur % 10 == 0))
                        responder.progress("depth " + m_depthSearch + " search " + (100 * itemIdx / m_itemCount) + "%");
                }
            }
            return movesMax;
        }

        int setBestMoves(String moves, int moveCount, Responder responder) {
            int movesMax;
            mutexLock();
            if (m_bestMoveCount < 0 || moveCount < m_bestMoveCount) {
                m_bestMoves     = moves;
                m_bestMoveCount = moveCount;
                m_movesMax      = m_catchFirst ? -1 : moveCount - 1;
                responder.movecount(moveCount + " at depth: " + String.format("%2d", m_depthSearch));
            }
            movesMax = m_movesMax;
            mutexUnlock();
            return movesMax;
        }

        int getBestMoves(String[] moves) {
            moves[0] = m_bestMoves;
            return m_bestMoveCount;
        }
    }

    private static boolean searchMovesQuickForCcp(cubecorners_perm ccp,
            CornerPermReprCubes ccpReprCubes,
            CubesReprByDepth cubesReprByDepth,
            BGCubesReprByDepthAdd bgcubesReprByDepthAdd,
            CubeCosetsAtDepth bgCosetsAtDepth,
            List<CubeWithMoves> csearchWithMovesAppend,
            int depth, int depth1Max, int movesMaxp,
            Responder responder, QuickSearchProgress searchProgress) {
        int movesMax = movesMaxp;
        cube[][] csearchTarr0 = new cube[3][csearchWithMovesAppend.size()];
        cube[][] csearchTarr1 = new cube[3][csearchWithMovesAppend.size()];
        for (int i = 0; i < csearchWithMovesAppend.size(); i++) {
            cube c = csearchWithMovesAppend.get(i).c;
            csearchTarr0[0][i] = c;
            csearchTarr0[1][i] = c.transform(1);
            csearchTarr0[2][i] = c.transform(2);
            cube crev = c.reverse();
            csearchTarr1[0][i] = crev;
            csearchTarr1[1][i] = crev.transform(1);
            csearchTarr1[2][i] = crev.transform(2);
        }
        for (int reversed = 0; reversed < (cubesReprByDepth.isUseReverse() ? 2 : 1); reversed++) {
            cubecorners_perm ccprev = (reversed != 0) ? ccp.reverse() : ccp;
            for (int symmetric = 0; symmetric <= 1; symmetric++) {
                cubecorners_perm ccprevsymm = (symmetric != 0) ? ccprev.symmetric() : ccprev;
                for (int td = 0; td < transform_dir.TCOUNT.ordinal(); td++) {
                    cubecorners_perm ccpT = ccprevsymm.transform(td);
                    for (CornerOrientReprCubes ccoReprCubes : ccpReprCubes.ccoCubesList()) {
                        cubecorner_orients cco = ccoReprCubes.getOrients();
                        cubecorner_orients ccorev = (reversed != 0) ? cco.reverse(ccp) : cco;
                        cubecorner_orients ccorevsymm = (symmetric != 0) ? ccorev.symmetric() : ccorev;
                        cubecorner_orients ccoT = ccorevsymm.transform(ccprevsymm, td);
                        List<cubeedges> ceTarr = new ArrayList<>();
                        for (int srchItem = 0; srchItem < csearchWithMovesAppend.size(); srchItem++) {
                            for (int searchRev = 0; searchRev < TWOPHASE_SEARCHREV; searchRev++) {
                                for (int searchTd = 0; searchTd < 3; searchTd++) {
                                    cube csearchT = (searchRev == 0 ? csearchTarr0 : csearchTarr1)[searchTd][srchItem];
                                    cubecorners_perm ccpSearch = cubecorners_perm.compose(ccpT, csearchT.ccp);
                                    cubecorner_orients ccoSearch = cubecorner_orients.compose(ccoT, csearchT.ccp, csearchT.cco);
                                    cubecorner_orients ccoSearchReprBG = ccoSearch.representativeBG(ccpSearch);
                                    int searchReprCOrientIdx = ccoSearchReprBG.getOrientIdx();
                                    if (bgCosetsAtDepth.containsCCOrients(searchReprCOrientIdx)) {
                                        if (ceTarr.isEmpty()) {
                                            for (long edges : ccoReprCubes.edgeList()) {
                                                cubeedges ce = new cubeedges(edges);
                                                cubeedges cerev = (reversed != 0) ? ce.reverse() : ce;
                                                cubeedges cerevsymm = (symmetric != 0) ? cerev.symmetric() : cerev;
                                                ceTarr.add(cerevsymm.transform(td));
                                            }
                                        }
                                        for (cubeedges ceT : ceTarr) {
                                            cubeedges ceSearch = cubeedges.compose(ceT, csearchT.ce);
                                            cubeedges ceSearchSpaceRepr = ceSearch.representativeBG();
                                            List<cube> cubesForCE = bgCosetsAtDepth.getCubesForCE(searchReprCOrientIdx, ceSearchSpaceRepr);
                                            if (cubesForCE != null) {
                                                cube cSearch1 = new cube(ccpSearch, ccoSearch, ceSearch);
                                                String[] inspaceWithCube2Moves = {""};
                                                int moveCount = searchPhase1Cube2(cubesReprByDepth,
                                                        bgcubesReprByDepthAdd,
                                                        cSearch1, cubesForCE, searchRev, searchTd,
                                                        depth1Max, movesMax - depth,
                                                        searchProgress.isCatchFirst(),
                                                        responder, inspaceWithCube2Moves);
                                                if (moveCount >= 0) {
                                                    cube cube1 = new cube(ccpT, ccoT, ceT);
                                                    cube cube1T = cube1.transform(transformReverse(searchTd));
                                                    String cube1Moves = cubesReprByDepth.getMoves(cube1T, searchRev != 0);
                                                    String cubeMovesAppend = csearchWithMovesAppend.get(srchItem).moves;
                                                    String ms = (searchRev != 0)
                                                        ? cube1Moves + inspaceWithCube2Moves[0]
                                                        : inspaceWithCube2Moves[0] + cube1Moves;
                                                    ms += cubeMovesAppend;
                                                    movesMax = searchProgress.setBestMoves(ms, depth + moveCount, responder);
                                                    if (movesMax < 0) return true;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (ProgressBase.isStopRequested()) return true;
                    }
                }
            }
        }
        return false;
    }

    private static void searchMovesQuickTa(int threadNo,
            CubesReprByDepth cubesReprByDepth,
            BGCubesReprByDepthAdd bgcubesReprByDepthAdd,
            CubeCosets bgCosets,
            cube csearch, int depth, int depth1Max,
            Responder responder, QuickSearchProgress searchProgress) {
        int[] itemIdx = {-1};
        while (true) {
            int movesMax = searchProgress.inc(responder, itemIdx);
            if (movesMax < 0) break;
            CornerPermReprCubes ccpReprCubes = cubesReprByDepth.getAt(depth).getAt(itemIdx[0]);
            cubecorners_perm ccp = cubesReprByDepth.getReprPermForIdx(itemIdx[0]);
            if (!ccpReprCubes.empty()) {
                List<CubeWithMoves> cubesWithMoves = List.of(new CubeWithMoves(csearch, ""));
                if (searchMovesQuickForCcp(ccp, ccpReprCubes, cubesReprByDepth,
                        bgcubesReprByDepthAdd, bgCosets.getAt(depth1Max),
                        cubesWithMoves, depth, depth1Max, movesMax, responder, searchProgress))
                    return;
            }
        }
    }

    private static void searchMovesQuickTb1(int threadNo, CubesReprByDepth cubesReprByDepth,
            BGCubesReprByDepthAdd bgcubesReprByDepthAdd,
            CubeCosets bgCosets, cube csearch, int depth1Max,
            Responder responder, QuickSearchProgress searchProgress) {
        int[] item2Idx = {-1};
        while (true) {
            int movesMax = searchProgress.inc(responder, item2Idx);
            if (movesMax < 0) break;
            CornerPermReprCubes ccp2ReprCubes = cubesReprByDepth.getAt(depth1Max).getAt(item2Idx[0]);
            cubecorners_perm ccp2 = cubesReprByDepth.getReprPermForIdx(item2Idx[0]);
            if (ccp2ReprCubes.empty()) continue;
            List<CubeWithMoves> cubesWithMoves = new ArrayList<>();
            for (int rd = 0; rd < rotate_dir.RCOUNT.ordinal(); rd++) {
                cube c1Search = cube.compose(crotated[rd], csearch);
                String cube1Moves = cubesReprByDepth.getMoves(crotated[rd]);
                cubesWithMoves.add(new CubeWithMoves(c1Search, cube1Moves));
            }
            if (searchMovesQuickForCcp(ccp2, ccp2ReprCubes, cubesReprByDepth,
                    bgcubesReprByDepthAdd, bgCosets.getAt(depth1Max),
                    cubesWithMoves, depth1Max + 1, depth1Max, movesMax, responder, searchProgress))
                return;
        }
    }

    private static void searchMovesQuickTb(int threadNo, CubesReprByDepth cubesReprByDepth,
            BGCubesReprByDepthAdd bgcubesReprByDepthAdd,
            CubeCosets bgCosets, cube csearch, int depth, int depth1Max,
            Responder responder, QuickSearchProgress searchProgress) {
        CubesReprAtDepth ccReprCubesC = cubesReprByDepth.getAt(depth);
        while (true) {
            int[] item2Idx = {0};
            int movesMax = searchProgress.inc(responder, item2Idx);
            if (movesMax < 0) break;
            CornerPermReprCubes ccp2ReprCubes = cubesReprByDepth.getAt(depth1Max).getAt(item2Idx[0]);
            cubecorners_perm ccp2 = cubesReprByDepth.getReprPermForIdx(item2Idx[0]);
            if (ccp2ReprCubes.empty()) continue;
            CornerPermReprCubes[] list = ccReprCubesC.ccpCubesList();
            for (int idx1 = 0; idx1 < list.length; idx1++) {
                CornerPermReprCubes ccp1ReprCubes = list[idx1];
                cubecorners_perm ccp1 = ccReprCubesC.getPermAt(idx1);
                if (ccp1ReprCubes.empty()) continue;
                for (CornerOrientReprCubes cco1ReprCubes : ccp1ReprCubes.ccoCubesList()) {
                    cubecorner_orients cco1 = cco1ReprCubes.getOrients();
                    for (long edges1 : cco1ReprCubes.edgeList()) {
                        cubeedges ce1 = new cubeedges(edges1);
                        List<cube> cubesChecked = new ArrayList<>();
                        List<CubeWithMoves> cubesWithMoves = new ArrayList<>();
                        for (int rev1 = 0; rev1 <= (cubesReprByDepth.isUseReverse() ? 1 : 0); rev1++) {
                            cubecorners_perm ccp1rev = (rev1 != 0) ? ccp1.reverse() : ccp1;
                            cubecorner_orients cco1rev = (rev1 != 0) ? cco1.reverse(ccp1) : cco1;
                            cubeedges ce1rev = (rev1 != 0) ? ce1.reverse() : ce1;
                            for (int sym1 = 0; sym1 < 2; sym1++) {
                                cubecorners_perm ccp1revsymm = (sym1 != 0) ? ccp1rev.symmetric() : ccp1rev;
                                cubecorner_orients cco1revsymm = (sym1 != 0) ? cco1rev.symmetric() : cco1rev;
                                cubeedges ce1revsymm = (sym1 != 0) ? ce1rev.symmetric() : ce1rev;
                                for (int td1 = 0; td1 < transform_dir.TCOUNT.ordinal(); td1++) {
                                    cubecorners_perm ccp1T = ccp1revsymm.transform(td1);
                                    cubecorner_orients cco1T = cco1revsymm.transform(ccp1revsymm, td1);
                                    cubeedges ce1T = ce1revsymm.transform(td1);
                                    cube c1T = new cube(ccp1T, cco1T, ce1T);
                                    if (cubesChecked.contains(c1T)) continue;
                                    cubesChecked.add(c1T);
                                    cubecorners_perm ccp1Search = cubecorners_perm.compose(ccp1T, csearch.ccp);
                                    cubecorner_orients cco1Search = cubecorner_orients.compose(cco1T, csearch.ccp, csearch.cco);
                                    cubeedges ce1Search = cubeedges.compose(ce1T, csearch.ce);
                                    cube c1Search = new cube(ccp1Search, cco1Search, ce1Search);
                                    String cube1Moves = cubesReprByDepth.getMoves(c1T);
                                    cubesWithMoves.add(new CubeWithMoves(c1Search, cube1Moves));
                                }
                            }
                        }
                        if (searchMovesQuickForCcp(ccp2, ccp2ReprCubes, cubesReprByDepth,
                                bgcubesReprByDepthAdd, bgCosets.getAt(depth1Max),
                                cubesWithMoves, depth + depth1Max, depth1Max, movesMax,
                                responder, searchProgress))
                            return;
                    }
                }
            }
        }
    }

    private static boolean searchMovesQuickA(CubesReprByDepthAdd cubesReprByDepthAdd,
            BGCubesReprByDepthAdd bgcubesReprByDepthAdd,
            CubeCosetsAdd bgCosetsAdd,
            cube csearch, int depthSearch, boolean catchFirst,
            Responder responder, int movesMax,
            int[] moveCount, String[] moves) {
        moveCount[0] = -1;
        CubesReprByDepth cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(0, responder);
        CubeCosets bgCosets = bgCosetsAdd.getBGcosets(cubesReprByDepthAdd, 0, responder);
        if (cubesReprByDepth == null || bgCosets == null) return true;
        int depthsAvail = Math.min(cubesReprByDepth.availCount(), bgCosets.availCount()) - 1;
        int depth, depth1Max;
        if (depthSearch <= depthsAvail) {
            depth = 0; depth1Max = depthSearch;
        } else if (depthSearch <= 2 * depthsAvail) {
            depth = depthSearch - depthsAvail; depth1Max = depthsAvail;
        } else {
            depth = depthSearch / 2; depth1Max = depthSearch - depth;
            cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depth1Max, responder);
            bgCosets = bgCosetsAdd.getBGcosets(cubesReprByDepthAdd, depth1Max, responder);
            if (cubesReprByDepth == null || bgCosets == null) return true;
        }
        QuickSearchProgress searchProgress = new QuickSearchProgress(
                cubesReprByDepth.getAt(depth).size(), depthSearch, catchFirst, movesMax);
        final CubesReprByDepth crbd = cubesReprByDepth;
        final CubeCosets bg = bgCosets;
        final int d = depth, dm = depth1Max;
        runInThreadPool(threadNo -> searchMovesQuickTa(threadNo, crbd, bgcubesReprByDepthAdd,
                bg, csearch, d, dm, responder, searchProgress));
        moveCount[0] = searchProgress.getBestMoves(moves);
        return ProgressBase.isStopRequested();
    }

    private static boolean searchMovesQuickB(CubesReprByDepthAdd cubesReprByDepthAdd,
            BGCubesReprByDepthAdd bgcubesReprByDepthAdd,
            CubeCosetsAdd bgCosetsAdd,
            cube csearch, int depth1Max, int depthSearch, boolean catchFirst,
            Responder responder, int movesMax, int[] moveCount, String[] moves) {
        moveCount[0] = -1;
        CubesReprByDepth cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(depth1Max, responder);
        CubeCosets bgCosets = bgCosetsAdd.getBGcosets(cubesReprByDepthAdd, depth1Max, responder);
        if (cubesReprByDepth == null || bgCosets == null) return true;
        int depth = depthSearch - 2 * depth1Max;
        QuickSearchProgress searchProgress = new QuickSearchProgress(
                cubesReprByDepth.getAt(depth1Max).size(), depthSearch, catchFirst, movesMax);
        final CubesReprByDepth crbd = cubesReprByDepth;
        final CubeCosets bg = bgCosets;
        if (depth == 1)
            runInThreadPool(threadNo -> searchMovesQuickTb1(threadNo, crbd, bgcubesReprByDepthAdd,
                    bg, csearch, depth1Max, responder, searchProgress));
        else
            runInThreadPool(threadNo -> searchMovesQuickTb(threadNo, crbd, bgcubesReprByDepthAdd,
                    bg, csearch, depth, depth1Max, responder, searchProgress));
        moveCount[0] = searchProgress.getBestMoves(moves);
        return ProgressBase.isStopRequested();
    }

    public static void searchMovesQuickCatchFirst(
            CubesReprByDepthAdd cubesReprByDepthAdd,
            BGCubesReprByDepthAdd bgcubesReprByDepthAdd,
            CubeCosetsAdd bgCosetsAdd,
            cube csearch, Responder responder) {
        CubesReprByDepth cubesReprByDepth = cubesReprByDepthAdd.getReprCubes(0, responder);
        CubeCosets bgCosets = bgCosetsAdd.getBGcosets(cubesReprByDepthAdd, 0, responder);
        if (cubesReprByDepth == null || bgCosets == null) return;
        int depth1Max = Math.min(cubesReprByDepth.availCount(), bgCosets.availCount()) - 1;
        depth1Max = Math.max(depth1Max, TWOPHASE_DEPTH1_CATCHFIRST_MAX);
        for (int depthSearch = 0; depthSearch <= 12; depthSearch++) {
            if (depthSearch > 3 * TWOPHASE_DEPTH1_CATCHFIRST_MAX) break;
            int[] moveCount = {0};
            String[] bestMoves = {""};
            boolean isFinish = (depthSearch <= 2 * depth1Max)
                ? searchMovesQuickA(cubesReprByDepthAdd, bgcubesReprByDepthAdd, bgCosetsAdd,
                        csearch, depthSearch, true, responder, 999, moveCount, bestMoves)
                : searchMovesQuickB(cubesReprByDepthAdd, bgcubesReprByDepthAdd, bgCosetsAdd,
                        csearch, TWOPHASE_DEPTH1_CATCHFIRST_MAX, depthSearch,
                        true, responder, 999, moveCount, bestMoves);
            if (moveCount[0] >= 0) {
                responder.solution(bestMoves[0]);
                return;
            }
            if (isFinish) return;
            responder.message("depth " + depthSearch + " end");
        }
        responder.message("not found");
    }

    public static void searchMovesQuickMulti(
            CubesReprByDepthAdd cubesReprByDepthAdd,
            BGCubesReprByDepthAdd bgcubesReprByDepthAdd,
            CubeCosetsAdd bgCosetsAdd,
            cube csearch, Responder responder) {
        String movesBest = "";
        int bestMoveCount = 999;
        for (int depthSearch = 0; depthSearch <= 12; depthSearch++) {
            if (depthSearch > 3 * TWOPHASE_DEPTH1_MULTI_MAX || depthSearch >= bestMoveCount) break;
            int[] moveCount = {0};
            String[] moves = {""};
            boolean isFinish = (depthSearch <= 2 * TWOPHASE_DEPTH1_MULTI_MAX)
                ? searchMovesQuickA(cubesReprByDepthAdd, bgcubesReprByDepthAdd, bgCosetsAdd,
                        csearch, depthSearch, false, responder, bestMoveCount - 1, moveCount, moves)
                : searchMovesQuickB(cubesReprByDepthAdd, bgcubesReprByDepthAdd, bgCosetsAdd,
                        csearch, TWOPHASE_DEPTH1_MULTI_MAX, depthSearch,
                        false, responder, bestMoveCount - 1, moveCount, moves);
            if (moveCount[0] >= 0) {
                movesBest = moves[0];
                bestMoveCount = moveCount[0];
                if (bestMoveCount == 0) break;
            }
            if (isFinish) break;
            responder.message("depth " + depthSearch + " end");
        }
        if (!movesBest.isEmpty()) {
            responder.solution(movesBest);
            return;
        }
        responder.message("not found");
    }
}

