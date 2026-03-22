package cubesrv;

import static cubesrv.CubeDefs.*;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.regex.Pattern;

public class CubeRead {

    private static boolean isCubeSolvable(Responder responder, cube c) {
        boolean isSwapsOdd = false;
        int permScanned = 0;
        for (int i = 0; i < 8; i++) {
            if ((permScanned & (1 << i)) != 0) continue;
            permScanned |= (1 << i);
            int p = i;
            while (true) {
                p = c.ccp.getAt(p);
                if (p == i) break;
                if ((permScanned & (1 << p)) != 0) {
                    responder.message("corner perm " + p + " is twice");
                    return false;
                }
                permScanned |= (1 << p);
                isSwapsOdd = !isSwapsOdd;
            }
        }
        permScanned = 0;
        for (int i = 0; i < 12; i++) {
            if ((permScanned & (1 << i)) != 0) continue;
            permScanned |= (1 << i);
            int p = i;
            while (true) {
                p = c.ce.getPermAt(p);
                if (p == i) break;
                if ((permScanned & (1 << p)) != 0) {
                    responder.message("edge perm " + p + " is twice");
                    return false;
                }
                permScanned |= (1 << p);
                isSwapsOdd = !isSwapsOdd;
            }
        }
        if (isSwapsOdd) {
            responder.message("cube unsolvable due to permutation parity");
            return false;
        }
        int sumOrient = 0;
        for (int i = 0; i < 8; i++) sumOrient += c.cco.getAt(i);
        if (sumOrient % 3 != 0) {
            responder.message("cube unsolvable due to corner orientations");
            return false;
        }
        sumOrient = 0;
        for (int i = 0; i < 12; i++) sumOrient += c.ce.getOrientAt(i);
        if (sumOrient % 2 != 0) {
            responder.message("cube unsolvable due to edge orientations");
            return false;
        }
        return true;
    }

    private static class elemLoc {
        final int wall, row, col;
        elemLoc(int wall, int row, int col) { this.wall = wall; this.row = row; this.col = col; }
    }

    public static boolean cubeFromColorsOnSquares(Responder responder, String squareColors, cube[] c) {
        int[] R120 = {1, 2, 0};
        int[] R240 = {2, 0, 1};
        cubecolor[][][] walls = new cubecolor[6][3][3];
        for (cubecolor[][] ww : walls) for (cubecolor[] wr : ww) java.util.Arrays.fill(wr, cubecolor.CCOUNT);
        String colorLetters = "YOBRGW";
        elemLoc[][] cornerLocMap = {
            {new elemLoc(2,0,0), new elemLoc(1,0,2), new elemLoc(0,2,0)},
            {new elemLoc(2,0,2), new elemLoc(0,2,2), new elemLoc(3,0,0)},
            {new elemLoc(2,2,0), new elemLoc(5,0,0), new elemLoc(1,2,2)},
            {new elemLoc(2,2,2), new elemLoc(3,2,0), new elemLoc(5,0,2)},
            {new elemLoc(4,0,2), new elemLoc(0,0,0), new elemLoc(1,0,0)},
            {new elemLoc(4,0,0), new elemLoc(3,0,2), new elemLoc(0,0,2)},
            {new elemLoc(4,2,2), new elemLoc(1,2,0), new elemLoc(5,2,0)},
            {new elemLoc(4,2,0), new elemLoc(5,2,2), new elemLoc(3,2,2)}
        };
        elemLoc[][] edgeLocMap = {
            {new elemLoc(2,0,1), new elemLoc(0,2,1)},
            {new elemLoc(1,1,2), new elemLoc(2,1,0)},
            {new elemLoc(3,1,0), new elemLoc(2,1,2)},
            {new elemLoc(2,2,1), new elemLoc(5,0,1)},
            {new elemLoc(0,1,0), new elemLoc(1,0,1)},
            {new elemLoc(0,1,2), new elemLoc(3,0,1)},
            {new elemLoc(5,1,0), new elemLoc(1,2,1)},
            {new elemLoc(5,1,2), new elemLoc(3,2,1)},
            {new elemLoc(4,0,1), new elemLoc(0,0,1)},
            {new elemLoc(1,1,0), new elemLoc(4,1,2)},
            {new elemLoc(3,1,2), new elemLoc(4,1,0)},
            {new elemLoc(4,2,1), new elemLoc(5,2,1)}
        };
        for (int cno = 0; cno <= 53; cno++) {
            int idx = colorLetters.indexOf(Character.toUpperCase(squareColors.charAt(cno)));
            if (idx < 0) {
                responder.message("bad letter at column " + cno);
                return false;
            }
            walls[cno/9][cno%9/3][cno%3] = cubecolor.values()[idx];
        }
        for (int i = 0; i <= 5; i++) {
            if (walls[i][1][1] != cubecolor.values()[i]) {
                responder.message("bad orientation: wall=" + i + " exp=" + colorLetters.charAt(i)
                        + " is=" + colorLetters.charAt(walls[i][1][1].ordinal()));
                return false;
            }
        }
        for (int i = 0; i <= 7; i++) {
            boolean match = false;
            for (int n = 0; n <= 7; n++) {
                cubecolor[] elemColors = cubeCornerColors[n];
                match = true;
                for (int r = 0; r <= 2; r++) {
                    elemLoc el = cornerLocMap[i][r];
                    if (walls[el.wall][el.row][el.col] != elemColors[r]) { match = false; break; }
                }
                if (match) { c[0].ccp.setAt(i, n); c[0].cco.setAt(i, 0); break; }
                match = true;
                for (int r = 0; r <= 2; r++) {
                    elemLoc el = cornerLocMap[i][r];
                    if (walls[el.wall][el.row][el.col] != elemColors[R120[r]]) { match = false; break; }
                }
                if (match) { c[0].ccp.setAt(i, n); c[0].cco.setAt(i, 1); break; }
                match = true;
                for (int r = 0; r <= 2; r++) {
                    elemLoc el = cornerLocMap[i][r];
                    if (walls[el.wall][el.row][el.col] != elemColors[R240[r]]) { match = false; break; }
                }
                if (match) { c[0].ccp.setAt(i, n); c[0].cco.setAt(i, 2); break; }
            }
            if (!match) { responder.message("corner " + i + " not found"); return false; }
            for (int j = 0; j < i; j++) {
                if (c[0].ccp.getAt(i) == c[0].ccp.getAt(j)) {
                    responder.message("corner " + c[0].ccp.getAt(i) + " is twice: at " + j + " and " + i);
                    return false;
                }
            }
        }
        for (int i = 0; i <= 11; i++) {
            boolean match = false;
            for (int n = 0; n <= 11; n++) {
                cubecolor[] elemColors = cubeEdgeColors[n];
                match = true;
                for (int r = 0; r <= 1; r++) {
                    elemLoc el = edgeLocMap[i][r];
                    if (walls[el.wall][el.row][el.col] != elemColors[r]) { match = false; break; }
                }
                if (match) { c[0].ce.setAt(i, n, 0); break; }
                match = true;
                for (int r = 0; r <= 1; r++) {
                    elemLoc el = edgeLocMap[i][r];
                    if (walls[el.wall][el.row][el.col] != elemColors[1-r]) { match = false; break; }
                }
                if (match) { c[0].ce.setAt(i, n, 1); break; }
            }
            if (!match) { responder.message("edge " + i + " not found"); return false; }
            for (int j = 0; j < i; j++) {
                if (c[0].ce.getPermAt(i) == c[0].ce.getPermAt(j)) {
                    responder.message("edge " + c[0].ce.getPermAt(i) + " is twice: at " + j + " and " + i);
                    return false;
                }
            }
        }
        if (!isCubeSolvable(responder, c[0])) return false;
        return true;
    }

