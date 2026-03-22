package cubesrv;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class CubeCosetsAtDepth {
    @SuppressWarnings("unchecked")
    final Map<CubeEdges, List<Cube>>[] m_itemsArr = new HashMap[2187];

    public CubeCosetsAtDepth() {
        for (int i = 0; i < 2187; i++) m_itemsArr[i] = new HashMap<>();
    }

    public boolean addCube(int ccoReprIdx, CubeEdges ceRepr, Cube c) {
        Map<CubeEdges, List<Cube>> items = m_itemsArr[ccoReprIdx];
        List<Cube> cubeList = items.computeIfAbsent(ceRepr, k -> new ArrayList<>());
        boolean res = cubeList.isEmpty();
        cubeList.add(c);
        return res;
    }

    public boolean containsCCOrients(int ccoReprIdx) {
        return !m_itemsArr[ccoReprIdx].isEmpty();
    }

    public List<Cube> getCubesForCE(int ccoReprIdx, CubeEdges ceRepr) {
        return m_itemsArr[ccoReprIdx].get(ceRepr);
    }
}
