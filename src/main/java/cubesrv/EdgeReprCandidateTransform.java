package cubesrv;

import static cubesrv.CubeDefs.csolved;

public class EdgeReprCandidateTransform {
    public int transformedIdx;
    public boolean reversed;
    public boolean symmetric;
    public CubeEdges ceTrans;

    public EdgeReprCandidateTransform(int transformedIdx, boolean reversed, boolean symmetric) {
        this.transformedIdx = transformedIdx;
        this.reversed = reversed;
        this.symmetric = symmetric;
        this.ceTrans = csolved.ce;
    }

    public EdgeReprCandidateTransform(int transformedIdx, boolean reversed, boolean symmetric, CubeEdges ceTrans) {
        this.transformedIdx = transformedIdx;
        this.reversed = reversed;
        this.symmetric = symmetric;
        this.ceTrans = ceTrans;
    }
}
