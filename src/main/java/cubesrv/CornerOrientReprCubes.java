package cubesrv;

import static cubesrv.CubeDefs.ctransformed;
import static cubesrv.CubeDefs.transformReverse;
import java.util.*;

public class CornerOrientReprCubes {
    private final CubecornerOrients m_orients;
    public long[] m_items = new long[0];
    private int[] m_orientOccur = null;

    public CornerOrientReprCubes(CubecornerOrients orients) {
        this.m_orients = orients;
    }

    public CubecornerOrients getOrients() {
        return m_orients;
    }

    public int addCubes(Collection<CubeEdges> cearr) {
        TreeMap<Long, Integer> edgeMap = new TreeMap<>();
        for (CubeEdges ce : cearr) {
            long edge = ce.get();
            if (!edgeMap.containsKey(edge)) {
                int idx = Arrays.binarySearch(m_items, edge);
                if (idx < 0) {
                    edgeMap.put(edge, -1 - idx);
                }
            }
        }
        List<Map.Entry<Long, Integer>> edgeList = new ArrayList<>(edgeMap.entrySet());
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
            CubeEdges ce = new CubeEdges(edges);
            int orientIdx = ce.getOrientIdx();
            orientOcc[orientIdx >>> 5] |= (1 << (orientIdx & 0x1f));
        }
        m_orientOccur = orientOcc;
    }

    public boolean containsCubeEdges(CubeEdges ce) {
        if (m_orientOccur != null) {
            int orientIdx = ce.getOrientIdx();
            int orientOcc = m_orientOccur[orientIdx >>> 5];
            if ((orientOcc & (1 << (orientIdx & 0x1f))) == 0)
                return false;
        }
        return Arrays.binarySearch(m_items, ce.get()) >= 0;
    }

    public boolean empty() {
        return m_items.length == 0;
    }

    public int size() {
        return m_items.length;
    }

    public long[] edgeList() {
        return m_items;
    }

    private static CubeEdges findSolutionEdgeMulti(CornerOrientReprCubes ccoReprCubes,
                                                   CornerOrientReprCubes ccoReprSearchCubes,
                                                   List<EdgeReprCandidateTransform> otransform, boolean reversed) {
        for (long edges : ccoReprCubes.edgeList()) {
            CubeEdges ce = new CubeEdges(edges);
            CubeEdges ceSearchRepr = CubecornerReprPerms.getComposedReprCubeedges(ce, reversed, otransform);
            if (ccoReprSearchCubes.containsCubeEdges(ceSearchRepr))
                return ce;
        }
        return new CubeEdges();
    }

    private static CubeEdges findSolutionEdgeSingle(CornerOrientReprCubes ccoReprCubes,
                                                    CornerOrientReprCubes ccoReprSearchCubes,
                                                    EdgeReprCandidateTransform erct, boolean reversed) {
        CubeEdges cetrans = ctransformed[erct.transformedIdx].ce;
        CubeEdges cetransRev = ctransformed[transformReverse(erct.transformedIdx)].ce;
        if (ccoReprCubes.size() <= ccoReprSearchCubes.size()) {
            for (long edges : ccoReprCubes.edgeList()) {
                CubeEdges ce = new CubeEdges(edges);
                CubeEdges cesymm = erct.symmetric ? ce.symmetric() : ce;
                CubeEdges ceSearchRepr;
                if (reversed) {
                    if (erct.reversed) ceSearchRepr = CubeEdges.compose3revmid(cetrans, cesymm, erct.ceTrans);
                    else ceSearchRepr = CubeEdges.compose3(erct.ceTrans, cesymm, cetransRev);
                } else {
                    if (erct.reversed) ceSearchRepr = CubeEdges.compose3revmid(erct.ceTrans, cesymm, cetransRev);
                    else ceSearchRepr = CubeEdges.compose3(cetrans, cesymm, erct.ceTrans);
                }
                if (ccoReprSearchCubes.containsCubeEdges(ceSearchRepr))
                    return ce;
            }
        } else {
            CubeEdges erctCeTransRev = erct.ceTrans.reverse();
            for (long edges : ccoReprSearchCubes.edgeList()) {
                CubeEdges ce = new CubeEdges(edges);
                CubeEdges ceSearch;
                if (reversed) {
                    if (erct.reversed) ceSearch = CubeEdges.compose3revmid(erct.ceTrans, ce, cetrans);
                    else ceSearch = CubeEdges.compose3(erctCeTransRev, ce, cetrans);
                } else {
                    if (erct.reversed) ceSearch = CubeEdges.compose3revmid(cetransRev, ce, erct.ceTrans);
                    else ceSearch = CubeEdges.compose3(cetransRev, ce, erctCeTransRev);
                }
                CubeEdges cesymmSearch = erct.symmetric ? ceSearch.symmetric() : ceSearch;
                if (ccoReprCubes.containsCubeEdges(cesymmSearch))
                    return cesymmSearch;
            }
        }
        return new CubeEdges();
    }

    public static CubeEdges findSolutionEdge(CornerOrientReprCubes ccoReprCubes,
                                             CornerOrientReprCubes ccoReprSearchCubes,
                                             List<EdgeReprCandidateTransform> otransform, boolean reversed) {
        if (otransform.size() == 1)
            return findSolutionEdgeSingle(ccoReprCubes, ccoReprSearchCubes, otransform.get(0), reversed);
        else
            return findSolutionEdgeMulti(ccoReprCubes, ccoReprSearchCubes, otransform, reversed);
    }
}
