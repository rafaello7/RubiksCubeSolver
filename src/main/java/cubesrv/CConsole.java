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

    private static cube generateCube() {
        Random rng = new Random();
        cube c = new cube(
            cubecorners_perm.fromPermIdx(rng.nextInt(40320)),
            cubecorner_orients.fromOrientIdx(rng.nextInt(2187)),
            cubeedges.fromPermAndOrientIdx(rng.nextInt(479001600), rng.nextInt(2048)));
        if (c.ccp.isPermParityOdd() != c.ce.isPermParityOdd()) {
            int p = c.ce.getPermAt(10);
            c.ce.setPermAt(10, c.ce.getPermAt(11));
            c.ce.setPermAt(11, p);
        }
        return c;
    }

    public static class ConsoleResponder extends Responder {
        private final int m_verboseLevel;
        String m_solution  = null;
        String m_movecount = null;
        private final long m_startNano = System.nanoTime();

        public ConsoleResponder(int verboseLevel) {
            m_verboseLevel = verboseLevel + 1;
        }

        public int durationTimeMs() {
            return (int)((System.nanoTime() - m_startNano) / 1_000_000L);
        }

        public String durationTime() {
            int ms = durationTimeMs();
            int minutes = ms / 60000;
            ms %= 60000;
            int seconds = ms / 1000;
            ms %= 1000;
            String res = (minutes != 0)
                ? minutes + ":" + String.format("%02d", seconds)
                : String.valueOf(seconds);
            res += String.format(".%03d", ms);
            return res;
        }

        @Override
        public void handleMessage(MessageType mt, String msg) {
            String pad = "                                                 ";
            pad = pad.substring(Math.min(msg.length(), pad.length()));
            switch (mt) {
                case MT_UNQUALIFIED -> {
                    if (m_verboseLevel > 0) {
                        System.out.print("\r" + durationTime() + " " + msg + " " + pad);
                        if (msg.startsWith("finished at ") || m_verboseLevel >= 3)
                            System.out.println();
                    }
                }
                case MT_PROGRESS -> {
                    if (m_verboseLevel > 1)
                        System.out.print("\r" + durationTime() + " " + msg + " " + pad);
                }
                case MT_MOVECOUNT -> {
                    System.out.println("\r" + durationTime() + " moves: " + msg + " " + pad);
                    m_movecount = msg;
                }
                case MT_SOLUTION -> {
                    System.out.println("\r" + durationTime() + " " + msg + " " + pad);
                    m_solution = msg;
                }
            }
        }

        public String getSolution()  { return m_solution; }
        public String getMoveCount() { return m_movecount; }
    }

    private static void solveCubeList(List<cube> cubes, String mode, int depthMax, boolean useReverse) {
        CubeSearcher cubeSearcher = new CubeSearcher(depthMax, useReverse);
        if (mode.equals("O")) {
            ConsoleResponder responder = new ConsoleResponder(2);
            cubeSearcher.fillCubes(responder);
            System.out.println();
        }
        TreeMap<String, int[]> moveCounters = new TreeMap<>();
        for (int i = 0; i < cubes.size(); i++) {
            cube c = cubes.get(i);
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
                    if (rd == rotate_dir.RCOUNT.ordinal()) {
                        System.out.println("fatal: unrecognized move " + s);
                        return;
                    }
                    c = cube.compose(c, crotated[rd]);
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
        List<cube> cubes = new ArrayList<>();
        ConsoleResponder responder = new ConsoleResponder(2);
        if (fnameOrCubeStr.endsWith(".txt")) {
            try {
                solveCubesFromFile(fnameOrCubeStr, responder, cubes);
            } catch (IOException e) {
                System.out.println("error reading file: " + e.getMessage());
                return;
            }
        } else {
            cube[] cv = {new cube()};
            if (!cubeFromString(responder, fnameOrCubeStr, cv)) return;
            cubes.add(cv[0]);
        }
        solveCubeList(cubes, mode, depthMax, useReverse);
    }

    public static void cubeTester(int cubeCount, String mode, int depthMax, boolean useReverse) {
        List<cube> cubes = new ArrayList<>();
        for (int i = 0; i < cubeCount; i++) cubes.add(generateCube());
        solveCubeList(cubes, mode, depthMax, useReverse);
    }
}

