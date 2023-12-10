#include "cuberead.h"
#include <cstring>
#include <map>

static bool isCubeSolvable(Responder &responder, const cube &c)
{
    bool isSwapsOdd = false;
    unsigned permScanned = 0;
    for(int i = 0; i < 8; ++i) {
        if( permScanned & 1 << i )
            continue;
        permScanned |= 1 << i;
        int p = i;
        while( (p = c.ccp.getAt(p)) != i ) {
            if( permScanned & 1 << p ) {
                responder.message("corner perm %d is twice", p);
                return false;
            }
            permScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
    permScanned = 0;
    for(int i = 0; i < 12; ++i) {
        if( permScanned & 1 << i )
            continue;
        permScanned |= 1 << i;
        int p = i;
        while( (p = c.ce.getPermAt(p)) != i ) {
            if( permScanned & 1 << p ) {
                responder.message("edge perm %d is twice", p);
                return false;
            }
            permScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
	if( isSwapsOdd ) {
		responder.message("cube unsolvable due to permutation parity");
		return false;
	}
	unsigned sumOrient = 0;
	for(int i = 0; i < 8; ++i)
		sumOrient += c.cco.getAt(i);
	if( sumOrient % 3 ) {
		responder.message("cube unsolvable due to corner orientations");
		return false;
	}
	sumOrient = 0;
	for(int i = 0; i < 12; ++i)
		sumOrient += c.ce.getOrientAt(i);
	if( sumOrient % 2 ) {
		responder.message("cube unsolvable due to edge orientations");
		return false;
	}
    return true;
}

struct elemLoc {
	int wall;
    int row;
	int col;
};

bool cubeFromColorsOnSquares(Responder &responder, const char *squareColors, cube &c)
{
    static const unsigned char R120[3] = { 1, 2, 0 };
    static const unsigned char R240[3] = { 2, 0, 1 };
	int walls[6][3][3];
	const char colorLetters[] = "YOBRGW";
	static const elemLoc cornerLocMap[8][3] = {
		{ { 2, 0, 0 }, { 1, 0, 2 }, { 0, 2, 0 } },
		{ { 2, 0, 2 }, { 0, 2, 2 }, { 3, 0, 0 } },

		{ { 2, 2, 0 }, { 5, 0, 0 }, { 1, 2, 2 } },
		{ { 2, 2, 2 }, { 3, 2, 0 }, { 5, 0, 2 } },

		{ { 4, 0, 2 }, { 0, 0, 0 }, { 1, 0, 0 } },
		{ { 4, 0, 0 }, { 3, 0, 2 }, { 0, 0, 2 } },

		{ { 4, 2, 2 }, { 1, 2, 0 }, { 5, 2, 0 } },
		{ { 4, 2, 0 }, { 5, 2, 2 }, { 3, 2, 2 } },
	};
	static const elemLoc edgeLocMap[12][2] = {
		{ { 2, 0, 1 }, { 0, 2, 1 } },
		{ { 1, 1, 2 }, { 2, 1, 0 } },
		{ { 3, 1, 0 }, { 2, 1, 2 } },
		{ { 2, 2, 1 }, { 5, 0, 1 } },

		{ { 0, 1, 0 }, { 1, 0, 1 } },
		{ { 0, 1, 2 }, { 3, 0, 1 } },
		{ { 5, 1, 0 }, { 1, 2, 1 } },
		{ { 5, 1, 2 }, { 3, 2, 1 } },

		{ { 4, 0, 1 }, { 0, 0, 1 } },
		{ { 1, 1, 0 }, { 4, 1, 2 } },
		{ { 3, 1, 2 }, { 4, 1, 0 } },
		{ { 4, 2, 1 }, { 5, 2, 1 } },
	};
    for(int cno = 0; cno < 54; ++cno) {
        const char *lpos = strchr(colorLetters, toupper(squareColors[cno]));
        if(lpos == NULL) {
            responder.message("bad letter at column %d", cno);
            return false;
        }
        walls[cno / 9][cno % 9 / 3][cno % 3] = lpos - colorLetters;
    }
	for(int i = 0; i < 6; ++i) {
		if( walls[i][1][1] != i ) {
			responder.message("bad orientation: wall=%d exp=%c is=%c",
					i, colorLetters[i], colorLetters[walls[i][1][1]]);
			return false;
		}
	}
	for(int i = 0; i < 8; ++i) {
		bool match = false;
		for(int n = 0; n < 8; ++n) {
			const int *elemColors = cubeCornerColors[n];
			match = true;
			for(int r = 0; r < 3 && match; ++r) {
				const elemLoc &el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[r] )
					match = false;
			}
			if( match ) {
				c.ccp.setAt(i, n);
				c.cco.setAt(i, 0);
				break;
			}
			match = true;
			for(int r = 0; r < 3 && match; ++r) {
				const elemLoc &el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[R120[r]] )
					match = false;
			}
			if( match ) {
				c.ccp.setAt(i, n);
				c.cco.setAt(i, 1);
				break;
			}
			match = true;
			for(int r = 0; r < 3 && match; ++r) {
				const elemLoc &el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[R240[r]] )
					match = false;
			}
			if( match ) {
				c.ccp.setAt(i, n);
				c.cco.setAt(i, 2);
				break;
			}
		}
		if( ! match ) {
			responder.message("corner %d not found", i);
			return false;
		}
		for(int j = 0; j < i; ++j) {
			if( c.ccp.getAt(i) == c.ccp.getAt(j) ) {
				responder.message("corner %d is twice: at %d and %d",
						c.ccp.getAt(i), j, i);
				return false;
			}
		}
	}
	for(int i = 0; i < 12; ++i) {
		bool match = false;
		for(int n = 0; n < 12; ++n) {
			const int *elemColors = cubeEdgeColors[n];
			match = true;
			for(int r = 0; r < 2 && match; ++r) {
				const elemLoc &el = edgeLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[r] )
					match = false;
			}
			if( match ) {
				c.ce.setAt(i, n, 0);
				break;
			}
			match = true;
			for(int r = 0; r < 2 && match; ++r) {
				const elemLoc &el = edgeLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[!r] )
					match = false;
			}
			if( match ) {
				c.ce.setAt(i, n, 1);
				break;
			}
		}
		if( ! match ) {
			responder.message("edge %d not found", i);
			return false;
		}
		for(int j = 0; j < i; ++j) {
			if( c.ce.getPermAt(i) == c.ce.getPermAt(j) ) {
				responder.message("edge %d is twice: at %d and %d",
						c.ce.getPermAt(i), j, i);
				return false;
			}
		}
	}
    if( ! isCubeSolvable(responder, c) )
        return false;
    return true;
}

