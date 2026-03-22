package cubesrv;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

public class CornerPermReprCubes {
    private static final CornerOrientReprCubes m_coreprCubesEmpty =
            new CornerOrientReprCubes(new CubecornerOrients());
    private final List<CornerOrientReprCubes> m_coreprCubes = new ArrayList<>();

    public boolean empty() {
        return m_coreprCubes.isEmpty();
    }

    public int size() {
        return m_coreprCubes.size();
    }

    public int cubeCount() {
        int res = 0;
        for (CornerOrientReprCubes it : m_coreprCubes) res += it.size();
        return res;
    }

    public void initOccur() {
        for (CornerOrientReprCubes cerepr : m_coreprCubes) cerepr.initOccur();
    }

    // Binary search by key extractor on a sorted list
    static int binarySearchByOrients(List<CornerOrientReprCubes> list, CubecornerOrients key) {
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

    public CornerOrientReprCubes cornerOrientCubesAt(CubecornerOrients cco) {
        int idx = binarySearchByOrients(m_coreprCubes, cco);
        return idx >= 0 ? m_coreprCubes.get(idx) : m_coreprCubesEmpty;
    }

    public CornerOrientReprCubes cornerOrientCubesAdd(CubecornerOrients cco) {
        int idx = binarySearchByOrients(m_coreprCubes, cco);
        if (idx < 0) {
            idx = -1 - idx;
            m_coreprCubes.add(idx, new CornerOrientReprCubes(cco));
        }
        return m_coreprCubes.get(idx);
    }

    public Collection<CornerOrientReprCubes> ccoCubesList() {
        return m_coreprCubes;
    }
}
