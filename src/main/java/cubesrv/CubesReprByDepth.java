package cubesrv;

import static cubesrv.CubeDefs.*;
import java.util.ArrayList;
import java.util.List;

public class CubesReprByDepth {
    public final CubecornerReprPerms m_reprPerms;
    private final List<CubesReprAtDepth> m_cubesAtDepths = new ArrayList<>();
    private int m_availCount = 1;

    public CubesReprByDepth(boolean useReverse) {
        this.m_reprPerms = new CubecornerReprPerms(useReverse);
        m_cubesAtDepths.add(new CubesReprAtDepth(m_reprPerms));
        int cornerPermReprIdx = m_reprPerms.getReprPermIdx(csolved.ccp);
        CornerPermReprCubes ccpCubes = m_cubesAtDepths.get(0).add(cornerPermReprIdx);
        List<EdgeReprCandidateTransform> otransform = new ArrayList<>();
        CubecornerOrients ccoRepr = m_reprPerms.getReprOrients(csolved.ccp, csolved.cco, otransform);
        CornerOrientReprCubes ccoCubes = ccpCubes.cornerOrientCubesAdd(ccoRepr);
        CubeEdges ceRepr = m_reprPerms.getReprCubeedges(csolved.ce, otransform);
        ccoCubes.addCubes(java.util.Collections.singletonList(ceRepr));
    }

    public boolean isUseReverse() {
        return m_reprPerms.isUseReverse();
    }

    public int availCount() {
        return m_availCount;
    }

    public void incAvailCount() {
        ++m_availCount;
    }

    public CubesReprAtDepth getAt(int idx) {
        while (idx >= m_cubesAtDepths.size())
            m_cubesAtDepths.add(new CubesReprAtDepth(m_reprPerms));
        return m_cubesAtDepths.get(idx);
    }

    public CubecornersPerm getReprPermForIdx(int reprPermIdx) {
        return m_reprPerms.getPermForIdx(reprPermIdx);
    }

