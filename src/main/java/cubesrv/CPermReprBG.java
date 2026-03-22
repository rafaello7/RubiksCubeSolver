package cubesrv;

public class CPermReprBG {
    public static final int RCOUNTBG = 10;
    public static final int TCOUNTBG = 8;

    public static final RotateDir[] BGSpaceRotations = {
        RotateDir.ORANGE180, RotateDir.RED180, RotateDir.YELLOW180, RotateDir.WHITE180,
        RotateDir.GREENCW, RotateDir.GREEN180, RotateDir.GREENCCW,
        RotateDir.BLUECW, RotateDir.BLUE180, RotateDir.BLUECCW
    };

    public static final TransformDir[] BGSpaceTransforms = {
        TransformDir.TD_0, TransformDir.TD_BG_CW, TransformDir.TD_BG_180, TransformDir.TD_BG_CCW,
        TransformDir.TD_YW_180, TransformDir.TD_OR_180, TransformDir.TD_E4_7, TransformDir.TD_E5_6
    };
}
