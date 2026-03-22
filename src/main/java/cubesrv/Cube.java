package cubesrv;

public class Cube implements Comparable<Cube> {
    public CubecornersPerm ccp;
    public CubecornerOrients cco;
    public CubeEdges ce;

    public Cube() {
        ccp = new CubecornersPerm();
        cco = new CubecornerOrients();
        ce = new CubeEdges();
    }

    public Cube(CubecornersPerm ccp, CubecornerOrients cco, CubeEdges ce) {
        this.ccp = ccp;
        this.cco = cco;
        this.ce = ce;
    }

    public static Cube compose(Cube c1, Cube c2) {
        return new Cube(
                CubecornersPerm.compose(c1.ccp, c2.ccp),
                CubecornerOrients.compose(c1.cco, c2.ccp, c2.cco),
                CubeEdges.compose(c1.ce, c2.ce));
    }

    public Cube symmetric() {
        return new Cube(ccp.symmetric(), cco.symmetric(), ce.symmetric());
    }

    public Cube reverse() {
        return new Cube(ccp.reverse(), cco.reverse(ccp), ce.reverse());
    }

    public Cube transform(int transformDir) {
        Cube ctrans = CubeDefs.ctransformed[transformDir];
        CubecornersPerm ccp1 = CubecornersPerm.compose(ctrans.ccp, this.ccp);
        CubecornerOrients cco1 = CubecornerOrients.compose(ctrans.cco, this.ccp, this.cco);
        CubeEdges ce1 = CubeEdges.compose(ctrans.ce, this.ce);

        Cube ctransRev = CubeDefs.ctransformed[CubeDefs.transformReverse(transformDir)];
        CubecornersPerm ccp2 = CubecornersPerm.compose(ccp1, ctransRev.ccp);
        CubecornerOrients cco2 = CubecornerOrients.compose(cco1, ctransRev.ccp, ctransRev.cco);
        CubeEdges ce2 = CubeEdges.compose(ce1, ctransRev.ce);
        return new Cube(ccp2, cco2, ce2);
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof Cube)) return false;
        Cube c = (Cube) o;
        return ccp.equals(c.ccp) && cco.equals(c.cco) && ce.equals(c.ce);
    }

    @Override
    public int hashCode() {
        return ccp.hashCode() * 31 * 31 + cco.hashCode() * 31 + ce.hashCode();
    }

    @Override
    public int compareTo(Cube o) {
        if (!ccp.equals(o.ccp)) return ccp.compareTo(o.ccp);
        if (!cco.equals(o.cco)) return cco.compareTo(o.cco);
        return ce.compareTo(o.ce);
    }

    public boolean isBGspace() {
        return cco.isBGspace() && ce.isBGspace();
    }

    public boolean isYWspace() {
        return cco.isYWspace(ccp) && ce.isYWspace();
    }

    public boolean isORspace() {
        return cco.isORspace(ccp) && ce.isORspace();
    }

    public Cube representativeBG() {
        return new Cube(CubeDefs.csolved.ccp, cco.representativeBG(ccp), ce.representativeBG());
    }

    public Cube representativeYW() {
        return new Cube(CubeDefs.csolved.ccp, cco.representativeYW(ccp), ce.representativeYW());
    }

    public Cube representativeOR() {
        return new Cube(CubeDefs.csolved.ccp, cco.representativeOR(ccp), ce.representativeOR());
    }

    public String toParamText() {
        String[] colorChars = {"Y", "O", "B", "R", "G", "W"};
        StringBuilder res = new StringBuilder();
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(4)][CubeDefs.R120[cco.getAt(4)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(8)][1 - ce.getOrientAt(8)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(5)][CubeDefs.R240[cco.getAt(5)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(4)][ce.getOrientAt(4)].ordinal()]);
        res.append('Y');
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(5)][ce.getOrientAt(5)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(0)][CubeDefs.R240[cco.getAt(0)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(0)][1 - ce.getOrientAt(0)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(1)][CubeDefs.R120[cco.getAt(1)]].ordinal()]);

        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(4)][CubeDefs.R240[cco.getAt(4)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(4)][1 - ce.getOrientAt(4)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(0)][CubeDefs.R120[cco.getAt(0)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(9)][ce.getOrientAt(9)].ordinal()]);
        res.append('O');
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(1)][ce.getOrientAt(1)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(6)][CubeDefs.R120[cco.getAt(6)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(6)][1 - ce.getOrientAt(6)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(2)][CubeDefs.R240[cco.getAt(2)]].ordinal()]);

        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(0)][cco.getAt(0)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(0)][ce.getOrientAt(0)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(1)][cco.getAt(1)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(1)][1 - ce.getOrientAt(1)].ordinal()]);
        res.append('B');
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(2)][1 - ce.getOrientAt(2)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(2)][cco.getAt(2)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(3)][ce.getOrientAt(3)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(3)][cco.getAt(3)].ordinal()]);

        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(1)][CubeDefs.R240[cco.getAt(1)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(5)][1 - ce.getOrientAt(5)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(5)][CubeDefs.R120[cco.getAt(5)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(2)][ce.getOrientAt(2)].ordinal()]);
        res.append('R');
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(10)][ce.getOrientAt(10)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(3)][CubeDefs.R120[cco.getAt(3)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(7)][1 - ce.getOrientAt(7)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(7)][CubeDefs.R240[cco.getAt(7)]].ordinal()]);

        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(5)][cco.getAt(5)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(8)][ce.getOrientAt(8)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(4)][cco.getAt(4)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(10)][1 - ce.getOrientAt(10)].ordinal()]);
        res.append('G');
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(9)][1 - ce.getOrientAt(9)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(7)][cco.getAt(7)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(11)][ce.getOrientAt(11)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(6)][cco.getAt(6)].ordinal()]);

        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(2)][CubeDefs.R120[cco.getAt(2)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(3)][1 - ce.getOrientAt(3)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(3)][CubeDefs.R240[cco.getAt(3)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(6)][ce.getOrientAt(6)].ordinal()]);
        res.append('W');
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(7)][ce.getOrientAt(7)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(6)][CubeDefs.R240[cco.getAt(6)]].ordinal()]);
        res.append(colorChars[CubeDefs.cubeEdgeColors[ce.getPermAt(11)][1 - ce.getOrientAt(11)].ordinal()]);
        res.append(colorChars[CubeDefs.cubeCornerColors[ccp.getAt(7)][CubeDefs.R120[cco.getAt(7)]].ordinal()]);
        return res.toString();
    }
}
