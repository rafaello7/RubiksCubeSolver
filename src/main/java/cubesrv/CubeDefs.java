package cubesrv;
// The cube corner and edge numbers
//           ___________
//           | YELLOW  |
//           | 4  8  5 |
//           | 4     5 |
//           | 0  0  1 |
//  _________|_________|_________ _________
// |  ORANGE |   BLUE  |   RED   |  GREEN  |
// | 4  4  0 | 0  0  1 | 1  5  5 | 5  8  4 |
// | 9     1 | 1     2 | 2    10 | 10    9 |
// | 6  6  2 | 2  3  3 | 3  7  7 | 7 11  6 |
// |_________|_________|_________|_________|
//           |  WHITE  |
//           | 2  3  3 |
//           | 6     7 |
//           | 6 11  7 |
//           |_________|
//
// The cube corner and edge orientation referential squares
// if a referential square of a corner/edge being moved goes into a
// referential square, the corner/edge orientation is not changed
//           ___________
//           | YELLOW  |
//           |         |
//           | e     e |
//           |         |
//  _________|_________|_________ _________
// |  ORANGE |   BLUE  |   RED   |  GREEN  |
// |         | c  e  c |         | c  e  c |
// | e     e |         | e     e |         |
// |         | c  e  c |         | c  e  c |
// |_________|_________|_________|_________|
//           |  WHITE  |
//           |         |
//           | e     e |
//           |         |
//           |_________|

public class CubeDefs {
    public static final int[] R120 = {1, 2, 0};
    public static final int[] R240 = {2, 0, 1};

    public static final CubeColor[][] cubeCornerColors = {
        {CubeColor.CBLUE,   CubeColor.CORANGE, CubeColor.CYELLOW},
        {CubeColor.CBLUE,   CubeColor.CYELLOW, CubeColor.CRED},
        {CubeColor.CBLUE,   CubeColor.CWHITE,  CubeColor.CORANGE},
        {CubeColor.CBLUE,   CubeColor.CRED,    CubeColor.CWHITE},
        {CubeColor.CGREEN,  CubeColor.CYELLOW, CubeColor.CORANGE},
        {CubeColor.CGREEN,  CubeColor.CRED,    CubeColor.CYELLOW},
        {CubeColor.CGREEN,  CubeColor.CORANGE, CubeColor.CWHITE},
        {CubeColor.CGREEN,  CubeColor.CWHITE,  CubeColor.CRED}
    };

    public static final CubeColor[][] cubeEdgeColors = {
        {CubeColor.CBLUE,   CubeColor.CYELLOW},
        {CubeColor.CORANGE, CubeColor.CBLUE},
        {CubeColor.CRED,    CubeColor.CBLUE},
        {CubeColor.CBLUE,   CubeColor.CWHITE},
        {CubeColor.CYELLOW, CubeColor.CORANGE},
        {CubeColor.CYELLOW, CubeColor.CRED},
        {CubeColor.CWHITE,  CubeColor.CORANGE},
        {CubeColor.CWHITE,  CubeColor.CRED},
        {CubeColor.CGREEN,  CubeColor.CYELLOW},
        {CubeColor.CORANGE, CubeColor.CGREEN},
        {CubeColor.CRED,    CubeColor.CGREEN},
        {CubeColor.CGREEN,  CubeColor.CWHITE}
    };

    public static final Cube csolved = new Cube(
        new CubecornersPerm(0,1,2,3,4,5,6,7),
        new CubecornerOrients(0,0,0,0,0,0,0,0),
        new CubeEdges(0,1,2,3,4,5,6,7,8,9,10,11, 0,0,0,0,0,0,0,0,0,0,0,0));

