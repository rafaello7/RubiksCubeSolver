#ifndef CUBEDEFS_H
#define CUBEDEFS_H

#include <string>

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

class cubecorners_perm {
	unsigned perm;
public:
	cubecorners_perm() : perm(0) {}
	cubecorners_perm(unsigned corner0perm, unsigned corner1perm, unsigned corner2perm,
			unsigned corner3perm, unsigned corner4perm, unsigned corner5perm,
			unsigned corner6perm, unsigned corner7perm);

	void setAt(unsigned idx, unsigned char p) {
		perm &= ~(7 << 3*idx);
		perm |= p << 3*idx;
	}
	unsigned getAt(unsigned idx) const { return perm >> 3*idx & 7; }
	unsigned get() const { return perm; }
	void set(unsigned p) { perm = p; }
    static cubecorners_perm compose(cubecorners_perm, cubecorners_perm);
    static cubecorners_perm compose3(cubecorners_perm, cubecorners_perm, cubecorners_perm);
    cubecorners_perm symmetric() const {
        unsigned permRes = perm ^ 0x924924;
        cubecorners_perm ccpres;
        ccpres.perm = (permRes >> 12 | permRes << 12) & 0xffffff;
        return ccpres;
    }
    cubecorners_perm reverse() const;
    cubecorners_perm transform(unsigned transformDir) const;
	bool operator==(const cubecorners_perm &ccp) const { return perm == ccp.perm; }
	bool operator!=(const cubecorners_perm &ccp) const { return perm != ccp.perm; }
	bool operator<(const cubecorners_perm &ccp) const { return perm < ccp.perm; }
    unsigned short getPermIdx() const;
    static cubecorners_perm fromPermIdx(unsigned short);
    bool isPermParityOdd() const;
};

class cubecorner_orients {
	unsigned short orients;
public:
	cubecorner_orients() : orients(0) {}
	cubecorner_orients(unsigned corner0orient, unsigned corner1orient,
			unsigned corner2orient, unsigned corner3orient,
			unsigned corner4orient, unsigned corner5orient,
			unsigned corner6orient, unsigned corner7orient);

	void setAt(unsigned idx, unsigned char orient) {
		orients &= ~(3 << 2*idx);
		orients |= orient << 2*idx;
	}
	unsigned getAt(unsigned idx) const { return orients >> 2*idx & 3; }
	unsigned get() const { return orients; }
	void set(unsigned orts) { orients = orts; }
    static cubecorner_orients compose(cubecorner_orients cco1,
            cubecorners_perm ccp2, cubecorner_orients cco2);
    static cubecorner_orients compose3(cubecorner_orients cco1,
            cubecorners_perm ccp2, cubecorner_orients cco2,
            cubecorners_perm ccp3, cubecorner_orients cco3);
    cubecorner_orients symmetric() const {
        cubecorner_orients ccores;
        // set orient 2 -> 1, 1 -> 2, 0 unchanged
        unsigned short orie = (orients & 0xaaaa) >> 1 | (orients & 0x5555) << 1;
        ccores.orients = (orie >> 8 | orie << 8) & 0xffff;
        return ccores;
    }
    cubecorner_orients reverse(cubecorners_perm) const;
    cubecorner_orients transform(cubecorners_perm, unsigned transformDir) const;
	bool operator==(const cubecorner_orients &cco) const { return orients == cco.orients; }
	bool operator!=(const cubecorner_orients &cco) const { return orients != cco.orients; }
	bool operator<(const cubecorner_orients &cco) const { return orients < cco.orients; }
    unsigned short getOrientIdx() const;
    static cubecorner_orients fromOrientIdx(unsigned short);
    bool isBGspace() const;
    bool isYWspace(cubecorners_perm) const;
    bool isORspace(cubecorners_perm) const;
    cubecorner_orients representativeBG(cubecorners_perm) const;
    cubecorner_orients representativeYW(cubecorners_perm) const;
    cubecorner_orients representativeOR(cubecorners_perm) const;
};

class cubeedges {
	unsigned long edges;
public:
	cubeedges() : edges(0) {}
	cubeedges(unsigned edge0perm, unsigned edge1perm, unsigned edge2perm,
              unsigned edge3perm, unsigned edge4perm, unsigned edge5perm,
              unsigned edge6perm, unsigned edge7perm, unsigned edge8perm,
              unsigned edge9perm, unsigned edge10perm,unsigned edge11perm,
              unsigned edge0orient, unsigned edge1orient, unsigned edge2orient,
              unsigned edge3orient, unsigned edge4orient, unsigned edge5orient,
              unsigned edge6orient, unsigned edge7orient, unsigned edge8orient,
              unsigned edge9orient, unsigned edge10orient, unsigned edge11orient);