static bool cubeFromScrambleStr(Responder &responder, const char *scrambleStr, cube &c) {
    const char rotateMapFromExtFmt[][3] = {
        "B1", "B2", "B3", "F1", "F2", "F3", "U1", "U2", "U3",
        "D1", "D2", "D3", "R1", "R2", "R3", "L1", "L2", "L3"
    };
    c = csolved;
    unsigned i = 0;
    while(scrambleStr[i] && scrambleStr[i+1]) {
        if( isalnum(scrambleStr[i]) ) {
            unsigned rd = 0;
            while( rd < RCOUNT && (scrambleStr[i] != rotateMapFromExtFmt[rd][0] ||
                        scrambleStr[i+1] != rotateMapFromExtFmt[rd][1]) )
                ++rd;
            if( rd == RCOUNT ) {
                responder.message("Unkown move: %c%c", scrambleStr[i], scrambleStr[i+1]);
                return false;
            }
            c = cube::compose(c, crotated[rd]);
            i += 2;
        }else
            ++i;
    }
    return true;
}

static std::string mapColorsOnSqaresFromExt(const char *ext) {
    // external format:
    //     U                Yellow
    //   L F R B   =   Blue   Red   Green Orange
    //     D                 White
    // sequence:
    //    yellow   green    red      white    blue     orange
    //  UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB
    const unsigned squaremap[] = {
         2,  5,  8,  1,  4,  7,  0,  3,  6, // upper -> yellow
        45, 46, 47, 48, 49, 50, 51, 52, 53, // back -> orange
        36, 37, 38, 39, 40, 41, 42, 43, 44, // left -> blue
        18, 19, 20, 21, 22, 23, 24, 25, 26, // front -> red
         9, 10, 11, 12, 13, 14, 15, 16, 17, // right -> green
        33, 30, 27, 34, 31, 28, 35, 32, 29  // down -> white
    };
    std::map<char, char> colormap = {
        {'U', 'Y'}, {'R', 'G'}, {'F', 'R'},
        {'D', 'W'}, {'L', 'B'}, {'B', 'O'}
    };
    std::string res;
    for(unsigned i = 0; i < 54; ++i) {
        res += colormap[ext[squaremap[i]]];
    }
    return res;
}

bool cubeFromString(Responder &responder, const char *cubeStr, cube &c) {
    bool res = false;
    unsigned len = strlen(cubeStr);
    if( strspn(cubeStr, "YOBRGW") == 54 )
        res = cubeFromColorsOnSquares(responder, cubeStr, c);
    else if( strspn(cubeStr, "URFDLB") == 54 ) {
        std::string fromExt = mapColorsOnSqaresFromExt(cubeStr);
        res = cubeFromColorsOnSquares(responder, fromExt.c_str(), c);
    }else if( strspn(cubeStr, "URFDLB123 \t\r\n") == len )
        res = cubeFromScrambleStr(responder, cubeStr, c);
    else
        responder.message("cube string format not recognized: %s", cubeStr);
    return res;
}