    public static final Cube[] crotated = {
        new Cube( // ORANGECW
            new CubecornersPerm(4,1,0,3,6,5,2,7),
            new CubecornerOrients(1,0,2,0,2,0,1,0),
            new CubeEdges(0,4,2,3,9,5,1,7,8,6,10,11, 0,1,0,0,1,0,1,0,0,1,0,0)),
        new Cube( // ORANGE180
            new CubecornersPerm(6,1,4,3,2,5,0,7),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(0,9,2,3,6,5,4,7,8,1,10,11, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // ORANGECCW
            new CubecornersPerm(2,1,6,3,0,5,4,7),
            new CubecornerOrients(1,0,2,0,2,0,1,0),
            new CubeEdges(0,6,2,3,1,5,9,7,8,4,10,11, 0,1,0,0,1,0,1,0,0,1,0,0)),
        new Cube( // REDCW
            new CubecornersPerm(0,3,2,7,4,1,6,5),
            new CubecornerOrients(0,2,0,1,0,1,0,2),
            new CubeEdges(0,1,7,3,4,2,6,10,8,9,5,11, 0,0,1,0,0,1,0,1,0,0,1,0)),
        new Cube( // RED180
            new CubecornersPerm(0,7,2,5,4,3,6,1),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(0,1,10,3,4,7,6,5,8,9,2,11, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // REDCCW
            new CubecornersPerm(0,5,2,1,4,7,6,3),
            new CubecornerOrients(0,2,0,1,0,1,0,2),
            new CubeEdges(0,1,5,3,4,10,6,2,8,9,7,11, 0,0,1,0,0,1,0,1,0,0,1,0)),
        new Cube( // YELLOWCW
            new CubecornersPerm(1,5,2,3,0,4,6,7),
            new CubecornerOrients(2,1,0,0,1,2,0,0),
            new CubeEdges(5,1,2,3,0,8,6,7,4,9,10,11, 1,0,0,0,1,1,0,0,1,0,0,0)),
        new Cube( // YELLOW180
            new CubecornersPerm(5,4,2,3,1,0,6,7),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(8,1,2,3,5,4,6,7,0,9,10,11, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // YELLOWCCW
            new CubecornersPerm(4,0,2,3,5,1,6,7),
            new CubecornerOrients(2,1,0,0,1,2,0,0),
            new CubeEdges(4,1,2,3,8,0,6,7,5,9,10,11, 1,0,0,0,1,1,0,0,1,0,0,0)),
        new Cube( // WHITECW
            new CubecornersPerm(0,1,6,2,4,5,7,3),
            new CubecornerOrients(0,0,1,2,0,0,2,1),
            new CubeEdges(0,1,2,6,4,5,11,3,8,9,10,7, 0,0,0,1,0,0,1,1,0,0,0,1)),
        new Cube( // WHITE180
            new CubecornersPerm(0,1,7,6,4,5,3,2),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(0,1,2,11,4,5,7,6,8,9,10,3, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // WHITECCW
            new CubecornersPerm(0,1,3,7,4,5,2,6),
            new CubecornerOrients(0,0,1,2,0,0,2,1),
            new CubeEdges(0,1,2,7,4,5,3,11,8,9,10,6, 0,0,0,1,0,0,1,1,0,0,0,1)),
        new Cube( // GREENCW
            new CubecornersPerm(0,1,2,3,5,7,4,6),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(0,1,2,3,4,5,6,7,10,8,11,9, 0,0,0,0,0,0,0,0,1,1,1,1)),
        new Cube( // GREEN180
            new CubecornersPerm(0,1,2,3,7,6,5,4),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(0,1,2,3,4,5,6,7,11,10,9,8, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // GREENCCW
            new CubecornersPerm(0,1,2,3,6,4,7,5),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(0,1,2,3,4,5,6,7,9,11,8,10, 0,0,0,0,0,0,0,0,1,1,1,1)),
        new Cube( // BLUECW
            new CubecornersPerm(2,0,3,1,4,5,6,7),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(1,3,0,2,4,5,6,7,8,9,10,11, 1,1,1,1,0,0,0,0,0,0,0,0)),
        new Cube( // BLUE180
            new CubecornersPerm(3,2,1,0,4,5,6,7),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(3,2,1,0,4,5,6,7,8,9,10,11, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // BLUECCW
            new CubecornersPerm(1,3,0,2,4,5,6,7),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(2,0,3,1,4,5,6,7,8,9,10,11, 1,1,1,1,0,0,0,0,0,0,0,0))
    };

    public static final Cube[] ctransformed = {
        csolved,
        new Cube( // TD_C0_7_CW
            new CubecornersPerm(0,2,4,6,1,3,5,7),
            new CubecornerOrients(1,2,2,1,2,1,1,2),
            new CubeEdges(1,4,6,9,0,3,8,11,2,5,7,10, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_C0_7_CCW
            new CubecornersPerm(0,4,1,5,2,6,3,7),
            new CubecornerOrients(2,1,1,2,1,2,2,1),
            new CubeEdges(4,0,8,5,1,9,2,10,6,3,11,7, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_C1_6_CW
            new CubecornersPerm(5,1,4,0,7,3,6,2),
            new CubecornerOrients(2,1,1,2,1,2,2,1),
            new CubeEdges(5,8,0,4,10,2,9,1,7,11,3,6, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_C1_6_CCW
            new CubecornersPerm(3,1,7,5,2,0,6,4),
            new CubecornerOrients(1,2,2,1,2,1,1,2),
            new CubeEdges(2,7,5,10,3,0,11,8,1,6,4,9, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_C2_5_CW
            new CubecornersPerm(3,7,2,6,1,5,0,4),
            new CubecornerOrients(2,1,1,2,1,2,2,1),
            new CubeEdges(7,3,11,6,2,10,1,9,5,0,8,4, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_C2_5_CCW
            new CubecornersPerm(6,4,2,0,7,5,3,1),
            new CubecornerOrients(1,2,2,1,2,1,1,2),
            new CubeEdges(9,6,4,1,11,8,3,0,10,7,5,2, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_C3_4_CW
            new CubecornersPerm(5,7,1,3,4,6,0,2),
            new CubecornerOrients(1,2,2,1,2,1,1,2),
            new CubeEdges(10,5,7,2,8,11,0,3,9,4,6,1, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_C3_4_CCW
            new CubecornersPerm(6,2,7,3,4,0,5,1),
            new CubecornerOrients(2,1,1,2,1,2,2,1),
            new CubeEdges(6,11,3,7,9,1,10,2,4,8,0,5, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_BG_CW
            new CubecornersPerm(1,3,0,2,5,7,4,6),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(2,0,3,1,5,7,4,6,10,8,11,9, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_BG_180
            new CubecornersPerm(3,2,1,0,7,6,5,4),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(3,2,1,0,7,6,5,4,11,10,9,8, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_BG_CCW
            new CubecornersPerm(2,0,3,1,6,4,7,5),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(1,3,0,2,6,4,7,5,9,11,8,10, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_YW_CW
            new CubecornersPerm(4,0,6,2,5,1,7,3),
            new CubecornerOrients(2,1,1,2,1,2,2,1),
            new CubeEdges(4,9,1,6,8,0,11,3,5,10,2,7, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_YW_180
            new CubecornersPerm(5,4,7,6,1,0,3,2),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(8,10,9,11,5,4,7,6,0,2,1,3, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_YW_CCW
            new CubecornersPerm(1,5,3,7,0,4,2,6),
            new CubecornerOrients(2,1,1,2,1,2,2,1),
            new CubeEdges(5,2,10,7,0,8,3,11,4,1,9,6, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_OR_CW
            new CubecornersPerm(4,5,0,1,6,7,2,3),
            new CubecornerOrients(1,2,2,1,2,1,1,2),
            new CubeEdges(8,4,5,0,9,10,1,2,11,6,7,3, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_OR_180
            new CubecornersPerm(6,7,4,5,2,3,0,1),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(11,9,10,8,6,7,4,5,3,1,2,0, 0,0,0,0,0,0,0,0,0,0,0,0)),
        new Cube( // TD_OR_CCW
            new CubecornersPerm(2,3,6,7,0,1,4,5),
            new CubecornerOrients(1,2,2,1,2,1,1,2),
            new CubeEdges(3,6,7,11,1,2,9,10,0,4,5,8, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_E0_11
            new CubecornersPerm(1,0,5,4,3,2,7,6),
            new CubecornerOrients(1,2,2,1,2,1,1,2),
            new CubeEdges(0,5,4,8,2,1,10,9,3,7,6,11, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_E1_10
            new CubecornersPerm(2,6,0,4,3,7,1,5),
            new CubecornerOrients(2,1,1,2,1,2,2,1),
            new CubeEdges(6,1,9,4,3,11,0,8,7,2,10,5, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_E2_9
            new CubecornersPerm(7,3,5,1,6,2,4,0),
            new CubecornerOrients(2,1,1,2,1,2,2,1),
            new CubeEdges(7,10,2,5,11,3,8,0,6,9,1,4, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_E3_8
            new CubecornersPerm(7,6,3,2,5,4,1,0),
            new CubecornerOrients(1,2,2,1,2,1,1,2),
            new CubeEdges(11,7,6,3,10,9,2,1,8,5,4,0, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_E4_7
            new CubecornersPerm(4,6,5,7,0,2,1,3),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(9,8,11,10,4,6,5,7,1,0,3,2, 1,1,1,1,1,1,1,1,1,1,1,1)),
        new Cube( // TD_E5_6
            new CubecornersPerm(7,5,6,4,3,1,2,0),
            new CubecornerOrients(0,0,0,0,0,0,0,0),
            new CubeEdges(10,11,8,9,7,5,6,4,2,3,0,1, 1,1,1,1,1,1,1,1,1,1,1,1))
    };

    public static String rotateDirName(int rd) {
        return switch (RotateDir.values()[rd]) {
            case ORANGECW  -> "orange-cw";
            case ORANGE180 -> "orange-180";
            case ORANGECCW -> "orange-ccw";
            case REDCW     -> "red-cw";
            case RED180    -> "red-180";
            case REDCCW    -> "red-ccw";
            case YELLOWCW  -> "yellow-cw";
            case YELLOW180 -> "yellow-180";
            case YELLOWCCW -> "yellow-ccw";
            case WHITECW   -> "white-cw";
            case WHITE180  -> "white-180";
            case WHITECCW  -> "white-ccw";
            case GREENCW   -> "green-cw";
            case GREEN180  -> "green-180";
            case GREENCCW  -> "green-ccw";
            case BLUECW    -> "blue-cw";
            case BLUE180   -> "blue-180";
            case BLUECCW   -> "blue-ccw";
            default        -> String.valueOf(rd);
        };
    }

    public static int rotateNameToDir(String rotateName) {
        for (int rd = 0; rd < RotateDir.RCOUNT.ordinal(); rd++) {
            String dirName = rotateDirName(rd);
            if (rotateName.startsWith(dirName))
                return rd;
        }
        return RotateDir.RCOUNT.ordinal();
    }

    public static int rotateDirReverse(int rd) {
        return switch (RotateDir.values()[rd]) {
            case ORANGECW  -> RotateDir.ORANGECCW.ordinal();
            case ORANGE180 -> RotateDir.ORANGE180.ordinal();
            case ORANGECCW -> RotateDir.ORANGECW.ordinal();
            case REDCW     -> RotateDir.REDCCW.ordinal();
            case RED180    -> RotateDir.RED180.ordinal();
            case REDCCW    -> RotateDir.REDCW.ordinal();
            case YELLOWCW  -> RotateDir.YELLOWCCW.ordinal();
            case YELLOW180 -> RotateDir.YELLOW180.ordinal();
            case YELLOWCCW -> RotateDir.YELLOWCW.ordinal();
            case WHITECW   -> RotateDir.WHITECCW.ordinal();
            case WHITE180  -> RotateDir.WHITE180.ordinal();
            case WHITECCW  -> RotateDir.WHITECW.ordinal();
            case GREENCW   -> RotateDir.GREENCCW.ordinal();
            case GREEN180  -> RotateDir.GREEN180.ordinal();
            case GREENCCW  -> RotateDir.GREENCW.ordinal();
            case BLUECW    -> RotateDir.BLUECCW.ordinal();
            case BLUE180   -> RotateDir.BLUE180.ordinal();
            case BLUECCW   -> RotateDir.BLUECW.ordinal();
            default        -> RotateDir.RCOUNT.ordinal();
        };
    }

    public static int transformReverse(int idx) {
        TransformDir td = TransformDir.values()[idx];
        return switch (td) {
            case TD_C0_7_CW  -> TransformDir.TD_C0_7_CCW.ordinal();
            case TD_C0_7_CCW -> TransformDir.TD_C0_7_CW.ordinal();
            case TD_C1_6_CW  -> TransformDir.TD_C1_6_CCW.ordinal();
            case TD_C1_6_CCW -> TransformDir.TD_C1_6_CW.ordinal();
            case TD_C2_5_CW  -> TransformDir.TD_C2_5_CCW.ordinal();
            case TD_C2_5_CCW -> TransformDir.TD_C2_5_CW.ordinal();
            case TD_C3_4_CW  -> TransformDir.TD_C3_4_CCW.ordinal();
            case TD_C3_4_CCW -> TransformDir.TD_C3_4_CW.ordinal();
            case TD_BG_CW    -> TransformDir.TD_BG_CCW.ordinal();
            case TD_BG_CCW   -> TransformDir.TD_BG_CW.ordinal();
            case TD_YW_CW    -> TransformDir.TD_YW_CCW.ordinal();
            case TD_YW_CCW   -> TransformDir.TD_YW_CW.ordinal();
            case TD_OR_CW    -> TransformDir.TD_OR_CCW.ordinal();
            case TD_OR_CCW   -> TransformDir.TD_OR_CW.ordinal();
            default          -> idx;
        };
    }

    public static String transformName(int td) {
        return switch (TransformDir.values()[td]) {
            case TD_0       -> "0";
            case TD_C0_7_CW -> "c0-7.cw";
            case TD_C0_7_CCW-> "c0-7.ccw";
            case TD_C1_6_CW -> "c1-6.cw";
            case TD_C1_6_CCW-> "c1-6.ccw";
            case TD_C2_5_CW -> "c2-5.cw";
            case TD_C2_5_CCW-> "c2-5.ccw";
            case TD_C3_4_CW -> "c3-4.cw";
            case TD_C3_4_CCW-> "c3-4.ccw";
            case TD_BG_CW   -> "bg.cw";
            case TD_BG_180  -> "bg.180";
            case TD_BG_CCW  -> "bg.ccw";
            case TD_YW_CW   -> "yw.cw";
            case TD_YW_180  -> "yw.180";
            case TD_YW_CCW  -> "yw.ccw";
            case TD_OR_CW   -> "or.cw";
            case TD_OR_180  -> "or.180";
            case TD_OR_CCW  -> "or.ccw";
            case TD_E0_11   -> "e0-11";
            case TD_E1_10   -> "e1-10";
            case TD_E2_9    -> "e2-9";
            case TD_E3_8    -> "e3-8";
            case TD_E4_7    -> "e4-7";
            case TD_E5_6    -> "e5-6";
            default         -> "unknown";
        };
    }

    public static void cubePrint(Cube c) {
        String[] colorPrint = {
            "\u001B[48;2;230;230;0m  \u001B[m",
            "\u001B[48;2;230;148;0m  \u001B[m",
            "\u001B[48;2;0;0;230m  \u001B[m",
            "\u001B[48;2;230;0;0m  \u001B[m",
            "\u001B[48;2;0;230;0m  \u001B[m",
            "\u001B[48;2;230;230;230m  \u001B[m"
        };
        System.out.println();
        System.out.println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(4)][R120[c.cco.getAt(4)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(8)][1-c.ce.getOrientAt(8)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(5)][R240[c.cco.getAt(5)]].ordinal()]);
        System.out.println("        " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(4)][c.ce.getOrientAt(4)].ordinal()] +
            colorPrint[CubeColor.CYELLOW.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(5)][c.ce.getOrientAt(5)].ordinal()]);
        System.out.println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(0)][R240[c.cco.getAt(0)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(0)][1-c.ce.getOrientAt(0)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(1)][R120[c.cco.getAt(1)]].ordinal()]);
        System.out.println();
        System.out.println(" " +
            colorPrint[cubeCornerColors[c.ccp.getAt(4)][R240[c.cco.getAt(4)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(4)][1-c.ce.getOrientAt(4)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(0)][R120[c.cco.getAt(0)]].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(0)][c.cco.getAt(0)].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(0)][c.ce.getOrientAt(0)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(1)][c.cco.getAt(1)].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(1)][R240[c.cco.getAt(1)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(5)][1-c.ce.getOrientAt(5)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(5)][R120[c.cco.getAt(5)]].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(5)][c.cco.getAt(5)].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(8)][c.ce.getOrientAt(8)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(4)][c.cco.getAt(4)].ordinal()]);
        System.out.println(" " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(9)][c.ce.getOrientAt(9)].ordinal()] +
            colorPrint[CubeColor.CORANGE.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(1)][c.ce.getOrientAt(1)].ordinal()] + " " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(1)][1-c.ce.getOrientAt(1)].ordinal()] +
            colorPrint[CubeColor.CBLUE.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(2)][1-c.ce.getOrientAt(2)].ordinal()] + " " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(2)][c.ce.getOrientAt(2)].ordinal()] +
            colorPrint[CubeColor.CRED.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(10)][c.ce.getOrientAt(10)].ordinal()] + " " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(10)][1-c.ce.getOrientAt(10)].ordinal()] +
            colorPrint[CubeColor.CGREEN.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(9)][1-c.ce.getOrientAt(9)].ordinal()]);
        System.out.println(" " +
            colorPrint[cubeCornerColors[c.ccp.getAt(6)][R120[c.cco.getAt(6)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(6)][1-c.ce.getOrientAt(6)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(2)][R240[c.cco.getAt(2)]].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(2)][c.cco.getAt(2)].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(3)][c.ce.getOrientAt(3)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(3)][c.cco.getAt(3)].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(3)][R120[c.cco.getAt(3)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(7)][1-c.ce.getOrientAt(7)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(7)][R240[c.cco.getAt(7)]].ordinal()] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(7)][c.cco.getAt(7)].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(11)][c.ce.getOrientAt(11)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(6)][c.cco.getAt(6)].ordinal()]);
        System.out.println();
        System.out.println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(2)][R120[c.cco.getAt(2)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(3)][1-c.ce.getOrientAt(3)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(3)][R240[c.cco.getAt(3)]].ordinal()]);
        System.out.println("        " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(6)][c.ce.getOrientAt(6)].ordinal()] +
            colorPrint[CubeColor.CWHITE.ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(7)][c.ce.getOrientAt(7)].ordinal()]);
        System.out.println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(6)][R240[c.cco.getAt(6)]].ordinal()] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(11)][1-c.ce.getOrientAt(11)].ordinal()] +
            colorPrint[cubeCornerColors[c.ccp.getAt(7)][R120[c.cco.getAt(7)]].ordinal()]);
        System.out.println();
    }
}