	void setAt(unsigned idx, unsigned char perm, unsigned char orient) {
		edges &= ~(0x1FUL << 5*idx);
		edges |= (unsigned long)(orient<<4 | perm) << 5*idx;
	}
	void setPermAt(unsigned idx, unsigned char perm) {
		edges &= ~(0xFUL << 5*idx);
		edges |= (unsigned long)perm << 5*idx;
	}
	void setOrientAt(unsigned idx, unsigned char orient) {
		edges &= ~(0x1UL << 5*idx+4);
		edges |= (unsigned long)orient << 5*idx+4;
	}
    static cubeedges compose(cubeedges, cubeedges);
    static cubeedges compose3(cubeedges, cubeedges, cubeedges);

    // the middle cubeedges should be reversed before compose
    static cubeedges compose3revmid(cubeedges, cubeedges, cubeedges);
    cubeedges symmetric() const {
        cubeedges ceres;
        // map perm: 0 1 2 3 4 5 6 7 8 9 10 11 -> 8 9 10 11 4 5 6 7 0 1 2 3
        unsigned long edg = edges ^ (~edges & 0x210842108421084ul) << 1;
        ceres.edges = edg >> 40 | edg & 0xfffff00000ul | (edg & 0xfffff) << 40;
        return ceres;
    }
    cubeedges reverse() const;
    cubeedges transform(int idx) const;
	unsigned getPermAt(unsigned idx) const { return edges >> 5 * idx & 0xf; }
	unsigned getOrientAt(unsigned idx) const { return edges >> (5*idx+4) & 1; }
	bool operator==(const cubeedges &ce) const { return edges == ce.edges; }
	bool operator!=(const cubeedges &ce) const { return edges != ce.edges; }
	bool operator<(const cubeedges &ce) const { return edges < ce.edges; }
    unsigned getPermIdx() const;
    unsigned short getOrientIdx() const;
    static cubeedges fromPermAndOrientIdx(unsigned permIdx, unsigned short orientIdx);
    bool isPermParityOdd() const;
    unsigned long get() const { return edges; }
    void set(unsigned long e) { edges = e; }
    bool isNil() const { return edges == 0; }
    bool isBGspace() const;
    bool isYWspace() const;
    bool isORspace() const;
    cubeedges representativeBG() const;
    cubeedges representativeYW() const;
    cubeedges representativeOR() const;
};

struct cube {
	cubecorners_perm ccp;
    cubecorner_orients cco;
	cubeedges ce;

    static cube compose(const cube &c1, const cube &c2) {
        return {
            .ccp = cubecorners_perm::compose(c1.ccp, c2.ccp),
            .cco = cubecorner_orients::compose(c1.cco, c2.ccp, c2.cco),
            .ce = cubeedges::compose(c1.ce, c2.ce)
        };
    }
    cube symmetric() const {
        return {
            .ccp = ccp.symmetric(),
            .cco = cco.symmetric(),
            .ce = ce.symmetric()
        };
    }
    cube reverse() const {
        return {
            .ccp = ccp.reverse(),
            .cco = cco.reverse(ccp),
            .ce = ce.reverse()
        };
    }
    cube transform(unsigned transformDir) const;
	bool operator==(const cube &c) const;
	bool operator!=(const cube &c) const;
	bool operator<(const cube &c) const;
    bool isBGspace() const { return cco.isBGspace() && ce.isBGspace(); }
    bool isYWspace() const { return cco.isYWspace(ccp) && ce.isYWspace(); }
    bool isORspace() const { return cco.isORspace(ccp) && ce.isORspace(); }
    cube representativeBG() const;
    cube representativeYW() const;
    cube representativeOR() const;
    std::string toParamText() const;
};

extern const char ASM_SETUP[];
extern const struct cube csolved;
extern const struct cube crotated[RCOUNT];
extern const struct cube ctransformed[TCOUNT];
const char *rotateDirName(int rd);
int rotateNameToDir(const char *rotateName);
int rotateDirReverse(int rd);
int transformReverse(int idx);
void cubePrint(const cube&);

#endif // CUBEDEFS_H
