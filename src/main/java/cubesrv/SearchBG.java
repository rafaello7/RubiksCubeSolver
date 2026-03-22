package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.CPermReprBG.*;
import static cubesrv.CubesReprBG.*;
import static cubesrv.CubesAddBG.*;

public class SearchBG {

    private static class SearchIndexesBG {
        int permReprIdx   = 0;
        boolean reversed  = false;
        boolean symmetric = false;
        int td            = 0;

        boolean inc(boolean useReverse) {
            if (++td == TCOUNTBG) {
                if (symmetric) {
                    if (useReverse) {
                        if (reversed && ++permReprIdx == 1672) return false;
                        reversed = !reversed;
                    } else if (++permReprIdx == 2768) {
                        return false;
                    }
                }
                symmetric = !symmetric;
                td = 0;
            }
            return true;
        }
    }

    private static String getInSpaceMovesForMatch(BGCubesReprByDepth cubesReprByDepth,
            cube cSearch, cube c, boolean searchRev, int searchTd,
            int reversed, int symmetric, int tdidx) {
        int td = BGSpaceTransforms[tdidx].ordinal();
        cube cSearchT     = cSearch.transform(transformReverse(td));
        cube cSearchTsymm = (symmetric != 0) ? cSearchT.symmetric() : cSearchT;
        cube cT           = c.transform(transformReverse(td));
        cube cTsymm       = (symmetric != 0) ? cT.symmetric() : cT;
        String moves;
        if (searchRev) {
            moves  = cubesReprByDepth.getMoves(cTsymm, searchTd, reversed == 0);
            moves += cubesReprByDepth.getMoves(cSearchTsymm, searchTd, reversed != 0);
        } else {
            moves  = cubesReprByDepth.getMoves(cSearchTsymm, searchTd, reversed == 0);
            moves += cubesReprByDepth.getMoves(cTsymm, searchTd, reversed != 0);
        }
        return moves;
    }

    private static void generateInSpaceSearchTarr(cube csearch, boolean useReverse,
            cube[][][] cSearchTarr) {
        for (int rev = 0; rev < (useReverse ? 2 : 1); rev++) {
            cube csearchrev = (rev != 0) ? csearch.reverse() : csearch;
            for (int sym = 0; sym <= 1; sym++) {
                cube csearchrevsymm = (sym != 0) ? csearchrev.symmetric() : csearchrev;
                for (int tdidx = 0; tdidx < TCOUNTBG; tdidx++)
                    cSearchTarr[rev][sym][tdidx] =
                        csearchrevsymm.transform(BGSpaceTransforms[tdidx].ordinal());
            }
        }
    }

    private static boolean searchInSpaceMovesForIdxs(BGCubesReprByDepth cubesReprByDepth,
            int depth, int depthMax, cube[][][] cSearchTarr,
            SearchIndexesBG indexes, String[] moves, boolean searchRev, int searchTd) {
        cube cSearchT = cSearchTarr[indexes.reversed ? 1 : 0][indexes.symmetric ? 1 : 0][indexes.td];
        cube[] c = {new cube()};
        cube[] cSearch = {new cube()};
        if (cubesReprByDepth.searchMovesForReprPerm(indexes.permReprIdx,
                depth, depthMax, cSearchT, indexes.reversed, c, cSearch)) {
            moves[0] = getInSpaceMovesForMatch(cubesReprByDepth, cSearch[0], c[0],
                    searchRev, searchTd, indexes.reversed ? 1 : 0, indexes.symmetric ? 1 : 0, indexes.td);
            return true;
        }
        return false;
    }

    private static boolean searchInSpaceMovesA(BGCubesReprByDepth cubesReprByDepthBG,
            cube[][][] cSpaceArr, boolean searchRev, int searchTd,
            int depth, int depthMax, String[] moves) {
        SearchIndexesBG indexes = new SearchIndexesBG();
        boolean useReverse = cubesReprByDepthBG.isUseReverse();
        do {
            if (searchInSpaceMovesForIdxs(cubesReprByDepthBG, depth, depthMax,
                    cSpaceArr, indexes, moves, searchRev, searchTd))
                return true;
        } while (indexes.inc(useReverse));
        return false;
    }