    private static boolean cubeFromScrambleStr(Responder responder, String scrambleStr, cube[] c) {
        String[] rotateMapFromExtFmt = {
            "B1","B2","B3","F1","F2","F3","U1","U2","U3",
            "D1","D2","D3","R1","R2","R3","L1","L2","L3"
        };
        c[0] = csolved;
        int i = 0;
        while (i+1 < scrambleStr.length()) {
            String scramSub = scrambleStr.substring(i);
            if (Character.isLetterOrDigit(scramSub.charAt(0))) {
                int rd = 0;
                while (rd < rotate_dir.RCOUNT.ordinal() && !scramSub.startsWith(rotateMapFromExtFmt[rd]))
                    ++rd;
                if (rd == rotate_dir.RCOUNT.ordinal()) {
                    responder.message("Unknown move: " + scramSub.substring(0, 2));
                    return false;
                }
                c[0] = cube.compose(c[0], crotated[rd]);
                i += 2;
            } else {
                ++i;
            }
        }
        return true;
    }

    private static String mapColorsOnSquaresFromExt(String ext) {
        int[] squaremap = {
             2,  5,  8,  1,  4,  7,  0,  3,  6,
            45, 46, 47, 48, 49, 50, 51, 52, 53,
            36, 37, 38, 39, 40, 41, 42, 43, 44,
            18, 19, 20, 21, 22, 23, 24, 25, 26,
             9, 10, 11, 12, 13, 14, 15, 16, 17,
            33, 30, 27, 34, 31, 28, 35, 32, 29
        };
        java.util.Map<Character,Character> colormap = new java.util.HashMap<>();
        colormap.put('U','Y'); colormap.put('R','G'); colormap.put('F','R');
        colormap.put('D','W'); colormap.put('L','B'); colormap.put('B','O');
        StringBuilder res = new StringBuilder();
        for (int i = 0; i <= 53; i++)
            res.append(colormap.get(ext.charAt(squaremap[i])));
        return res.toString();
    }

    public static boolean cubeFromString(Responder responder, String cubeStr, cube[] c) {
        if (Pattern.matches("[YOBRGW]{54}", cubeStr))
            return cubeFromColorsOnSquares(responder, cubeStr, c);
        else if (Pattern.matches("[URFDLB]{54}", cubeStr)) {
            String fromExt = mapColorsOnSquaresFromExt(cubeStr);
            return cubeFromColorsOnSquares(responder, fromExt, c);
        } else if (Pattern.matches("[URFDLB123 \\t\\r\\n]+", cubeStr))
            return cubeFromScrambleStr(responder, cubeStr, c);
        else
            responder.message("cube string format not recognized: " + cubeStr);
        return false;
    }

    public static void solveCubesFromFile(String fname, Responder responder,
            java.util.List<cube> cubes) throws IOException {
        try (BufferedReader br = new BufferedReader(new FileReader(fname))) {
            String line;
            while ((line = br.readLine()) != null) {
                cube[] cv = {new cube()};
                if (cubeFromString(responder, line, cv))
                    cubes.add(cv[0]);
            }
        }
    }
}

