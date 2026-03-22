package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.CubeRead.*;
import static cubesrv.CubeSearch.*;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.TreeMap;

public class CConsole {
    private static Cube generateCube() {
        Random rng = new Random();
        Cube c = new Cube(
            CubecornersPerm.fromPermIdx(rng.nextInt(40320)),
            CubecornerOrients.fromOrientIdx(rng.nextInt(2187)),
            CubeEdges.fromPermAndOrientIdx(rng.nextInt(479001600), rng.nextInt(2048)));
        if (c.ccp.isPermParityOdd() != c.ce.isPermParityOdd()) {
            int p = c.ce.getPermAt(10);
            c.ce.setPermAt(10, c.ce.getPermAt(11));
            c.ce.setPermAt(11, p);
        }
        return c;
    }

    private static void solveCubeList(List<Cube> cubes, String mode, int depthMax, boolean useReverse) {
        CubeSearcher cubeSearcher = new CubeSearcher(depthMax, useReverse);
        if (mode.equals("O")) {
            ConsoleResponder responder = new ConsoleResponder(2);
            cubeSearcher.fillCubes(responder);
            System.out.println();
        }
        TreeMap<String, int[]> moveCounters = new TreeMap<>();
        for (int i = 0; i < cubes.size(); i++) {
            Cube c = cubes.get(i);
            System.out.println(i + "  " + c.toParamText());
            System.out.println();
            cubePrint(c);
            ConsoleResponder responder = new ConsoleResponder(2);
            cubeSearcher.searchMoves(c, mode, responder);
            System.out.println();
            String solution = responder.getSolution() != null ? responder.getSolution() : "";
            for (String s : solution.split(" ")) {
                if (!s.isEmpty()) {
                    int rd = rotateNameToDir(s);
                    if (rd == RotateDir.RCOUNT.ordinal()) {
                        System.out.println("fatal: unrecognized move " + s);
                        return;
                    }
                    c = Cube.compose(c, crotated[rd]);
                }
            }
            if (!c.equals(csolved)) {
                System.out.println("fatal: bad solution!");
                return;
            }
            String moveCount = responder.getMoveCount();
            if (moveCount != null) {
                int[] ent = moveCounters.get(moveCount);
                if (ent == null) {
                    moveCounters.put(moveCount, new int[]{1, responder.durationTimeMs()});
                } else {
                    ++ent[0];
                    ent[1] += responder.durationTimeMs();
                }
            }
        }
        int durationTimeTot = 0;
        System.out.println("          moves  cubes  avg time (s)");
        System.out.println("---------------  -----  -------------");
        for (String mc : moveCounters.descendingKeySet()) {
            int[] stats = moveCounters.get(mc);
            System.out.printf("%15s %6d  %.4f%n", mc, stats[0], stats[1]/1000.0/stats[0]);
            durationTimeTot += stats[1];
        }
        System.out.println("---------------  -----  -------------");
        System.out.printf("total %7.0f s %6d  %.4f%n",
                durationTimeTot/1000.0, cubes.size(), durationTimeTot/1000.0/cubes.size());
        System.out.println();
    }

    public static void solveCubes(String fnameOrCubeStr, String mode, int depthMax, boolean useReverse) {
        List<Cube> cubes = new ArrayList<>();
        ConsoleResponder responder = new ConsoleResponder(2);
        if (fnameOrCubeStr.endsWith(".txt")) {
            try {
                solveCubesFromFile(fnameOrCubeStr, responder, cubes);
            } catch (IOException e) {
                System.out.println("error reading file: " + e.getMessage());
                return;
            }
        } else {
            Cube[] cv = {new Cube()};
            if (!cubeFromString(responder, fnameOrCubeStr, cv)) return;
            cubes.add(cv[0]);
        }
        solveCubeList(cubes, mode, depthMax, useReverse);
    }

    public static void cubeTester(int cubeCount, String mode, int depthMax, boolean useReverse) {
        List<Cube> cubes = new ArrayList<>();
        for (int i = 0; i < cubeCount; i++) cubes.add(generateCube());
        solveCubeList(cubes, mode, depthMax, useReverse);
    }
}