    private static boolean searchInSpaceMovesB(BGCubesReprByDepth cubesReprByDepthBG,
            cube cSpace, boolean searchRev, int searchTd, int depth, int depthMax,
            String[] moves) {
        BGCubesReprAtDepth ccReprCubesC = cubesReprByDepthBG.getAt(depth);
        for (int idx1 = 0; idx1 < ccReprCubesC.ccpCubesList().length; idx1++) {
            BGCornerPermReprCubes ccpCubes1 = ccReprCubesC.ccpCubesList()[idx1];
            cubecorners_perm ccp1 = ccReprCubesC.getPermAt(idx1);
            if (ccpCubes1.empty()) continue;
            for (long edges1 : ccpCubes1.edgeList()) {
                cubeedges ce1 = new cubeedges(edges1);
                cube c1 = new cube(ccp1, csolved.cco, ce1);
                java.util.Set<cube> cubesChecked = new java.util.HashSet<>();
                for (int rev1 = 0; rev1 <= (cubesReprByDepthBG.isUseReverse() ? 1 : 0); rev1++) {
                    cube c1r = (rev1 != 0) ? c1.reverse() : c1;
                    for (int sym1 = 0; sym1 <= 1; sym1++) {
                        cube c1rs = (sym1 != 0) ? c1r.symmetric() : c1r;
                        for (int td1idx = 0; td1idx < TCOUNTBG; td1idx++) {
                            cube c1T = c1rs.transform(BGSpaceTransforms[td1idx].ordinal());
                            if (cubesChecked.contains(c1T)) continue;
                            cubesChecked.add(c1T);
                            cube cSearch1 = cube.compose(c1T, cSpace);
                            String[] moves2 = {""};
                            cube[][][] cSpaceArr = new cube[2][2][TCOUNTBG];
                            for (cube[][] a : cSpaceArr) for (cube[] b : a) java.util.Arrays.fill(b, new cube());
                            generateInSpaceSearchTarr(cSearch1, cubesReprByDepthBG.isUseReverse(), cSpaceArr);
                            if (searchInSpaceMovesA(cubesReprByDepthBG, cSpaceArr, searchRev,
                                    searchTd, depthMax, depthMax, moves2)) {
                                if (searchRev)
                                    moves[0] = cubesReprByDepthBG.getMoves(c1T, searchTd, true) + moves2[0];
                                else
                                    moves[0] = moves2[0] + cubesReprByDepthBG.getMoves(c1T, searchTd);
                                return true;
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    public static int searchInSpaceMoves(BGCubesReprByDepthAdd cubesReprByDepthAdd,
            cube cSpace, boolean searchRev, int searchTd,
            int movesMax, Responder responder, String[] moves) {
        BGCubesReprByDepth cubesReprByDepthBG = cubesReprByDepthAdd.getReprCubes(0, responder);
        if (cubesReprByDepthBG == null) return -1;

        for (int depthSearch = 0; depthSearch <= movesMax; depthSearch++) {
            if (depthSearch >= cubesReprByDepthBG.availCount()) break;
            if (cubesReprByDepthBG.getAt(depthSearch).containsCube(cSpace)) {
                moves[0] = cubesReprByDepthBG.getMoves(cSpace, searchTd, !searchRev);
                return depthSearch;
            }
        }

        if (cubesReprByDepthBG.availCount() <= movesMax) {
            cube[][][] cSpaceTarr = new cube[2][2][TCOUNTBG];
            for (cube[][] a : cSpaceTarr) for (cube[] b : a) java.util.Arrays.fill(b, new cube());
            generateInSpaceSearchTarr(cSpace, cubesReprByDepthBG.isUseReverse(), cSpaceTarr);

            for (int depthSearch = cubesReprByDepthBG.availCount(); depthSearch <= movesMax; depthSearch++) {
                if (depthSearch > 3 * TWOPHASE_DEPTH2_MAX) break;

                if (depthSearch < 2 * cubesReprByDepthBG.availCount() - 1 && depthSearch <= movesMax) {
                    int depthMax = cubesReprByDepthBG.availCount() - 1;
                    if (searchInSpaceMovesA(cubesReprByDepthBG, cSpaceTarr, searchRev, searchTd,
                            depthSearch - depthMax, depthMax, moves))
                        return depthSearch;
                } else if (depthSearch <= 2 * TWOPHASE_DEPTH2_MAX) {
                    int depth   = depthSearch / 2;
                    int depthMax = depthSearch - depth;
                    cubesReprByDepthBG = cubesReprByDepthAdd.getReprCubes(depthMax, responder);
                    if (cubesReprByDepthBG == null) return -1;
                    if (searchInSpaceMovesA(cubesReprByDepthBG, cSpaceTarr, searchRev, searchTd,
                            depth, depthMax, moves))
                        return depthSearch;
                } else {
                    cubesReprByDepthBG = cubesReprByDepthAdd.getReprCubes(TWOPHASE_DEPTH2_MAX, responder);
                    if (cubesReprByDepthBG == null) return -1;
                    int depth = depthSearch - 2 * TWOPHASE_DEPTH2_MAX;
                    if (searchInSpaceMovesB(cubesReprByDepthBG, cSpace, searchRev, searchTd,
                            depth, TWOPHASE_DEPTH2_MAX, moves))
                        return 2 * TWOPHASE_DEPTH2_MAX + depth;
                }
            }
        }
        return -1;
    }
}

