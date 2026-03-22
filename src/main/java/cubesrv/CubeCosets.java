package cubesrv;

public class CubeCosets {
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
