#include "cube.h"

static const unsigned char R120[3] = { 1, 2, 0 };
static const unsigned char R240[3] = { 2, 0, 1 };

bool cube::operator==(const cube &c) const
{
	return ccp == c.ccp && cco == c.cco && ce == c.ce;
}

bool cube::operator!=(const cube &c) const
{
	return ccp != c.ccp || cco != c.cco || ce != c.ce;
}

bool cube::operator<(const cube &c) const
{
	return ccp < c.ccp || ccp == c.ccp && cco < c.cco ||
        ccp == c.ccp && cco == c.cco && ce < c.ce;
}

const struct cube csolved = {
    .ccp =   cubecorners_perm(0, 1, 2, 3, 4, 5, 6, 7),
    .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
    .ce  = cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
};

cube cube::representativeBG() const {
    cubecorner_orients ccoRepr = cco.representativeBG(ccp);
    cubeedges ceRepr = ce.representativeBG();
    return { .ccp = csolved.ccp, .cco = ccoRepr, .ce = ceRepr };
}

cube cube::representativeYW() const {
    cubecorner_orients ccoRepr = cco.representativeYW(ccp);
    cubeedges ceRepr = ce.representativeYW();
    return { .ccp = csolved.ccp, .cco = ccoRepr, .ce = ceRepr };
}

cube cube::representativeOR() const {
    cubecorner_orients ccoRepr = cco.representativeOR(ccp);
    cubeedges ceRepr = ce.representativeOR();
    return { .ccp = csolved.ccp, .cco = ccoRepr, .ce = ceRepr };
}

