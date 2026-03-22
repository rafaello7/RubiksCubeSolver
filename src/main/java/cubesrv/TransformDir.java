package cubesrv;

// The cube transformation (colors switch) through the cube rotation
public enum TransformDir {
    TD_0,           // no rotation
    TD_C0_7_CW,     // rotation along axis from corner 0 to 7, 120 degree clockwise
    TD_C0_7_CCW,    // rotation along axis from corner 0 to 7, counterclockwise
    TD_C1_6_CW,
    TD_C1_6_CCW,
    TD_C2_5_CW,
    TD_C2_5_CCW,
    TD_C3_4_CW,
    TD_C3_4_CCW,
    TD_BG_CW,       // rotation along axis through the wall middle, from blue to green wall, clockwise
    TD_BG_180,
    TD_BG_CCW,
    TD_YW_CW,
    TD_YW_180,
    TD_YW_CCW,
    TD_OR_CW,
    TD_OR_180,
    TD_OR_CCW,
    TD_E0_11,       // rotation along axis from edge 0 to 11, 180 degree
    TD_E1_10,
    TD_E2_9,
    TD_E3_8,
    TD_E4_7,
    TD_E5_6,
    TCOUNT
}
