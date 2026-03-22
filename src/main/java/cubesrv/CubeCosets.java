package cubesrv;

import static cubesrv.CubeDefs.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class CubeCosets {

    public static class CubeCosetsAtDepth {
        @SuppressWarnings("unchecked")
        final Map<cubeedges, List<cube>>[] m_itemsArr = new HashMap[2187];

        public CubeCosetsAtDepth() {
            for (int i = 0; i < 2187; i++) m_itemsArr[i] = new HashMap<>();
        }

        public boolean addCube(int ccoReprIdx, cubeedges ceRepr, cube c) {
            Map<cubeedges, List<cube>> items = m_itemsArr[ccoReprIdx];
            List<cube> cubeList = items.computeIfAbsent(ceRepr, k -> new ArrayList<>());
            boolean res = cubeList.isEmpty();
            cubeList.add(c);
            return res;
        }

        public boolean containsCCOrients(int ccoReprIdx) {
            return !m_itemsArr[ccoReprIdx].isEmpty();
        }

        public List<cube> getCubesForCE(int ccoReprIdx, cubeedges ceRepr) {
            return m_itemsArr[ccoReprIdx].get(ceRepr);
        }
    }

    private final CubeCosetsAtDepth[] m_cubesAtDepths;
    private int m_availCount = 0;

    public CubeCosets(int size) {
        m_cubesAtDepths = new CubeCosetsAtDepth[size];
        for (int i = 0; i < size; i++) m_cubesAtDepths[i] = new CubeCosetsAtDepth();
    }

    public int availCount()    { return m_availCount; }
    public void incAvailCount(){ ++m_availCount; }
    public int availMaxCount() { return m_cubesAtDepths.length; }
    public CubeCosetsAtDepth getAt(int idx) { return m_cubesAtDepths[idx]; }
}