std::string cube::toParamText() const
{
    const char *colorChars = "YOBRGW";
    std::string res;

    res += colorChars[cubeCornerColors[ccp.getAt(4)][R120[cco.getAt(4)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(8)][1-ce.getOrientAt(8)]];
    res += colorChars[cubeCornerColors[ccp.getAt(5)][R240[cco.getAt(5)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(4)][ce.getOrientAt(4)]];
    res += 'Y';
    res += colorChars[cubeEdgeColors[ce.getPermAt(5)][ce.getOrientAt(5)]];
    res += colorChars[cubeCornerColors[ccp.getAt(0)][R240[cco.getAt(0)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(0)][1-ce.getOrientAt(0)]];
    res += colorChars[cubeCornerColors[ccp.getAt(1)][R120[cco.getAt(1)]]];

    res += colorChars[cubeCornerColors[ccp.getAt(4)][R240[cco.getAt(4)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(4)][1-ce.getOrientAt(4)]];
    res += colorChars[cubeCornerColors[ccp.getAt(0)][R120[cco.getAt(0)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(9)][ce.getOrientAt(9)]];
    res += 'O';
    res += colorChars[cubeEdgeColors[ce.getPermAt(1)][ce.getOrientAt(1)]];
    res += colorChars[cubeCornerColors[ccp.getAt(6)][R120[cco.getAt(6)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(6)][1-ce.getOrientAt(6)]];
    res += colorChars[cubeCornerColors[ccp.getAt(2)][R240[cco.getAt(2)]]];

    res += colorChars[cubeCornerColors[ccp.getAt(0)][cco.getAt(0)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(0)][ce.getOrientAt(0)]];
    res += colorChars[cubeCornerColors[ccp.getAt(1)][cco.getAt(1)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(1)][1-ce.getOrientAt(1)]];
    res += 'B';
    res += colorChars[cubeEdgeColors[ce.getPermAt(2)][1-ce.getOrientAt(2)]];
    res += colorChars[cubeCornerColors[ccp.getAt(2)][cco.getAt(2)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(3)][ce.getOrientAt(3)]];
    res += colorChars[cubeCornerColors[ccp.getAt(3)][cco.getAt(3)]];

    res += colorChars[cubeCornerColors[ccp.getAt(1)][R240[cco.getAt(1)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(5)][1-ce.getOrientAt(5)]];
    res += colorChars[cubeCornerColors[ccp.getAt(5)][R120[cco.getAt(5)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(2)][ce.getOrientAt(2)]];
    res += 'R';
    res += colorChars[cubeEdgeColors[ce.getPermAt(10)][ce.getOrientAt(10)]];
    res += colorChars[cubeCornerColors[ccp.getAt(3)][R120[cco.getAt(3)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(7)][1-ce.getOrientAt(7)]];
    res += colorChars[cubeCornerColors[ccp.getAt(7)][R240[cco.getAt(7)]]];

    res += colorChars[cubeCornerColors[ccp.getAt(5)][cco.getAt(5)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(8)][ce.getOrientAt(8)]];
    res += colorChars[cubeCornerColors[ccp.getAt(4)][cco.getAt(4)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(10)][1-ce.getOrientAt(10)]];
    res += 'G';
    res += colorChars[cubeEdgeColors[ce.getPermAt(9)][1-ce.getOrientAt(9)]];
    res += colorChars[cubeCornerColors[ccp.getAt(7)][cco.getAt(7)]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(11)][ce.getOrientAt(11)]];
    res += colorChars[cubeCornerColors[ccp.getAt(6)][cco.getAt(6)]];

    res += colorChars[cubeCornerColors[ccp.getAt(2)][R120[cco.getAt(2)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(3)][1-ce.getOrientAt(3)]];
    res += colorChars[cubeCornerColors[ccp.getAt(3)][R240[cco.getAt(3)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(6)][ce.getOrientAt(6)]];
    res += 'W';
    res += colorChars[cubeEdgeColors[ce.getPermAt(7)][ce.getOrientAt(7)]];
    res += colorChars[cubeCornerColors[ccp.getAt(6)][R240[cco.getAt(6)]]];
    res += colorChars[cubeEdgeColors[ce.getPermAt(11)][1-ce.getOrientAt(11)]];
    res += colorChars[cubeCornerColors[ccp.getAt(7)][R120[cco.getAt(7)]]];
    return res;
}

const struct cube crotated[RCOUNT] = {
    {   // ORANGECW
        .ccp =   cubecorners_perm(4, 1, 0, 3, 6, 5, 2, 7),
        .cco = cubecorner_orients(1, 0, 2, 0, 2, 0, 1, 0),
		.ce  = cubeedges(0, 4, 2, 3, 9, 5, 1, 7, 8, 6, 10, 11,
                         0, 1, 0, 0, 1, 0, 1, 0, 0, 1,  0,  0)
    },{ // ORANGE180
		.ccp =   cubecorners_perm(6, 1, 4, 3, 2, 5, 0, 7),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 9, 2, 3, 6, 5, 4, 7, 8, 1, 10, 11,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
    },{ // ORANGECCW
		.ccp =   cubecorners_perm(2, 1, 6, 3, 0, 5, 4, 7),
        .cco = cubecorner_orients(1, 0, 2, 0, 2, 0, 1, 0),
		.ce  = cubeedges(0, 6, 2, 3, 1, 5, 9, 7, 8, 4, 10, 11,
                         0, 1, 0, 0, 1, 0, 1, 0, 0, 1,  0,  0)
    },{ // REDCW
		.ccp =   cubecorners_perm(0, 3, 2, 7, 4, 1, 6, 5),
        .cco = cubecorner_orients(0, 2, 0, 1, 0, 1, 0, 2),
		.ce  = cubeedges(0, 1, 7, 3, 4, 2, 6, 10, 8, 9, 5, 11,
                         0, 0, 1, 0, 0, 1, 0,  1, 0, 0, 1,  0)
    },{ // RED180
		.ccp =   cubecorners_perm(0, 7, 2, 5, 4, 3, 6, 1),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 1, 10, 3, 4, 7, 6, 5, 8, 9, 2, 11,
                         0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  0)
    },{ // REDCCW
		.ccp =   cubecorners_perm(0, 5, 2, 1, 4, 7, 6, 3),
        .cco = cubecorner_orients(0, 2, 0, 1, 0, 1, 0, 2),
		.ce  = cubeedges(0, 1, 5, 3, 4, 10, 6, 2, 8, 9, 7, 11,
                         0, 0, 1, 0, 0,  1, 0, 1, 0, 0, 1,  0)
    },{ // YELLOWCW
		.ccp =   cubecorners_perm(1, 5, 2, 3, 0, 4, 6, 7),
        .cco = cubecorner_orients(2, 1, 0, 0, 1, 2, 0, 0),
		.ce  = cubeedges(5, 1, 2, 3, 0, 8, 6, 7, 4, 9, 10, 11,
                         1, 0, 0, 0, 1, 1, 0, 0, 1, 0,  0,  0)
    },{ // YELLOW180
		.ccp =   cubecorners_perm(5, 4, 2, 3, 1, 0, 6, 7),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(8, 1, 2, 3, 5, 4, 6, 7, 0, 9, 10, 11,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
    },{ // YELLOWCCW
		.ccp =   cubecorners_perm(4, 0, 2, 3, 5, 1, 6, 7),
        .cco = cubecorner_orients(2, 1, 0, 0, 1, 2, 0, 0),
		.ce  = cubeedges(4, 1, 2, 3, 8, 0, 6, 7, 5, 9, 10, 11,
                         1, 0, 0, 0, 1, 1, 0, 0, 1, 0,  0,  0)
    },{ // WHITECW
		.ccp =   cubecorners_perm(0, 1, 6, 2, 4, 5, 7, 3),
        .cco = cubecorner_orients(0, 0, 1, 2, 0, 0, 2, 1),
		.ce  = cubeedges(0, 1, 2, 6, 4, 5, 11, 3, 8, 9, 10, 7,
                         0, 0, 0, 1, 0, 0,  1, 1, 0, 0,  0, 1)
    },{ // WHITE180
		.ccp =   cubecorners_perm(0, 1, 7, 6, 4, 5, 3, 2),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 1, 2, 11, 4, 5, 7, 6, 8, 9, 10, 3,
                         0, 0, 0,  0, 0, 0, 0, 0, 0, 0,  0, 0)
    },{ // WHITECCW
		.ccp =   cubecorners_perm(0, 1, 3, 7, 4, 5, 2, 6),
        .cco = cubecorner_orients(0, 0, 1, 2, 0, 0, 2, 1),
		.ce  = cubeedges(0, 1, 2, 7, 4, 5, 3, 11, 8, 9, 10, 6,
                         0, 0, 0, 1, 0, 0, 1,  1, 0, 0,  0, 1)
    },{ // GREENCW
		.ccp =   cubecorners_perm(0, 1, 2, 3, 5, 7, 4, 6),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 10, 8, 11, 9,
                         0, 0, 0, 0, 0, 0, 0, 0,  1, 1,  1, 1)
    },{ // GREEN180
		.ccp =   cubecorners_perm(0, 1, 2, 3, 7, 6, 5, 4),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 11, 10, 9, 8,
                         0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0)
    },{ // GREENCCW
		.ccp =   cubecorners_perm(0, 1, 2, 3, 6, 4, 7, 5),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 8, 10,
                         0, 0, 0, 0, 0, 0, 0, 0, 1,  1, 1,  1)
    },{ // BLUECW
		.ccp =   cubecorners_perm(2, 0, 3, 1, 4, 5, 6, 7),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(1, 3, 0, 2, 4, 5, 6, 7, 8, 9, 10, 11,
                         1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  0, 0)
    },{ // BLUE180
		.ccp =   cubecorners_perm(3, 2, 1, 0, 4, 5, 6, 7),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(3, 2, 1, 0, 4, 5, 6, 7, 8, 9, 10, 11,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
    },{ // BLUECCW
		.ccp =   cubecorners_perm(1, 3, 0, 2, 4, 5, 6, 7),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(2, 0, 3, 1, 4, 5, 6, 7, 8, 9, 10, 11,
                         1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  0,  0)
	}
};

const struct cube ctransformed[TCOUNT] = {
    csolved,
    {   // TD_C0_7_CW
		.ccp =   cubecorners_perm(0, 2, 4, 6, 1, 3, 5, 7),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(1, 4, 6, 9, 0, 3, 8, 11, 2, 5, 7, 10,
                         0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0,  0)
    },{ // TD_C0_7_CCW
		.ccp =   cubecorners_perm(0, 4, 1, 5, 2, 6, 3, 7),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(4, 0, 8, 5, 1, 9, 2, 10, 6, 3, 11, 7,
                         0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0)
    },{ // TD_C1_6_CW
        .ccp =   cubecorners_perm(5, 1, 4, 0, 7, 3, 6, 2),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
        .ce  = cubeedges(5, 8, 0, 4, 10, 2, 9, 1, 7, 11, 3, 6,
                         0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0)
    },{ // TD_C1_6_CCW
		.ccp =   cubecorners_perm(3, 1, 7, 5, 2, 0, 6, 4),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(2, 7, 5, 10, 3, 0, 11, 8, 1, 6, 4, 9,
                         0, 0, 0,  0, 0, 0,  0, 0, 0, 0, 0, 0)
    },{ // TD_C2_5_CW
		.ccp =   cubecorners_perm(3, 7, 2, 6, 1, 5, 0, 4),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(7, 3, 11, 6, 2, 10, 1, 9, 5, 0, 8, 4,
                         0, 0,  0, 0, 0,  0, 0, 0, 0, 0, 0, 0)
    },{ // TD_C2_5_CCW
		.ccp =   cubecorners_perm(6, 4, 2, 0, 7, 5, 3, 1),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(9, 6, 4, 1, 11, 8, 3, 0, 10, 7, 5, 2,
                         0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0)
    },{ // TD_C3_4_CW
		.ccp =   cubecorners_perm(5, 7, 1, 3, 4, 6, 0, 2),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(10, 5, 7, 2, 8, 11, 0, 3, 9, 4, 6, 1,
                          0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0)
    },{ // TD_C3_4_CCW
		.ccp =   cubecorners_perm(6, 2, 7, 3, 4, 0, 5, 1),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(6, 11, 3, 7, 9, 1, 10, 2, 4, 8, 0, 5,
                         0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0)
    },{ // TD_BG_CW
		.ccp =   cubecorners_perm(1, 3, 0, 2, 5, 7, 4, 6),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(2, 0, 3, 1, 5, 7, 4, 6, 10, 8, 11, 9,
                         1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1)
    },{ // TD_BG_180
		.ccp =   cubecorners_perm(3, 2, 1, 0, 7, 6, 5, 4),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8,
                         0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0)
    },{ // TD_BG_CCW
		.ccp =   cubecorners_perm(2, 0, 3, 1, 6, 4, 7, 5),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(1, 3, 0, 2, 6, 4, 7, 5, 9, 11, 8, 10,
                         1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1,  1)
    },{ // TD_YW_CW
		.ccp =   cubecorners_perm(4, 0, 6, 2, 5, 1, 7, 3),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(4, 9, 1, 6, 8, 0, 11, 3, 5, 10, 2, 7,
                         1, 1, 1, 1, 1, 1,  1, 1, 1,  1, 1, 1)
    },{ // TD_YW_180
		.ccp =   cubecorners_perm(5, 4, 7, 6, 1, 0, 3, 2),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(8, 10, 9, 11, 5, 4, 7, 6, 0, 2, 1, 3,
                         0,  0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0)
    },{ // TD_YW_CCW
		.ccp =   cubecorners_perm(1, 5, 3, 7, 0, 4, 2, 6),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(5, 2, 10, 7, 0, 8, 3, 11, 4, 1, 9, 6,
                         1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1, 1)
    },{ // TD_OR_CW
		.ccp =   cubecorners_perm(4, 5, 0, 1, 6, 7, 2, 3),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(8, 4, 5, 0, 9, 10, 1, 2, 11, 6, 7, 3,
                         1, 1, 1, 1, 1,  1, 1, 1,  1, 1, 1, 1)
    },{ // TD_OR_180
		.ccp =   cubecorners_perm(6, 7, 4, 5, 2, 3, 0, 1),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(11, 9, 10, 8, 6, 7, 4, 5, 3, 1, 2, 0,
                          0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    },{ // TD_OR_CCW
		.ccp =   cubecorners_perm(2, 3, 6, 7, 0, 1, 4, 5),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(3, 6, 7, 11, 1, 2, 9, 10, 0, 4, 5, 8,
                         1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1)
    },{ // TD_E0_11
		.ccp =   cubecorners_perm(1, 0, 5, 4, 3, 2, 7, 6),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(0, 5, 4, 8, 2, 1, 10, 9, 3, 7, 6, 11,
                         1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1,  1)
    },{ // TD_E1_10
		.ccp =   cubecorners_perm(2, 6, 0, 4, 3, 7, 1, 5),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(6, 1, 9, 4, 3, 11, 0, 8, 7, 2, 10, 5,
                         1, 1, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1)
    },{ // TD_E2_9
		.ccp =   cubecorners_perm(7, 3, 5, 1, 6, 2, 4, 0),
        .cco = cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
		.ce  = cubeedges(7, 10, 2, 5, 11, 3, 8, 0, 6, 9, 1, 4,
                         1,  1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1)
    },{ // TD_E3_8
		.ccp =   cubecorners_perm(7, 6, 3, 2, 5, 4, 1, 0),
        .cco = cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
		.ce  = cubeedges(11, 7, 6, 3, 10, 9, 2, 1, 8, 5, 4, 0,
                          1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1)
    },{ // TD_E4_7
		.ccp =   cubecorners_perm(4, 6, 5, 7, 0, 2, 1, 3),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(9, 8, 11, 10, 4, 6, 5, 7, 1, 0, 3, 2,
                         1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
    },{ // TD_E5_6
		.ccp =   cubecorners_perm(7, 5, 6, 4, 3, 1, 2, 0),
        .cco = cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
		.ce  = cubeedges(10, 11, 8, 9, 7, 5, 6, 4, 2, 3, 0, 1,
                          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
	}
};

cube cube::transform(unsigned transformDir) const
{
    const cube &ctrans = ctransformed[transformDir];
    cubecorners_perm ccp1 = cubecorners_perm::compose(ctrans.ccp, this->ccp);
    cubecorner_orients cco1 = cubecorner_orients::compose(ctrans.cco, this->ccp, this->cco);
    cubeedges ce1 = cubeedges::compose(ctrans.ce, this->ce);

    const cube &ctransRev = ctransformed[transformReverse(transformDir)];
    cubecorners_perm ccp2 = cubecorners_perm::compose(ccp1, ctransRev.ccp);
    cubecorner_orients cco2 = cubecorner_orients::compose(cco1, ctransRev.ccp, ctransRev.cco);
    cubeedges ce2 = cubeedges::compose(ce1, ctransRev.ce);
	return { .ccp = ccp2, .cco = cco2, .ce = ce2 };
}

void cubePrint(const cube &c)
{
	const char *colorPrint[CCOUNT] = {
		"\033[48;2;230;230;0m  \033[m",		// CYELLOW
		"\033[48;2;230;148;0m  \033[m",		// CORANGE
		"\033[48;2;0;0;230m  \033[m",		// CBLUE
		"\033[48;2;230;0;0m  \033[m",		// CRED
		"\033[48;2;0;230;0m  \033[m",		// CGREEN
		"\033[48;2;230;230;230m  \033[m",	// CWHITE
	};

	printf("\n");
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(4)][R120[c.cco.getAt(4)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(8)][!c.ce.getOrientAt(8)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(5)][R240[c.cco.getAt(5)]]]);
	printf("        %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.getPermAt(4)][c.ce.getOrientAt(4)]],
			colorPrint[CYELLOW],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(5)][c.ce.getOrientAt(5)]]);
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(0)][R240[c.cco.getAt(0)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(0)][!c.ce.getOrientAt(0)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(1)][R120[c.cco.getAt(1)]]]);
	printf("\n");
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(4)][R240[c.cco.getAt(4)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(4)][!c.ce.getOrientAt(4)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(0)][R120[c.cco.getAt(0)]]],
			colorPrint[cubeCornerColors[c.ccp.getAt(0)][c.cco.getAt(0)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(0)][c.ce.getOrientAt(0)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(1)][c.cco.getAt(1)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(1)][R240[c.cco.getAt(1)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(5)][!c.ce.getOrientAt(5)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(5)][R120[c.cco.getAt(5)]]],
			colorPrint[cubeCornerColors[c.ccp.getAt(5)][c.cco.getAt(5)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(8)][c.ce.getOrientAt(8)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(4)][c.cco.getAt(4)]]);
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.getPermAt(9)][c.ce.getOrientAt(9)]],
			colorPrint[CORANGE],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(1)][c.ce.getOrientAt(1)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(1)][!c.ce.getOrientAt(1)]],
			colorPrint[CBLUE],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(2)][!c.ce.getOrientAt(2)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(2)][c.ce.getOrientAt(2)]],
			colorPrint[CRED],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(10)][c.ce.getOrientAt(10)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(10)][!c.ce.getOrientAt(10)]],
			colorPrint[CGREEN],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(9)][!c.ce.getOrientAt(9)]]);
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(6)][R120[c.cco.getAt(6)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(6)][!c.ce.getOrientAt(6)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(2)][R240[c.cco.getAt(2)]]],
			colorPrint[cubeCornerColors[c.ccp.getAt(2)][c.cco.getAt(2)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(3)][c.ce.getOrientAt(3)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(3)][c.cco.getAt(3)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(3)][R120[c.cco.getAt(3)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(7)][!c.ce.getOrientAt(7)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(7)][R240[c.cco.getAt(7)]]],
			colorPrint[cubeCornerColors[c.ccp.getAt(7)][c.cco.getAt(7)]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(11)][c.ce.getOrientAt(11)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(6)][c.cco.getAt(6)]]);
	printf("\n");
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(2)][R120[c.cco.getAt(2)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(3)][!c.ce.getOrientAt(3)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(3)][R240[c.cco.getAt(3)]]]);
	printf("        %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.getPermAt(6)][c.ce.getOrientAt(6)]],
			colorPrint[CWHITE],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(7)][c.ce.getOrientAt(7)]]);
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.ccp.getAt(6)][R240[c.cco.getAt(6)]]],
			colorPrint[cubeEdgeColors[c.ce.getPermAt(11)][!c.ce.getOrientAt(11)]],
			colorPrint[cubeCornerColors[c.ccp.getAt(7)][R120[c.cco.getAt(7)]]]);
	printf("\n");
}

