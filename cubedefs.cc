#include "cubedefs.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <string>

#ifdef ASMCHECK
static std::string dumpedges(unsigned long edges) {
    std::ostringstream res;
    res << std::hex << std::setw(15) << edges << "  ";
    for(int i = 63; i >= 0; --i) {
        res << (edges & 1ul << i ? "1" : "0");
        if( i && i % 5 == 0 )
            res << "|";
    }
    return res.str();
}
#endif

const char ASM_SETUP[] =
#ifdef USE_ASM
    " asm"
#endif
#ifdef ASMCHECK
    " with check"
#endif
    "";

const int cubeCornerColors[8][3] = {
  { CBLUE,   CORANGE, CYELLOW }, { CBLUE,  CYELLOW, CRED },
  { CBLUE,   CWHITE,  CORANGE }, { CBLUE,  CRED,    CWHITE },
  { CGREEN,  CYELLOW, CORANGE }, { CGREEN, CRED,    CYELLOW },
  { CGREEN,  CORANGE, CWHITE  }, { CGREEN, CWHITE,  CRED }
};

const int cubeEdgeColors[12][2] = {
  { CBLUE,   CYELLOW },
  { CORANGE, CBLUE   }, { CRED,    CBLUE },
  { CBLUE,   CWHITE  },
  { CYELLOW, CORANGE }, { CYELLOW, CRED },
  { CWHITE,  CORANGE }, { CWHITE,  CRED },
  { CGREEN,  CYELLOW },
  { CORANGE, CGREEN  }, { CRED,    CGREEN },
  { CGREEN,  CWHITE  }
};

const char *rotateDirName(int rd) {
	switch( rd ) {
	case ORANGECW:  return "orange-cw";
	case ORANGE180: return "orange-180";
	case ORANGECCW: return "orange-ccw";
	case REDCW:  return "red-cw";
	case RED180: return "red-180";
	case REDCCW: return "red-ccw";
	case YELLOWCW:  return "yellow-cw";
	case YELLOW180: return "yellow-180";
	case YELLOWCCW: return "yellow-ccw";
	case WHITECW:  return "white-cw";
	case WHITE180: return "white-180";
	case WHITECCW: return "white-ccw";
	case GREENCW:  return "green-cw";
	case GREEN180: return "green-180";
	case GREENCCW: return "green-ccw";
	case BLUECW:  return "blue-cw";
	case BLUE180: return "blue-180";
	case BLUECCW: return "blue-ccw";
	}
	static char buf[20];
	sprintf(buf, "%d", rd);
	return buf;
}

int rotateNameToDir(const char *rotateName) {
    int rd;
    for(rd = 0; rd < RCOUNT; ++rd) {
        const char *dirName = rotateDirName(rd);
        if( !strncmp(rotateName, dirName, strlen(dirName) ) )
            break;
    }
    return rd;
}

int rotateDirReverse(int rd) {
	switch( rd ) {
	case ORANGECW:  return ORANGECCW;
	case ORANGE180: return ORANGE180;
	case ORANGECCW: return ORANGECW;
	case REDCW:  return REDCCW;
	case RED180: return RED180;
	case REDCCW: return REDCW;
	case YELLOWCW:  return YELLOWCCW;
	case YELLOW180: return YELLOW180;
	case YELLOWCCW: return YELLOWCW;
	case WHITECW:  return WHITECCW;
	case WHITE180: return WHITE180;
	case WHITECCW: return WHITECW;
	case GREENCW:  return GREENCCW;
	case GREEN180: return GREEN180;
	case GREENCCW: return GREENCW;
	case BLUECW:  return BLUECCW;
	case BLUE180: return BLUE180;
	case BLUECCW: return BLUECW;
	}
	return RCOUNT;
}

int transformReverse(int idx) {
    switch( idx ) {
        case TD_C0_7_CW: return  TD_C0_7_CCW;
        case TD_C0_7_CCW: return  TD_C0_7_CW;
        case TD_C1_6_CW: return  TD_C1_6_CCW;
        case TD_C1_6_CCW: return  TD_C1_6_CW;
        case TD_C2_5_CW: return  TD_C2_5_CCW;
        case TD_C2_5_CCW: return  TD_C2_5_CW;
        case TD_C3_4_CW: return  TD_C3_4_CCW;
        case TD_C3_4_CCW: return  TD_C3_4_CW;
        case TD_BG_CW: return  TD_BG_CCW;
        case TD_BG_CCW: return TD_BG_CW;
        case TD_YW_CW: return TD_YW_CCW;
        case TD_YW_CCW: return TD_YW_CW;
        case TD_OR_CW: return TD_OR_CCW;
        case TD_OR_CCW: return TD_OR_CW;
    }
    return idx;
}

const char *transformName(unsigned td) {
    switch( td ) {
    case TD_0: return "0";
    case TD_C0_7_CW: return "c0-7.cw";
    case TD_C0_7_CCW: return "c0-7.ccw";
    case TD_C1_6_CW: return "c1-6.cw";
    case TD_C1_6_CCW: return "c1-6.ccw";
    case TD_C2_5_CW: return "c2-5.cw";
    case TD_C2_5_CCW: return "c2-5.ccw";
    case TD_C3_4_CW: return "c3-4.cw";
    case TD_C3_4_CCW: return "c3-4.ccw";
    case TD_BG_CW: return "bg.cw";
    case TD_BG_180: return "bg.180";
    case TD_BG_CCW: return "bg.ccw";
    case TD_YW_CW: return "yw.cw";
    case TD_YW_180: return "yw.180";
    case TD_YW_CCW: return "yw.ccw";
    case TD_OR_CW: return "or.cw";
    case TD_OR_180: return "or.180";
    case TD_OR_CCW: return "or.ccw";
    case TD_E0_11: return "e0-11";
    case TD_E1_10: return "e1-10";
    case TD_E2_9: return "e2-9";
    case TD_E3_8: return "e3-8";
    case TD_E4_7: return "e4-7";
    case TD_E5_6: return "e5-6";
    }
    return "unknown";
}


/*
struct cubecorners {
	CornersPerm perm;
	CornersOrient orients;

    cubecorners(unsigned corner0perm, unsigned corner0orient,
            unsigned corner1perm, unsigned corner1orient,
            unsigned corner2perm, unsigned corner2orient,
            unsigned corner3perm, unsigned corner3orient,
            unsigned corner4perm, unsigned corner4orient,
            unsigned corner5perm, unsigned corner5orient,
            unsigned corner6perm, unsigned corner6orient,
            unsigned corner7perm, unsigned corner7orient)
        : perm(corner0perm, corner1perm, corner2perm, corner3perm, corner4perm, corner5perm,
                corner6perm, corner7perm),
        orients(corner0orient, corner1orient, corner2orient,
                corner3orient, corner4orient, corner5orient, corner6orient, corner7orient)
        {
        }
};
*/