    public String getMoves(Cube c, boolean movesRevp) {
        boolean movesRev = movesRevp;
        List<Integer> rotateDirs = new ArrayList<>();
        int insertPos = 0;
        Cube crepr = m_reprPerms.cubeRepresentative(c);
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
        Cube cc = c;
        while (depth-- > 0) {
            int cm = 0;
            Cube ccRev = cc.reverse();
            Cube cc1 = cc;
            while (cm < RotateDir.RCOUNT.ordinal()) {
                cc1 = Cube.compose(cc, crotated[cm]);
                Cube cc1repr = m_reprPerms.cubeRepresentative(cc1);
                int ccpReprIdx2 = m_reprPerms.getReprPermIdx(cc1repr.ccp);
                CornerPermReprCubes ccpReprCubes = m_cubesAtDepths.get(depth).getAt(ccpReprIdx2);
                CornerOrientReprCubes ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(cc1repr.cco);
                if (ccoReprCubes.containsCubeEdges(cc1repr.ce)) break;
                cc1 = Cube.compose(ccRev, crotated[cm]);
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
            if (cm == RotateDir.RCOUNT.ordinal()) {
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

    public String getMoves(Cube c) {
        return getMoves(c, false);
    }

    public int addCubesForReprPerm(int reprPermIdx, int depth) {
        List<EdgeReprCandidateTransform> otransformNew = new ArrayList<>();
        int cubeCount = 0;
        CubesReprAtDepth ccpReprCubesC = m_cubesAtDepths.get(depth - 1);
        CornerPermReprCubes ccpReprCubesNewP = (depth == 1) ? null : m_cubesAtDepths.get(depth - 2).getAt(reprPermIdx);
        CornerPermReprCubes ccpReprCubesNewC = ccpReprCubesC.getAt(reprPermIdx);
        CornerPermReprCubes ccpReprCubesNewN = m_cubesAtDepths.get(depth).add(reprPermIdx);
        CubecornersPerm ccpNewRepr = m_reprPerms.getPermForIdx(reprPermIdx);
        java.util.Set<CubecornersPerm> ccpChecked = new java.util.HashSet<>();
        for (int trrev = 0; trrev < (m_reprPerms.isUseReverse() ? 2 : 1); trrev++) {
            CubecornersPerm ccpNewReprRev = (trrev != 0) ? ccpNewRepr.reverse() : ccpNewRepr;
            for (int symmetric = 0; symmetric <= 1; symmetric++) {
                CubecornersPerm ccpNewS = (symmetric != 0) ? ccpNewReprRev.symmetric() : ccpNewReprRev;
                for (int td = 0; td < TransformDir.TCOUNT.ordinal(); td++) {
                    CubecornersPerm ccpNew = ccpNewS.transform(td);
                    if (!ccpChecked.contains(ccpNew)) {
                        ccpChecked.add(ccpNew);
                        for (int rd = 0; rd < RotateDir.RCOUNT.ordinal(); rd++) {
                            int rdRev = rotateDirReverse(rd);
                            for (int reversed = 0; reversed < (m_reprPerms.isUseReverse() ? 2 : 1); reversed++) {
                                CubecornersPerm ccp = (reversed != 0)
                                        ? CubecornersPerm.compose(crotated[rdRev].ccp, ccpNew)
                                        : CubecornersPerm.compose(ccpNew, crotated[rdRev].ccp);
                                int ccpReprIdx = m_reprPerms.getReprPermIdx(ccp);
                                if (m_reprPerms.getPermForIdx(ccpReprIdx).equals(ccp)) {
                                    CornerPermReprCubes cpermReprCubesC = ccpReprCubesC.getAt(ccpReprIdx);
                                    for (CornerOrientReprCubes corientReprCubesC : cpermReprCubesC.ccoCubesList()) {
                                        CubecornerOrients cco = corientReprCubesC.getOrients();
                                        CubecornerOrients ccoNew = (reversed != 0)
                                                ? CubecornerOrients.compose(crotated[rd].cco, ccp, cco)
                                                : CubecornerOrients.compose(cco, crotated[rd].ccp, crotated[rd].cco);
                                        CubecornerOrients ccoReprNew = m_reprPerms.getComposedReprOrients(
                                                ccpNew, ccoNew, reversed != 0, crotated[rd].ce, otransformNew);
                                        CornerOrientReprCubes corientReprCubesNewP =
                                                (ccpReprCubesNewP != null) ? ccpReprCubesNewP.cornerOrientCubesAt(ccoReprNew) : null;
                                        CornerOrientReprCubes corientReprCubesNewC =
                                                ccpReprCubesNewC.cornerOrientCubesAt(ccoReprNew);
                                        List<CubeEdges> ceNewArr = new ArrayList<>();
                                        for (long edges : corientReprCubesC.edgeList()) {
                                            CubeEdges ce = new CubeEdges(edges);
                                            CubeEdges cenewRepr = CubecornerReprPerms.getComposedReprCubeedges(
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
                                          Cube cSearchT, boolean reversed, Cube[] c, Cube[] cSearch) {
        List<EdgeReprCandidateTransform> otransform = new ArrayList<>();
        CornerPermReprCubes ccpReprCubes = m_cubesAtDepths.get(depth).getAt(reprPermIdx);
        CubecornersPerm ccp = m_reprPerms.getPermForIdx(reprPermIdx);
        if (!ccpReprCubes.empty()) {
            CubecornersPerm ccpSearch = reversed
                    ? CubecornersPerm.compose(cSearchT.ccp, ccp)
                    : CubecornersPerm.compose(ccp, cSearchT.ccp);
            CornerPermReprCubes ccpReprSearchCubes = m_cubesAtDepths.get(depthMax).getFor(ccpSearch);
            if (ccpReprCubes.size() <= ccpReprSearchCubes.size() || !m_reprPerms.isSingleTransform(ccpSearch)) {
                for (CornerOrientReprCubes ccoReprCubes : ccpReprCubes.ccoCubesList()) {
                    CubecornerOrients cco = ccoReprCubes.getOrients();
                    CubecornerOrients ccoSearch = reversed
                            ? CubecornerOrients.compose(cSearchT.cco, ccp, cco)
                            : CubecornerOrients.compose(cco, cSearchT.ccp, cSearchT.cco);
                    CubecornerOrients ccoSearchRepr = m_reprPerms.getComposedReprOrients(
                            ccpSearch, ccoSearch, reversed, cSearchT.ce, otransform);
                    CornerOrientReprCubes ccoReprSearchCubes = ccpReprSearchCubes.cornerOrientCubesAt(ccoSearchRepr);
                    if (ccoReprSearchCubes.empty()) continue;
                    CubeEdges ce = CornerOrientReprCubes.findSolutionEdge(
                            ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
                    if (!ce.isNil()) {
                        CubeEdges ceSearch = reversed
                                ? CubeEdges.compose(cSearchT.ce, ce)
                                : CubeEdges.compose(ce, cSearchT.ce);
                        cSearch[0] = new Cube(ccpSearch, ccoSearch, ceSearch);
                        c[0] = new Cube(ccp, cco, ce);
                        return true;
                    }
                }
            } else {
                for (CornerOrientReprCubes ccoReprSearchCubes : ccpReprSearchCubes.ccoCubesList()) {
                    CubecornerOrients ccoSearchRepr = ccoReprSearchCubes.getOrients();
                    CubecornerOrients cco = m_reprPerms.getOrientsForComposedRepr(
                            ccpSearch, ccoSearchRepr, reversed, cSearchT, otransform);
                    CornerOrientReprCubes ccoReprCubes = ccpReprCubes.cornerOrientCubesAt(cco);
                    if (ccoReprCubes.empty()) continue;
                    CubeEdges ce = CornerOrientReprCubes.findSolutionEdge(
                            ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
                    if (!ce.isNil()) {
                        CubecornerOrients ccoSearch = reversed
                                ? CubecornerOrients.compose(cSearchT.cco, ccp, cco)
                                : CubecornerOrients.compose(cco, cSearchT.ccp, cSearchT.cco);
                        CubeEdges ceSearch = reversed
                                ? CubeEdges.compose(cSearchT.ce, ce)
                                : CubeEdges.compose(ce, cSearchT.ce);
                        cSearch[0] = new Cube(ccpSearch, ccoSearch, ceSearch);
                        c[0] = new Cube(ccp, cco, ce);
                        return true;
                    }
                }
            }
        }
        return false;
    }
}
