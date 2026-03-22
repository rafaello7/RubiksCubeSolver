package cubesrv;

public class CubesReprAtDepth {
    private final CubecornerReprPerms m_reprPerms;
    private final CornerPermReprCubes[] m_cornerPermReprCubes;

    public CubesReprAtDepth(CubecornerReprPerms reprPerms) {
        this.m_reprPerms = reprPerms;
        m_cornerPermReprCubes = new CornerPermReprCubes[reprPerms.reprPermCount()];
        for (int i = 0; i < m_cornerPermReprCubes.length; i++)
            m_cornerPermReprCubes[i] = new CornerPermReprCubes();
    }

    public int size() {
        return m_cornerPermReprCubes.length;
    }

    public int cubeCount() {
        int res = 0;
        for (CornerPermReprCubes c : m_cornerPermReprCubes) res += c.cubeCount();
        return res;
    }

    public CornerPermReprCubes add(int idx) {
        return m_cornerPermReprCubes[idx];
    }

    public void initOccur(int idx) {
        m_cornerPermReprCubes[idx].initOccur();
    }

    public CornerPermReprCubes getAt(int idx) {
        return m_cornerPermReprCubes[idx];
    }

    public CornerPermReprCubes getFor(CubecornersPerm ccp) {
        int reprPermIdx = m_reprPerms.getReprPermIdx(ccp);
        return m_cornerPermReprCubes[reprPermIdx];
    }

    public CornerPermReprCubes[] ccpCubesList() {
        return m_cornerPermReprCubes;
    }

    public CubecornersPerm getPermAt(int reprPermIdx) {
        return m_reprPerms.getPermForIdx(reprPermIdx);
    }
}
