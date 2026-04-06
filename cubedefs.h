#ifndef CUBEDEFS_H
#define CUBEDEFS_H

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

#ifdef __x86_64__
#define USE_ASM
//#define ASMCHECK
#endif

enum rotate_dir {
	ORANGECW, ORANGE180, ORANGECCW, REDCW, RED180, REDCCW,
	YELLOWCW, YELLOW180, YELLOWCCW, WHITECW, WHITE180, WHITECCW,
	GREENCW, GREEN180, GREENCCW, BLUECW, BLUE180, BLUECCW,
	RCOUNT
};

enum cubecolor {
	CYELLOW, CORANGE, CBLUE, CRED, CGREEN, CWHITE, CCOUNT
};

// The cube transformation (colors switch) through the cube rotation
enum transform_dir {
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
};

extern const int cubeCornerColors[8][3];
extern const int cubeEdgeColors[12][2];

extern const char ASM_SETUP[];
const char *rotateDirName(int rd);
int rotateNameToDir(const char *rotateName);
const char *transformName(unsigned);
int rotateDirReverse(int rd);
int transformReverse(int idx);

#endif // CUBEDEFS_H
