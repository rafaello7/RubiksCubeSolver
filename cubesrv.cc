#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdarg.h>

static void sendRespMessage(int fd, const char *fmt, ...)
{
    va_list args;
    char message[4096], respBuf[5000];

    va_start(args, fmt);
    vsprintf(message, fmt, args);
    va_end(args);
    sprintf(respBuf, "%x\r\n%s\r\n", strlen(message), message);
    write(fd, respBuf, strlen(respBuf));
}

static std::string fmt_time()
{
	return "";
}

enum rotate_dir {
	ORANGECW, ORANGE180, ORANGECCW, REDCW, RED180, REDCCW,
	YELLOWCW, YELLOW180, YELLOWCCW, WHITECW, WHITE180, WHITECCW,
	GREENCW, GREEN180, GREENCCW, BLUECW, BLUE180, BLUECCW,
	RCOUNT
};

enum cubecolor {
	CYELLOW, CORANGE, CBLUE, CRED, CGREEN, CWHITE, CCOUNT
};

static const int cubeCornerColors[8][3] = {
  { CBLUE,   CORANGE, CYELLOW }, { CBLUE,  CYELLOW, CRED },
  { CBLUE,   CWHITE,  CORANGE }, { CBLUE,  CRED,    CWHITE },
  { CGREEN,  CYELLOW, CORANGE }, { CGREEN, CRED,    CYELLOW },
  { CGREEN,  CORANGE, CWHITE  }, { CGREEN, CWHITE,  CRED }
};

static const int cubeEdgeColors[12][2] = {
  { CBLUE,   CYELLOW },
  { CORANGE, CBLUE   }, { CRED,    CBLUE },
  { CBLUE,   CWHITE  },
  { CYELLOW, CORANGE }, { CYELLOW, CRED },
  { CWHITE,  CORANGE }, { CWHITE,  CRED },
  { CGREEN,  CYELLOW },
  { CORANGE, CGREEN  }, { CRED,    CGREEN },
  { CGREEN,  CWHITE  }
};

static const char *rotateDirName(int rd) {
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

static int rotateDirReverse(int rd) {
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

static const unsigned char R120[3] = { 1, 2, 0 };
static const unsigned char R240[3] = { 2, 0, 1 };

class cubecorner_perms {
	unsigned perms;
public:
	cubecorner_perms() : perms(0) {}
	cubecorner_perms(unsigned corner0perm, unsigned corner1perm, unsigned corner2perm,
			unsigned corner3perm, unsigned corner4perm, unsigned corner5perm,
			unsigned corner6perm, unsigned corner7perm);

	void setAt(unsigned idx, unsigned char perm) {
		perms &= ~(7 << 3*idx);
		perms |= perm << 3*idx;
	}
	unsigned getAt(unsigned idx) const { return perms >> 3*idx & 7; }
	bool operator==(const cubecorner_perms &ccp) const { return perms == ccp.perms; }
	bool operator<(const cubecorner_perms &ccp) const { return perms < ccp.perms; }
};

cubecorner_perms::cubecorner_perms(unsigned corner0perm, unsigned corner1perm,
			unsigned corner2perm, unsigned corner3perm,
			unsigned corner4perm, unsigned corner5perm,
			unsigned corner6perm, unsigned corner7perm)
	: perms(corner0perm | corner1perm << 3 | corner2perm << 6 |
			corner3perm << 9 | corner4perm << 12 | corner5perm << 15 |
			corner6perm << 18 | corner7perm << 21)
{
}

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
	bool operator==(const cubecorner_orients &cco) const { return orients == cco.orients; }
	bool operator<(const cubecorner_orients &cco) const { return orients < cco.orients; }
};

cubecorner_orients::cubecorner_orients(unsigned corner0orient, unsigned corner1orient,
			unsigned corner2orient, unsigned corner3orient,
			unsigned corner4orient, unsigned corner5orient,
			unsigned corner6orient, unsigned corner7orient)
	: orients(corner0orient | corner1orient << 2 | corner2orient << 4 |
			corner3orient << 6 | corner4orient << 8 | corner5orient << 10 |
			corner6orient << 12 | corner7orient << 14)
{
}

class cubecorners {
	cubecorner_perms perms;
	cubecorner_orients orients;
public:
	cubecorners() {}
	cubecorners(const cubecorner_perms &perms, const cubecorner_orients &orients)
		: perms(perms), orients(orients) {}
	cubecorners(unsigned corner0perm, unsigned corner0orient,
			unsigned corner1perm, unsigned corner1orient,
			unsigned corner2perm, unsigned corner2orient,
			unsigned corner3perm, unsigned corner3orient,
			unsigned corner4perm, unsigned corner4orient,
			unsigned corner5perm, unsigned corner5orient,
			unsigned corner6perm, unsigned corner6orient,
			unsigned corner7perm, unsigned corner7orient);

	unsigned getPermAt(unsigned idx) const { return perms.getAt(idx); }
	void setPermAt(unsigned idx, unsigned char perm) { perms.setAt(idx, perm); }
	unsigned getOrientAt(unsigned idx) const { return orients.getAt(idx); }
	void setOrientAt(unsigned idx, unsigned char orient) { orients.setAt(idx, orient); }
	cubecorner_perms getPerms() const { return perms; }
	cubecorner_orients getOrients() const { return orients; }
	void setOrients(unsigned orts) { orients.set(orts); }
	bool operator==(const cubecorners &cc) const {
		return perms == cc.perms && orients == cc.orients;
	}
	bool operator<(const cubecorners &cc) const {
		return perms < cc.perms || perms == cc.perms && orients < cc.orients;
	}
};

cubecorners::cubecorners(unsigned corner0perm, unsigned corner0orient,
			unsigned corner1perm, unsigned corner1orient,
			unsigned corner2perm, unsigned corner2orient,
			unsigned corner3perm, unsigned corner3orient,
			unsigned corner4perm, unsigned corner4orient,
			unsigned corner5perm, unsigned corner5orient,
			unsigned corner6perm, unsigned corner6orient,
			unsigned corner7perm, unsigned corner7orient)

	: perms(corner0perm, corner1perm, corner2perm, corner3perm, corner4perm, corner5perm,
			corner6perm, corner7perm),
	orients(corner0orient, corner1orient, corner2orient,
			corner3orient, corner4orient, corner5orient, corner6orient, corner7orient)
{
}

class cubeedges {
	unsigned long edges;
public:
	cubeedges() : edges(0) {}
	cubeedges(unsigned edge0perm, unsigned edge0orient,
			unsigned edge1perm, unsigned edge1orient,
			unsigned edge2perm, unsigned edge2orient,
			unsigned edge3perm, unsigned edge3orient,
			unsigned edge4perm, unsigned edge4orient,
			unsigned edge5perm, unsigned edge5orient,
			unsigned edge6perm, unsigned edge6orient,
			unsigned edge7perm, unsigned edge7orient,
			unsigned edge8perm, unsigned edge8orient,
			unsigned edge9perm, unsigned edge9orient,
			unsigned edge10perm, unsigned edge10orient,
			unsigned edge11perm, unsigned edge11orient);

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
    static cubeedges transform(cubeedges, int idx);
	unsigned edgeN(unsigned idx) const { return edges >> 5 * idx & 0xf; }
	unsigned edgeR(unsigned idx) const { return edges >> (5*idx+4) & 1; }
	bool operator==(const cubeedges &ce) const { return edges == ce.edges; }
	bool operator<(const cubeedges &ce) const { return edges < ce.edges; }
};

cubeedges::cubeedges(unsigned edge0perm, unsigned edge0orient,
			unsigned edge1perm, unsigned edge1orient,
			unsigned edge2perm, unsigned edge2orient,
			unsigned edge3perm, unsigned edge3orient,
			unsigned edge4perm, unsigned edge4orient,
			unsigned edge5perm, unsigned edge5orient,
			unsigned edge6perm, unsigned edge6orient,
			unsigned edge7perm, unsigned edge7orient,
			unsigned edge8perm, unsigned edge8orient,
			unsigned edge9perm, unsigned edge9orient,
			unsigned edge10perm, unsigned edge10orient,
			unsigned edge11perm, unsigned edge11orient)
	: edges((unsigned long)edge0perm        | (unsigned long)edge0orient << 4 |
			(unsigned long)edge1perm << 5   | (unsigned long)edge1orient << 9 |
			(unsigned long)edge2perm << 10  | (unsigned long)edge2orient << 14 |
			(unsigned long)edge3perm << 15  | (unsigned long)edge3orient << 19 |
			(unsigned long)edge4perm << 20  | (unsigned long)edge4orient << 24 |
			(unsigned long)edge5perm << 25  | (unsigned long)edge5orient << 29 |
			(unsigned long)edge6perm << 30  | (unsigned long)edge6orient << 34 |
			(unsigned long)edge7perm << 35  | (unsigned long)edge7orient << 39 |
			(unsigned long)edge8perm << 40  | (unsigned long)edge8orient << 44 |
			(unsigned long)edge9perm << 45  | (unsigned long)edge9orient << 49 |
			(unsigned long)edge10perm << 50 | (unsigned long)edge10orient << 54 |
			(unsigned long)edge11perm << 55 | (unsigned long)edge11orient << 59)
{
}

cubeedges cubeedges::compose(cubeedges ce1, cubeedges ce2)
{
	cubeedges res;
	for(int i = 0; i < 12; ++i) {
        unsigned char edge2item = ce2.edges >> 5 * i;
        unsigned char edge2perm = edge2item & 0xf;
        unsigned char edge2orient = edge2item & 0x10;
        unsigned char edge1item = ce1.edges >> 5 * edge2perm & 0x1f;
        res.edges |= (unsigned long)(edge1item ^ edge2orient) << 5*i;
	}
	return res;
}

struct cube {
	struct cubecorners cc;
	struct cubeedges ce;

	bool operator==(const cube &c) const;
};

bool cube::operator==(const cube &c) const
{ 
	return cc == c.cc && ce == c.ce;
}

//           ___________
//           | YELLOW  |
//           | 4  8  5 |
//           | 4     5 |
//           | 2  3  3 |
//  _________|_________|_________ _________
// |  ORANGE |   BLUE  |   RED   |  GREEN  |
// | 4  4  0 | 0  0  1 | 1  5  5 | 5  8  4 |
// | 9     1 | 1     2 | 1    10 | 10    9 |
// | 6  6  2 | 2  3  3 | 3  7  7 | 7 11  6 |
// |_________|_________|_________|_________|
//           |  WHITE  |
//           | 2  3  3 |
//           | 6     7 |
//           | 6 11  7 |
//           |_________|
//
// orientation 0:
//           ___________
//           | YELLOW  |
//           |         |
//           | e     e |
//           |         |
//  _________|_________|_________ _________
// |  ORANGE |   BLUE  |   RED   |  GREEN  |
// |         | c  e  c |         | c     c |
// | e     e |         | e     e |         |
// |         | c  e  c |         | c     c |
// |_________|_________|_________|_________|
//           |  WHITE  |
//           |         |
//           | e     e |
//           |         |
//           |_________|

const struct cube csolved = {
	.cc = cubecorners(
		0, 0,  1, 0,
		2, 0,  3, 0,
		4, 0,  5, 0,
		6, 0,  7, 0
	),
	.ce = cubeedges(
		0, 0,
		1, 0,
		2, 0,
		3, 0,
		4, 0,
		5, 0,
		6, 0,
		7, 0,
		8, 0,
		9, 0,
		10, 0,
		11, 0
	)
};

const struct cube crotated[RCOUNT] = {
	{ // ORANGECW
		.cc = cubecorners(
			4, 1,
			1, 0,
			0, 2,
			3, 0,
			6, 2,
			5, 0,
			2, 1,
			7, 0
		),
		.ce = cubeedges(
			 0, 0,
			 4, 1,
			 2, 0,
			 3, 0,
			 9, 1,
			 5, 0,
			 1, 1,
			 7, 0,
			 8, 0,
			 6, 1,
			10, 0,
			11, 0
		)
	},{ // ORANGE180
		.cc = cubecorners(
			6, 0,
			1, 0,
			4, 0,
			3, 0,
			2, 0,
			5, 0,
			0, 0,
			7, 0
		),
		.ce = cubeedges(
			 0, 0,
			 9, 0,
			 2, 0,
			 3, 0,
			 6, 0,
			 5, 0,
			 4, 0,
			 7, 0,
			 8, 0,
			 1, 0,
			10, 0,
			11, 0
		)
	},{ // ORANGECCW
		.cc = cubecorners(
			2, 1,
			1, 0,
			6, 2,
			3, 0,
			0, 2,
			5, 0,
			4, 1,
			7, 0
		),
		.ce = cubeedges(
			 0, 0,
			 6, 1,
			 2, 0,
			 3, 0,
			 1, 1,
			 5, 0,
			 9, 1,
			 7, 0,
			 8, 0,
			 4, 1,
			10, 0,
			11, 0
		)
	},{ // REDCW
		.cc = cubecorners(
			0, 0,
			3, 2,
			2, 0,
			7, 1,
			4, 0,
			1, 1,
			6, 0,
			5, 2
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 7, 1,
			 3, 0,
			 4, 0,
			 2, 1,
			 6, 0,
			10, 1,
			 8, 0,
			 9, 0,
			 5, 1,
			11, 0
		)
	},{ // RED180
		.cc = cubecorners(
			0, 0,
			7, 0,
			2, 0,
			5, 0,
			4, 0,
			3, 0,
			6, 0,
			1, 0
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			10, 0,
			 3, 0,
			 4, 0,
			 7, 0,
			 6, 0,
			 5, 0,
			 8, 0,
			 9, 0,
			 2, 0,
			11, 0
		)
	},{ // REDCCW
		.cc = cubecorners(
			0, 0,
			5, 2,
			2, 0,
			1, 1,
			4, 0,
			7, 1,
			6, 0,
			3, 2
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 5, 1,
			 3, 0,
			 4, 0,
			10, 1,
			 6, 0,
			 2, 1,
			 8, 0,
			 9, 0,
			 7, 1,
			11, 0
		)
	},{ // YELLOWCW
		.cc = cubecorners(
			1, 2,
			5, 1,
			2, 0,
			3, 0,
			0, 1,
			4, 2,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 5, 1,
			 1, 0,
			 2, 0,
			 3, 0,
			 0, 1,
			 8, 1,
			 6, 0,
			 7, 0,
			 4, 1,
			 9, 0,
			10, 0,
			11, 0
		)
	},{ // YELLOW180
		.cc = cubecorners(
			5, 0,
			4, 0,
			2, 0,
			3, 0,
			1, 0,
			0, 0,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 8, 0,
			 1, 0,
			 2, 0,
			 3, 0,
			 5, 0,
			 4, 0,
			 6, 0,
			 7, 0,
			 0, 0,
			 9, 0,
			10, 0,
			11, 0
		)
	},{ // YELLOWCCW
		.cc = cubecorners(
			4, 2,
			0, 1,
			2, 0,
			3, 0,
			5, 1,
			1, 2,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 4, 1,
			 1, 0,
			 2, 0,
			 3, 0,
			 8, 1,
			 0, 1,
			 6, 0,
			 7, 0,
			 5, 1,
			 9, 0,
			10, 0,
			11, 0
		)
	},{ // WHITECW
		.cc = cubecorners(
			0, 0,
			1, 0,
			6, 1,
			2, 2,
			4, 0,
			5, 0,
			7, 2,
			3, 1
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			 6, 1,
			 4, 0,
			 5, 0,
			11, 1,
			 3, 1,
			 8, 0,
			 9, 0,
			10, 0,
			 7, 1
		)
	},{ // WHITE180
		.cc = cubecorners(
			0, 0,
			1, 0,
			7, 0,
			6, 0,
			4, 0,
			5, 0,
			3, 0,
			2, 0
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			11, 0,
			 4, 0,
			 5, 0,
			 7, 0,
			 6, 0,
			 8, 0,
			 9, 0,
			10, 0,
			 3, 0
		)
	},{ // WHITECCW
		.cc = cubecorners(
			0, 0,
			1, 0,
			3, 1,
			7, 2,
			4, 0,
			5, 0,
			2, 2,
			6, 1
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			 7, 1,
			 4, 0,
			 5, 0,
			 3, 1,
			11, 1,
			 8, 0,
			 9, 0,
			10, 0,
			 6, 1
		)
	},{ // GREENCW
		.cc = cubecorners(
			0, 0,
			1, 0,
			2, 0,
			3, 0,
			5, 0,
			7, 0,
			4, 0,
			6, 0
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			 3, 0,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			10, 1,
			 8, 1,
			11, 1,
			 9, 1
		)
	},{ // GREEN180
		.cc = cubecorners(
			0, 0,
			1, 0,
			2, 0,
			3, 0,
			7, 0,
			6, 0,
			5, 0,
			4, 0
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			 3, 0,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			11, 0,
			10, 0,
			 9, 0,
			 8, 0
		)
	},{ // GREENCCW
		.cc = cubecorners(
			0, 0,
			1, 0,
			2, 0,
			3, 0,
			6, 0,
			4, 0,
			7, 0,
			5, 0
		),
		.ce = cubeedges(
			 0, 0,
			 1, 0,
			 2, 0,
			 3, 0,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			 9, 1,
			11, 1,
			 8, 1,
			10, 1
		)
	},{ // BLUECW
		.cc = cubecorners(
			2, 0,
			0, 0,
			3, 0,
			1, 0,
			4, 0,
			5, 0,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 1, 1,
			 3, 1,
			 0, 1,
			 2, 1,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			 8, 0,
			 9, 0,
			10, 0,
			11, 0
		)
	},{ // BLUE180
		.cc = cubecorners(
			3, 0,
			2, 0,
			1, 0,
			0, 0,
			4, 0,
			5, 0,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 3, 0,
			 2, 0,
			 1, 0,
			 0, 0,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			 8, 0,
			 9, 0,
			10, 0,
			11, 0
		)
	},{ // BLUECCW
		.cc = cubecorners(
			1, 0,
			3, 0,
			0, 0,
			2, 0,
			4, 0,
			5, 0,
			6, 0,
			7, 0
		),
		.ce = cubeedges(
			 2, 1,
			 0, 1,
			 3, 1,
			 1, 1,
			 4, 0,
			 5, 0,
			 6, 0,
			 7, 0,
			 8, 0,
			 9, 0,
			10, 0,
			11, 0
		)
	}
};

//   Y
// O B R G      == Y O B R G W
//   W
const struct cube ctransformed[] = {
	{ // B W R Y O G
		.cc = cubecorners(
			3, 1,
			1, 2,
			7, 2,
			5, 1,
			2, 2,
			0, 1,
			6, 1,
			4, 2
		),
		.ce = cubeedges(
			 2, 0,
			 7, 0,
			 5, 0,
			10, 0,
			 3, 0,
			 0, 0,
			11, 0,
			 8, 0,
			 1, 0,
			 6, 0,
			 4, 0,
			 9, 0
		)
	},{
        .cc = cubecorners(
             5, 2,
             1, 1,
             4, 1,
             0, 2,
             7, 1,
             3, 2,
             6, 2,
             2, 1
        ),
        .ce = cubeedges(
             5, 0,
             8, 0,
             0, 0,
             4, 0,
            10, 0,
             2, 0,
             9, 0,
             1, 0,
             7, 0,
            11, 0,
             3, 0,
             6, 0
        )
	},{
		.cc = cubecorners(
			 0,  2,
			 4,  1,
			 1,  1,
			 5,  2,
			 2,  1,
			 6,  2,
			 3,  2,
			 7,  1
		),
		.ce = cubeedges(
			 4,  0,
			 0,  0,
			 8,  0,
			 5,  0,
			 1,  0,
			 9,  0,
			 2,  0,
			10,  0,
			 6,  0,
			 3,  0,
			11,  0,
			 7,  0
		)
	},{
		.cc = cubecorners(
			 0,  1,
			 2,  2,
			 4,  2,
			 6,  1,
			 1,  2,
			 3,  1,
			 5,  1,
			 7,  2
		),
		.ce = cubeedges(
			 1,  0,
			 4,  0,
			 6,  0,
			 9,  0,
			 0,  0,
			 3,  0,
			 8,  0,
			11,  0,
			 2,  0,
			 5,  0,
			 7,  0,
			10,  0
		)
	},{
		.cc = cubecorners(
			 5,  1,
			 7,  2,
			 1,  2,
			 3,  1,
			 4,  2,
			 6,  1,
			 0,  1,
			 2,  2
		),
		.ce = cubeedges(
			10,  0,
			 5,  0,
			 7,  0,
			 2,  0,
			 8,  0,
			11,  0,
			 0,  0,
			 3,  0,
			 9,  0,
			 4,  0,
			 6,  0,
			 1,  0
		)
	},{
		.cc = cubecorners(
			 6,  2,
			 2,  1,
			 7,  1,
			 3,  2,
			 4,  1,
			 0,  2,
			 5,  2,
			 1,  1
		),
		.ce = cubeedges(
			 6,  0,
			11,  0,
			 3,  0,
			 7,  0,
			 9,  0,
			 1,  0,
			10,  0,
			 2,  0,
			 4,  0,
			 8,  0,
			 0,  0,
			 5,  0
		)
	},{
		.cc = cubecorners(
			 6,  1,
			 4,  2,
			 2,  2,
			 0,  1,
			 7,  2,
			 5,  1,
			 3,  1,
			 1,  2
		),
		.ce = cubeedges(
			 9,  0,
			 6,  0,
			 4,  0,
			 1,  0,
			11,  0,
			 8,  0,
			 3,  0,
			 0,  0,
			10,  0,
			 7,  0,
			 5,  0,
			 2,  0
		)
	},{
		.cc = cubecorners(
			 3,  2,
			 7,  1,
			 2,  1,
			 6,  2,
			 1,  1,
			 5,  2,
			 0,  2,
			 4,  1
		),
		.ce = cubeedges(
			 7,  0,
			 3,  0,
			11,  0,
			 6,  0,
			 2,  0,
			10,  0,
			 1,  0,
			 9,  0,
			 5,  0,
			 0,  0,
			 8,  0,
			 4,  0
		)
	},{
		.cc = cubecorners(
			 2,  1,
			 3,  2,
			 6,  2,
			 7,  1,
			 0,  2,
			 1,  1,
			 4,  1,
			 5,  2
		),
		.ce = cubeedges(
			 3,  1,
			 6,  1,
			 7,  1,
			11,  1,
			 1,  1,
			 2,  1,
			 9,  1,
			10,  1,
			 0,  1,
			 4,  1,
			 5,  1,
			 8,  1
		)
	},{
		.cc = cubecorners(
			 6,  0,
			 7,  0,
			 4,  0,
			 5,  0,
			 2,  0,
			 3,  0,
			 0,  0,
			 1,  0
		),
		.ce = cubeedges(
			11,  0,
			 9,  0,
			10,  0,
			 8,  0,
			 6,  0,
			 7,  0,
			 4,  0,
			 5,  0,
			 3,  0,
			 1,  0,
			 2,  0,
			 0,  0
		)
	},{
		.cc = cubecorners(
			 4,  1,
			 5,  2,
			 0,  2,
			 1,  1,
			 6,  2,
			 7,  1,
			 2,  1,
			 3,  2
		),
		.ce = cubeedges(
			 8,  1,
			 4,  1,
			 5,  1,
			 0,  1,
			 9,  1,
			10,  1,
			 1,  1,
			 2,  1,
			11,  1,
			 6,  1,
			 7,  1,
			 3,  1
		)
	},{
		.cc = cubecorners(
			 2,  0,
			 0,  0,
			 3,  0,
			 1,  0,
			 6,  0,
			 4,  0,
			 7,  0,
			 5,  0
		),
		.ce = cubeedges(
			 1,  1,
			 3,  1,
			 0,  1,
			 2,  1,
			 6,  1,
			 4,  1,
			 7,  1,
			 5,  1,
			 9,  1,
			11,  1,
			 8,  1,
			10,  1
		)
	},{
		.cc = cubecorners(
			 3,  0,
			 2,  0,
			 1,  0,
			 0,  0,
			 7,  0,
			 6,  0,
			 5,  0,
			 4,  0
		),
		.ce = cubeedges(
			 3,  0,
			 2,  0,
			 1,  0,
			 0,  0,
			 7,  0,
			 6,  0,
			 5,  0,
			 4,  0,
			11,  0,
			10,  0,
			 9,  0,
			 8,  0
		)
	},{
		.cc = cubecorners(
			 1,  0,
			 3,  0,
			 0,  0,
			 2,  0,
			 5,  0,
			 7,  0,
			 4,  0,
			 6,  0
		),
		.ce = cubeedges(
			 2,  1,
			 0,  1,
			 3,  1,
			 1,  1,
			 5,  1,
			 7,  1,
			 4,  1,
			 6,  1,
			10,  1,
			 8,  1,
			11,  1,
			 9,  1
		)
	},{
		.cc = cubecorners(
			 4,  2,
			 0,  1,
			 6,  1,
			 2,  2,
			 5,  1,
			 1,  2,
			 7,  2,
			 3,  1
		),
		.ce = cubeedges(
			 4,  1,
			 9,  1,
			 1,  1,
			 6,  1,
			 8,  1,
			 0,  1,
			11,  1,
			 3,  1,
			 5,  1,
			10,  1,
			 2,  1,
			 7,  1
		)
	},{
		.cc = cubecorners(
			 5,  0,
			 4,  0,
			 7,  0,
			 6,  0,
			 1,  0,
			 0,  0,
			 3,  0,
			 2,  0
		),
		.ce = cubeedges(
			 8,  0,
			10,  0,
			 9,  0,
			11,  0,
			 5,  0,
			 4,  0,
			 7,  0,
			 6,  0,
			 0,  0,
			 2,  0,
			 1,  0,
			 3,  0
		)
	},{
		.cc = cubecorners(
			 1,  2,
			 5,  1,
			 3,  1,
			 7,  2,
			 0,  1,
			 4,  2,
			 2,  2,
			 6,  1
		),
		.ce = cubeedges(
			 5,  1,
			 2,  1,
			10,  1,
			 7,  1,
			 0,  1,
			 8,  1,
			 3,  1,
			11,  1,
			 4,  1,
			 1,  1,
			 9,  1,
			 6,  1
		)
	},{
		.cc = cubecorners(
			 4,  0,
			 6,  0,
			 5,  0,
			 7,  0,
			 0,  0,
			 2,  0,
			 1,  0,
			 3,  0
		),
		.ce = cubeedges(
			 9,  1,
			 8,  1,
			11,  1,
			10,  1,
			 4,  1,
			 6,  1,
			 5,  1,
			 7,  1,
			 1,  1,
			 0,  1,
			 3,  1,
			 2,  1
		)
	},{
		.cc = cubecorners(
			 2,  2,
			 6,  1,
			 0,  1,
			 4,  2,
			 3,  1,
			 7,  2,
			 1,  2,
			 5,  1
		),
		.ce = cubeedges(
			 6,  1,
			 1,  1,
			 9,  1,
			 4,  1,
			 3,  1,
			11,  1,
			 0,  1,
			 8,  1,
			 7,  1,
			 2,  1,
			10,  1,
			 5,  1
		)
	},{
		.cc = cubecorners(
			 1,  1,
			 0,  2,
			 5,  2,
			 4,  1,
			 3,  2,
			 2,  1,
			 7,  1,
			 6,  2
		),
		.ce = cubeedges(
			 0,  1,
			 5,  1,
			 4,  1,
			 8,  1,
			 2,  1,
			 1,  1,
			10,  1,
			 9,  1,
			 3,  1,
			 7,  1,
			 6,  1,
			11,  1
		)
	},{
		.cc = cubecorners(
			 7,  0,
			 5,  0,
			 6,  0,
			 4,  0,
			 3,  0,
			 1,  0,
			 2,  0,
			 0,  0
		),
		.ce = cubeedges(
			10,  1,
			11,  1,
			 8,  1,
			 9,  1,
			 7,  1,
			 5,  1,
			 6,  1,
			 4,  1,
			 2,  1,
			 3,  1,
			 0,  1,
			 1,  1
		)
	},{
		.cc = cubecorners(
			 7,  2,
			 3,  1,
			 5,  1,
			 1,  2,
			 6,  1,
			 2,  2,
			 4,  2,
			 0,  1
		),
		.ce = cubeedges(
			 7,  1,
			10,  1,
			 2,  1,
			 5,  1,
			11,  1,
			 3,  1,
			 8,  1,
			 0,  1,
			 6,  1,
			 9,  1,
			 1,  1,
			 4,  1
		)
	},{
		.cc = cubecorners(
			 7,  1,
			 6,  2,
			 3,  2,
			 2,  1,
			 5,  2,
			 4,  1,
			 1,  1,
			 0,  2
		),
		.ce = cubeedges(
			11,  1,
			 7,  1,
			 6,  1,
			 3,  1,
			10,  1,
			 9,  1,
			 2,  1,
			 1,  1,
			 8,  1,
			 5,  1,
			 4,  1,
			 0,  1
		)
	}
};

const int TCOUNT = sizeof(ctransformed) / sizeof(ctransformed[0]);

static int transformReverse(int idx) {
    switch( idx ) {
        case 0: return 1;
        case 1: return 0;
        case 2: return 3;
        case 3: return 2;
        case 4: return 5;
        case 5: return 4;
        case 6: return 7;
        case 7: return 6;
        case 8: return 10;
        case 10: return 8;
        case 11: return 13;
        case 13: return 11;
        case 14: return 16;
        case 16: return 14;
    }
    return idx;
}

static cubecorner_perms cubecornerPermsCompose(const cubecorner_perms &ccp1,
		const cubecorner_perms &ccp2)
{
	cubecorner_perms res;
	for(int i = 0; i < 8; ++i)
		res.setAt(i, ccp1.getAt(ccp2.getAt(i)));
	return res;
}

static cubecorner_orients cubecornerOrientsCompose(cubecorner_orients cco1,
		const cubecorners &cc2)
{
	static const unsigned char MOD3[] = { 0, 1, 2, 0, 1 };
	cubecorner_orients res;
	for(int i = 0; i < 8; ++i)
		res.setAt(i, MOD3[cco1.getAt(cc2.getPermAt(i)) + cc2.getOrientAt(i)]);
	return res;
}

static cubecorner_orients cubecornerOrientsCompose(cubecorner_orients cco1,
		const cubecorner_perms &ccp2, cubecorner_orients cco2)
{
	static const unsigned char MOD3[] = { 0, 1, 2, 0, 1 };
	cubecorner_orients res;
	for(int i = 0; i < 8; ++i)
		res.setAt(i, MOD3[cco1.getAt(ccp2.getAt(i)) + cco2.getAt(i)]);
	return res;
}

static cubecorners cubecornersCompose(const cubecorners &cc1, const cubecorners &cc2)
{
	static const unsigned char MOD3[] = { 0, 1, 2, 0, 1 };
	cubecorners res;
	for(int i = 0; i < 8; ++i) {
		unsigned corner2Perm = cc2.getPermAt(i);
		res.setPermAt(i, cc1.getPermAt(corner2Perm));
		res.setOrientAt(i, MOD3[cc1.getOrientAt(corner2Perm) + cc2.getOrientAt(i)]);
	}
	return res;
}

static cube cubesCompose(const cube &c1, const cube &c2)
{
	cube res;
	res.cc = cubecornersCompose(c1.cc, c2.cc);
	res.ce = cubeedges::compose(c1.ce, c2.ce);
	return res;
}

static cube cubeTransform(const cube &c, int idx)
{
    const cube &ctrans = ctransformed[idx];
    cubecorners cc1 = cubecornersCompose(ctrans.cc, c.cc);
    cubeedges ce1 = cubeedges::compose(ctrans.ce, c.ce);
    const cube &ctransRev = ctransformed[transformReverse(idx)];
    cubecorners cc2 = cubecornersCompose(cc1, ctransRev.cc);
    cubeedges ce2 = cubeedges::compose(ce1, ctransRev.ce);
	return { .cc = cc2, .ce = ce2 };
}

static cubecorner_perms cubecornerPermsTransform1(cubecorner_perms ccp, int idx)
{
    cubecorner_perms ccp1 = cubecornerPermsCompose(ctransformed[idx].cc.getPerms(), ccp);
	return cubecornerPermsCompose(ccp1,
            ctransformed[transformReverse(idx)].cc.getPerms());
}

static cubecorner_perms cubecornerPermsTransform(cubecorner_perms ccp, int idx)
{
    cubecorner_perms cctr = ctransformed[idx].cc.getPerms();
    cubecorner_perms ccrtr = ctransformed[transformReverse(idx)].cc.getPerms();
	cubecorner_perms res;
	for(int cno = 0; cno < 8; ++cno) {
		res.setAt(cno, cctr.getAt(ccp.getAt(ccrtr.getAt(cno))));
    }
    return res;
}

static cubecorner_orients cubecornerOrientsTransform(cubecorner_perms ccp, cubecorner_orients cco, int idx)
{
    cubecorner_orients cco1 = cubecornerOrientsCompose(ctransformed[idx].cc.getOrients(), ccp, cco);
	return cubecornerOrientsCompose(cco1, ctransformed[transformReverse(idx)].cc);
}

cubeedges cubeedges::transform(cubeedges ce, int idx)
{
    cubeedges cetrans = ctransformed[idx].ce;
    cubeedges cetransRev = ctransformed[transformReverse(idx)].ce;
#if 0
    cubeedges ce1 = cubeedges::compose(cetrans, ce);
    cubeedges ce2 = cubeedges::compose(ce1, cetransRev);
	return ce2;
#else
	cubeedges res;
	for(int i = 0; i < 12; ++i) {
		unsigned ctrRevPerm = cetransRev.edges >> 5 * i & 0xf;
		unsigned ceItem = ce.edges >> 5 * ctrRevPerm;
		unsigned cePerm = ceItem & 0xf;
		unsigned ceOrient = ceItem & 0x10;
		unsigned ctransPerm = cetrans.edges >> 5 * cePerm & 0xf;
		res.edges |= (unsigned long)(ctransPerm | ceOrient) << 5*i;
	}
	return res;
#endif
}

static cubecorner_perms cornersIdxToPerm(unsigned short idx);

struct cubecorner_repr {
    unsigned reprIdx;
    unsigned transform;
};

static std::vector<cubecorner_perms> gPermRepr;
static std::map<cubecorner_perms, cubecorner_repr> gPermReprIdxs;

static void permReprInit()
{
    for(int i = 0; i < 40320; ++i) {
        cubecorner_perms perm = cornersIdxToPerm(i);
        cubecorner_perms permRepr = perm;
        unsigned transform = 1;
        for(int i = 0; i < TCOUNT; ++i) {
            cubecorner_perms cctr = ctransformed[i].cc.getPerms();
            cubecorner_perms ccrtr = ctransformed[transformReverse(i)].cc.getPerms();
            cubecorner_perms cand;
            for(int cno = 0; cno < 8; ++cno) {
                cand.setAt(cno, cctr.getAt(perm.getAt(ccrtr.getAt(cno))));
            }
            if( cand < permRepr ) {
                permRepr = cand;
                transform = 2 << i;
            }else if( cand == permRepr ) {
                transform |= 2 << i;
            }
        }
        std::map<cubecorner_perms, cubecorner_repr>::const_iterator it = gPermReprIdxs.find(permRepr);
        unsigned permReprIdx;
        if( it == gPermReprIdxs.end() ) {
            permReprIdx = gPermRepr.size();
            gPermRepr.push_back(permRepr);
            cubecorner_repr repr = { .reprIdx = permReprIdx, .transform = 0 };
            gPermReprIdxs[permRepr] = repr;
        }else
            permReprIdx = it->second.reprIdx;
        cubecorner_repr repr = { .reprIdx = permReprIdx, .transform = transform };
        gPermReprIdxs[perm] = repr;
    }
}

static cubecorner_perms cubecornerPermsRepresentative(cubecorner_perms ccp)
{
    unsigned permReprIdx = gPermReprIdxs.at(ccp).reprIdx;
    return gPermRepr[permReprIdx];
}

static cubecorner_orients cubecornerOrientsRepresentative(cubecorner_perms ccp, cubecorner_orients cco,
        std::vector<int> &transform)
{
    cubecorner_repr repr = gPermReprIdxs.at(ccp);
    cubecorner_orients orepr = cco;
    bool isInit = repr.transform & 1;
	transform.clear();
	transform.push_back(-1);
    for(int i = 0; i < TCOUNT; ++i) {
        if( repr.transform & 2 << i ) {
            cubecorner_orients ocand = cubecornerOrientsTransform(ccp, cco, i);
            if( !isInit || ocand < orepr ) {
                orepr = ocand;
				transform.clear();
				transform.push_back(i);
                isInit = true;
            }else if( isInit && ocand == orepr )
				transform.push_back(i);
        }
    }
    return orepr;
}

static unsigned cubecornerPermRepresentativeIdx(cubecorner_perms ccp)
{
    return gPermReprIdxs.at(ccp).reprIdx;
}

static cubeedges cubeedgesRepresentative(cubeedges ce, const std::vector<int> &transform)
{
    cubeedges cerepr = transform[0] == -1 ? ce : cubeedges::transform(ce, transform[0]);
    for(int i = 1; i < transform.size(); ++i) {
		cubeedges cand = cubeedges::transform(ce, transform[i]);
		if( cand < cerepr )
			cerepr = cand;
    }
    return cerepr;
}

static void cubePrint(const cube &c)
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
			colorPrint[cubeCornerColors[c.cc.getPermAt(4)][R120[c.cc.getOrientAt(4)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(8)][!c.ce.edgeR(8)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(5)][R240[c.cc.getOrientAt(5)]]]);
	printf("        %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.edgeN(4)][c.ce.edgeR(4)]],
			colorPrint[CYELLOW],
			colorPrint[cubeEdgeColors[c.ce.edgeN(5)][c.ce.edgeR(5)]]);
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.cc.getPermAt(0)][R240[c.cc.getOrientAt(0)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(0)][!c.ce.edgeR(0)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(1)][R120[c.cc.getOrientAt(1)]]]);
	printf("\n");
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeCornerColors[c.cc.getPermAt(4)][R240[c.cc.getOrientAt(4)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(4)][!c.ce.edgeR(4)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(0)][R120[c.cc.getOrientAt(0)]]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(0)][c.cc.getOrientAt(0)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(0)][c.ce.edgeR(0)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(1)][c.cc.getOrientAt(1)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(1)][R240[c.cc.getOrientAt(1)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(5)][!c.ce.edgeR(5)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(5)][R120[c.cc.getOrientAt(5)]]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(5)][c.cc.getOrientAt(5)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(8)][c.ce.edgeR(8)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(4)][c.cc.getOrientAt(4)]]);
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.edgeN(9)][c.ce.edgeR(9)]],
			colorPrint[CORANGE],
			colorPrint[cubeEdgeColors[c.ce.edgeN(1)][c.ce.edgeR(1)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(1)][!c.ce.edgeR(1)]],
			colorPrint[CBLUE],
			colorPrint[cubeEdgeColors[c.ce.edgeN(2)][!c.ce.edgeR(2)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(2)][c.ce.edgeR(2)]],
			colorPrint[CRED],
			colorPrint[cubeEdgeColors[c.ce.edgeN(10)][c.ce.edgeR(10)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(10)][!c.ce.edgeR(10)]],
			colorPrint[CGREEN],
			colorPrint[cubeEdgeColors[c.ce.edgeN(9)][!c.ce.edgeR(9)]]);
	printf(" %s%s%s %s%s%s %s%s%s %s%s%s\n",
			colorPrint[cubeCornerColors[c.cc.getPermAt(6)][R120[c.cc.getOrientAt(6)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(6)][!c.ce.edgeR(6)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(2)][R240[c.cc.getOrientAt(2)]]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(2)][c.cc.getOrientAt(2)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(3)][c.ce.edgeR(3)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(3)][c.cc.getOrientAt(3)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(3)][R120[c.cc.getOrientAt(3)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(7)][!c.ce.edgeR(7)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(7)][R240[c.cc.getOrientAt(7)]]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(7)][c.cc.getOrientAt(7)]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(11)][c.ce.edgeR(11)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(6)][c.cc.getOrientAt(6)]]);
	printf("\n");
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.cc.getPermAt(2)][R120[c.cc.getOrientAt(2)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(3)][!c.ce.edgeR(3)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(3)][R240[c.cc.getOrientAt(3)]]]);
	printf("        %s%s%s\n",
			colorPrint[cubeEdgeColors[c.ce.edgeN(6)][c.ce.edgeR(6)]],
			colorPrint[CWHITE],
			colorPrint[cubeEdgeColors[c.ce.edgeN(7)][c.ce.edgeR(7)]]);
	printf("        %s%s%s\n",
			colorPrint[cubeCornerColors[c.cc.getPermAt(6)][R240[c.cc.getOrientAt(6)]]],
			colorPrint[cubeEdgeColors[c.ce.edgeN(11)][!c.ce.edgeR(11)]],
			colorPrint[cubeCornerColors[c.cc.getPermAt(7)][R120[c.cc.getOrientAt(7)]]]);
	printf("\n");
}

static bool isCubeSolvable(int reqFd, const cube &c)
{
    bool isSwapsOdd = false;
    unsigned permsScanned = 0;
    for(int i = 0; i < 8; ++i) {
        if( permsScanned & 1 << i )
            continue;
        permsScanned |= 1 << i;
        int p = i;
        while( (p = c.cc.getPermAt(p)) != i ) {
            if( permsScanned & 1 << p ) {
                sendRespMessage(reqFd, "corner perm %d is twice\n", p);
                return false;
            }
            permsScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
    permsScanned = 0;
    for(int i = 0; i < 12; ++i) {
        if( permsScanned & 1 << i )
            continue;
        permsScanned |= 1 << i;
        int p = i;
        while( (p = c.ce.edgeN(p)) != i ) {
            if( permsScanned & 1 << p ) {
                sendRespMessage(reqFd, "edge perm %d is twice\n", p);
                return false;
            }
            permsScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
	if( isSwapsOdd ) {
		sendRespMessage(reqFd, "cube unsolvable due to permutation parity\n");
		return false;
	}
	unsigned sumOrient = 0;
	for(int i = 0; i < 8; ++i)
		sumOrient += c.cc.getOrientAt(i);
	if( sumOrient % 3 ) {
		sendRespMessage(reqFd, "cube unsolvable due to corner orientations\n");
		return false;
	}
	sumOrient = 0;
	for(int i = 0; i < 12; ++i)
		sumOrient += c.ce.edgeR(i);
	if( sumOrient % 2 ) {
		sendRespMessage(reqFd, "cube unsolvable due to edge orientations\n");
		return false;
	}
    return true;
}

struct elemLoc {
	int wall;
    int row;
	int col;
};

static bool cubeRead(int reqFd, const char *squareColors, cube &c)
{
	char line[100];
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
            sendRespMessage(reqFd, "bad letter at column %d\n", cno);
            return false;
        }
        walls[cno / 9][cno % 9 / 3][cno % 3] = lpos - colorLetters;
    }
	for(int i = 0; i < 6; ++i) {
		if( walls[i][1][1] != i ) {
			sendRespMessage(reqFd, "bad orientation: wall=%d exp=%c is=%c\n",
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
				c.cc.setPermAt(i, n);
				c.cc.setOrientAt(i, 0);
				break;
			}
			match = true;
			for(int r = 0; r < 3 && match; ++r) {
				const elemLoc &el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[R120[r]] )
					match = false;
			}
			if( match ) {
				c.cc.setPermAt(i, n);
				c.cc.setOrientAt(i, 1);
				break;
			}
			match = true;
			for(int r = 0; r < 3 && match; ++r) {
				const elemLoc &el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[R240[r]] )
					match = false;
			}
			if( match ) {
				c.cc.setPermAt(i, n);
				c.cc.setOrientAt(i, 2);
				break;
			}
		}
		if( ! match ) {
			sendRespMessage(reqFd, "corner %d not found\n", i);
			return false;
		}
		for(int j = 0; j < i; ++j) {
			if( c.cc.getPermAt(i) == c.cc.getPermAt(j) ) {
				sendRespMessage(reqFd, "corner %d is twice: at %d and %d\n",
						c.cc.getPermAt(i), j, i);
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
			sendRespMessage(reqFd, "edge %d not found\n", i);
			return false;
		}
		for(int j = 0; j < i; ++j) {
			if( c.ce.edgeN(i) == c.ce.edgeN(j) ) {
				sendRespMessage(reqFd, "edge %d is twice: at %d and %d\n",
						c.ce.edgeN(i), j, i);
				return false;
			}
		}
	}
    if( ! isCubeSolvable(reqFd, c) )
        return false;
	return true;
}

static cubecorner_perms cornersIdxToPerm(unsigned short idx)
{
	unsigned unused = 0x76543210;
	cubecorner_perms ccp;

	for(unsigned cornerIdx = 8; cornerIdx > 0; --cornerIdx) {
		unsigned p = idx % cornerIdx * 4;
		ccp.setAt(8-cornerIdx, unused >> p & 0xf);
		unsigned m = (1 << p) - 1;
		unused = unused & m | unused >> 4 & ~m;
		idx /= cornerIdx;
	}
	return ccp;
}

static unsigned short cornersPermToIdx(cubecorner_perms ccp)
{
	unsigned short res = 0;
	unsigned indexes = 0;
	for(int i = 7; i >= 0; --i) {
		unsigned p = ccp.getAt(i) * 4;
		res = (8-i) * res + (indexes >> p & 0xf);
		indexes += 0x11111111 << p;
	}
	return res;
}

static cubecorner_orients cornersIdxToOrient(unsigned short idx)
{
    cubecorner_orients res;
    int sum = 0;
    for(unsigned i = 0; i < 7; ++i) {
        unsigned short val = idx % 3;
        idx /= 3;
        res.setAt(6-i, val);
        sum += val;
    }
    res.setAt(7, (15-sum) % 3);
    return res;
}

static unsigned short cornersOrientToIdx(cubecorner_orients cco)
{
	unsigned short res = 0;
	for(unsigned i = 0; i < 7; ++i)
		res = res * 3 + cco.getAt(i);
	return res;
}

enum { CSARR_SIZE = 9, CSREPR_SIZE = 3 };

const struct {
    unsigned csarr, csrepr;
} depthMaxByMem[] = {
    { .csarr = 6, .csrepr = 1 },    // ~1GB
    { .csarr = 7, .csrepr = 0 },    // ~2.3GB
    { .csarr = 6, .csrepr = 2 },    // ~8.7GB
    { .csarr = 7, .csrepr = 1 },    // ~10GB
    { .csarr = 8, .csrepr = 0 },    // ~24.7GB
};

static int depthMaxSelFn() {
    long pageSize = sysconf(_SC_PAGESIZE);
    long pageCount = sysconf(_SC_PHYS_PAGES);
    long memSizeGB = pageSize * pageCount / 1073741824;
    int model = memSizeGB >= 26 ? 4 : memSizeGB >= 11 ? 3 : memSizeGB >= 10 ? 2 : memSizeGB >= 3 ? 1 : 0;
    printf("page size: %ld, count: %ld, mem GB: %ld, model: %d\n",
            pageSize, pageCount, memSizeGB, model);
    return model;
}

const auto [DEPTH_MAX, DREPR_MAX] = depthMaxByMem[depthMaxSelFn()];

class CornerOrientCubes {
    cubecorner_orients m_orients;
    std::vector<cubeedges> m_items;

public:
    CornerOrientCubes(cubecorner_orients);
    void swap(CornerOrientCubes &cs) {
        std::swap(m_orients, cs.m_orients);
        m_items.swap(cs.m_items);
    }
    cubecorner_orients cornerOrients() const { return m_orients; }
    bool addCube(cubeedges ce);
    unsigned edgeCount() const { return m_items.size(); }
    cubeedges cubeedgesAt(int edgeNo) const { return m_items[edgeNo]; }
    bool containsCubeEdges(cubeedges) const;
};

CornerOrientCubes::CornerOrientCubes(cubecorner_orients orients)
	: m_orients(orients)
{
}

bool CornerOrientCubes::addCube(cubeedges ce)
{
	unsigned lo = 0, hi = m_items.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_items[mi] < ce )
			lo = mi + 1;
		else
			hi = mi;
	}
	if( lo < m_items.size() && m_items[lo] == ce )
		return false;
	if( lo == m_items.size() ) {
		m_items.push_back(ce);
	}else{
		hi = m_items.size();
		m_items.push_back(m_items[hi-1]);
		while( --hi > lo )
			m_items[hi] = m_items[hi-1];
		m_items[hi] = ce;
	}
	return true;
}

bool CornerOrientCubes::containsCubeEdges(cubeedges ce) const
{
	unsigned lo = 0, hi = m_items.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_items[mi] < ce )
			lo = mi + 1;
		else
			hi = mi;
	}
	return lo < m_items.size() && m_items[lo] == ce;
}

class CornerPermCubes {
public:
	CornerPermCubes();
	~CornerPermCubes();
	const CornerOrientCubes *cornerOrientCubes(cubecorner_orients) const;
	CornerOrientCubes &cornerOrientCubesAddIfAbsent(cubecorner_orients);
	int cornerOrientCubeCount() const { return m_coCubes.size(); }
	const CornerOrientCubes &cornerOrientCubesAt(int cornerOrientNo) const {
        return m_coCubes[cornerOrientNo];
    }
	CornerOrientCubes &cornerOrientCubesAt(int cornerOrientNo) {
        return m_coCubes[cornerOrientNo];
    }

private:
	std::vector<CornerOrientCubes> m_coCubes;
};

CornerPermCubes::CornerPermCubes()
{
}

CornerPermCubes::~CornerPermCubes()
{
}

const CornerOrientCubes *CornerPermCubes::cornerOrientCubes(cubecorner_orients cco) const
{
	unsigned lo = 0, hi = m_coCubes.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_coCubes[mi].cornerOrients() < cco )
			lo = mi + 1;
		else
			hi = mi;
	}
	return lo < m_coCubes.size() && cco == m_coCubes[lo].cornerOrients() ?
		&m_coCubes[lo] : NULL;
}

CornerOrientCubes &CornerPermCubes::cornerOrientCubesAddIfAbsent(cubecorner_orients cco)
{
	unsigned lo = 0, hi = m_coCubes.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_coCubes[mi].cornerOrients() < cco )
			lo = mi + 1;
		else
			hi = mi;
	}
	if( lo == m_coCubes.size() || cco < m_coCubes[lo].cornerOrients() ) {
		m_coCubes.push_back(CornerOrientCubes(cco));
		hi = m_coCubes.size();
		while( --hi > lo )
			m_coCubes[hi].swap(m_coCubes[hi-1]);
	}
	return m_coCubes[lo];
}

class CornerPermReprCubes {
	std::vector<cubeedges> m_items[2187];

public:
	CornerPermReprCubes();
	~CornerPermReprCubes();
	bool addCube(unsigned cornerOrientIdx, cubeedges);
	bool containsCubeEdges(unsigned cornerOrientIdx, cubeedges) const;
    int edgeCount(unsigned cornerOrientIdx) const { return m_items[cornerOrientIdx].size(); }
	cubeedges cubeedgesAt(unsigned cornerOrientIdx, int edgeNo) const;
};

CornerPermReprCubes::CornerPermReprCubes()
{
}

CornerPermReprCubes::~CornerPermReprCubes()
{
}

bool CornerPermReprCubes::addCube(unsigned cornerOrientIdx, cubeedges ce)
{
    std::vector<cubeedges> &items = m_items[cornerOrientIdx];
	unsigned lo = 0, hi = items.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( items[mi] < ce )
			lo = mi + 1;
		else
			hi = mi;
	}
	if( lo < items.size() && items[lo] == ce )
		return false;
	if( lo == items.size() ) {
		items.push_back(ce);
	}else{
		hi = items.size();
		items.push_back(items[hi-1]);
		while( --hi > lo )
			items[hi] = items[hi-1];
		items[hi] = ce;
	}
	return true;
}

bool CornerPermReprCubes::containsCubeEdges(unsigned cornerOrientIdx, cubeedges ce) const
{
    const std::vector<cubeedges> &items = m_items[cornerOrientIdx];
	unsigned lo = 0, hi = items.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( items[mi] < ce )
			lo = mi + 1;
		else
			hi = mi;
	}
	return lo < items.size() && items[lo] == ce;
}

cubeedges CornerPermReprCubes::cubeedgesAt(unsigned cornerOrientIdx, int edgeNo) const
{
	return m_items[cornerOrientIdx][edgeNo];
}

class CubesAtDepth {
    std::vector<CornerPermCubes> m_cornerPermCubes;
public:
    CubesAtDepth();
    CubesAtDepth(const CubesAtDepth&) = delete;
    ~CubesAtDepth();
    bool empty() const { return m_cornerPermCubes.empty(); }
    void init() { m_cornerPermCubes.resize(40320); }
    CornerPermCubes &operator[](unsigned idx) { return m_cornerPermCubes[idx]; }
    const CornerPermCubes &operator[](unsigned idx) const { return m_cornerPermCubes[idx]; }
};

CubesAtDepth::CubesAtDepth() {
}

CubesAtDepth::~CubesAtDepth() {
}

class CubesReprAtDepth {
    std::vector<CornerPermReprCubes> m_cornerPermReprCubes;
public:
    CubesReprAtDepth();
    CubesReprAtDepth(const CubesReprAtDepth&) = delete;
    ~CubesReprAtDepth();
    bool empty() const { return m_cornerPermReprCubes.empty(); }
    void init() { m_cornerPermReprCubes.resize(1844); }
    CornerPermReprCubes &operator[](unsigned idx) { return m_cornerPermReprCubes[idx]; }
    const CornerPermReprCubes &operator[](unsigned idx) const { return m_cornerPermReprCubes[idx]; }
};

CubesReprAtDepth::CubesReprAtDepth() {
}

CubesReprAtDepth::~CubesReprAtDepth() {
}

static std::string printMoves(const CubesAtDepth *cubesByDepth, const cube &c)
{
	std::vector<int> rotateDirs;
	unsigned short cornersPerm = cornersPermToIdx(c.cc.getPerms());
	int depth = 0;
	while( true ) {
		const CornerOrientCubes *corientCubes =
            cubesByDepth[depth][cornersPerm].cornerOrientCubes(c.cc.getOrients());
		if( corientCubes != NULL && corientCubes->containsCubeEdges(c.ce) )
			break;
		++depth;
	}
	cube cc = c;
	while( depth-- > 0 ) {
		int cm = 0;
		cube cc1;
		while( true ) {
			cc1 = cubesCompose(cc, crotated[cm]);
			cornersPerm = cornersPermToIdx(cc1.cc.getPerms());
            const CornerOrientCubes *corientCubes =
                cubesByDepth[depth][cornersPerm].cornerOrientCubes(cc1.cc.getOrients());
			if( corientCubes != NULL && corientCubes->containsCubeEdges(cc1.ce) )
				break;
			++cm;
		}
		rotateDirs.push_back(rotateDirReverse(cm));
		cc = cc1;
	}
    std::string res;
	for(int j = rotateDirs.size(); j > 0; --j) {
		res += " ";
        res += rotateDirName(rotateDirs[j-1]);
    }
    return res;
}

static std::string printMovesRev(const CubesAtDepth *cubesByDepth,
		const cube &c)
{
	unsigned short cornersPerm = cornersPermToIdx(c.cc.getPerms());
	int depth = 0;
	while( true ) {
        const CornerOrientCubes *corientCubes =
            cubesByDepth[depth][cornersPerm].cornerOrientCubes(c.cc.getOrients());
		if( corientCubes != NULL && corientCubes->containsCubeEdges(c.ce) )
			break;
		++depth;
        if( depth == DEPTH_MAX+1)
            break;
	}
	cube cc = c;
    std::string res;
	while( depth-- > 0 ) {
		int cm = 0;
		cube cc1;
		while( true ) {
			cc1 = cubesCompose(cc, crotated[cm]);
			cornersPerm = cornersPermToIdx(cc1.cc.getPerms());
            const CornerOrientCubes *corientCubes =
                cubesByDepth[depth][cornersPerm].cornerOrientCubes(cc1.cc.getOrients());
			if( corientCubes != NULL && corientCubes->containsCubeEdges(cc1.ce) )
				break;
			++cm;
		}
        res += " ";
        res += rotateDirName(cm);
		cc = cc1;
	}
    return res;
}

static std::string printMovesRevDeep(const CubesAtDepth *cubesByDepth,
		const cube &c)
{
    for(int depthMax = 1;; ++depthMax) {
        for(int depth = depthMax - 1; depth <= depthMax; ++depth) {
            for(int cornerPerm = 0; cornerPerm < 40320; ++cornerPerm) {
                const CornerPermCubes &cpermCubes = cubesByDepth[depth][cornerPerm];
                cubecorner_perms ccp = cornersIdxToPerm(cornerPerm);
                cubecorner_perms ccpNew = cubecornerPermsCompose(c.cc.getPerms(), ccp);
                unsigned cornerPermSearch = cornersPermToIdx(ccpNew);
                const CornerPermCubes &cornerPermCubesSearch = cubesByDepth[depthMax][cornerPermSearch];
                for(int cornerOrientNo = 0; cornerOrientNo < cpermCubes.cornerOrientCubeCount(); ++cornerOrientNo) {
                    const CornerOrientCubes &corientCubes = cpermCubes.cornerOrientCubesAt(cornerOrientNo);
                    cubecorner_orients cco = corientCubes.cornerOrients();
                    cubecorner_orients ccoNew = cubecornerOrientsCompose(c.cc.getOrients(), ccp, cco);
                    const CornerOrientCubes *corientCubesSearch = cornerPermCubesSearch.cornerOrientCubes(ccoNew);
                    if( corientCubesSearch != NULL ) {
                        for(int edgeNo = 0; edgeNo < corientCubes.edgeCount(); ++edgeNo) {
                            const cubeedges ce = corientCubes.cubeedgesAt(edgeNo);
                            cubeedges ceNew = cubeedges::compose(c.ce, ce);
                            if( corientCubesSearch->containsCubeEdges(ceNew) ) {
                                cube c = { .cc = cubecorners(ccp, cco), .ce = ce };
                                std::string moves = printMoves(cubesByDepth, c);
                                cube cnew = { .cc = cubecorners(ccpNew, ccoNew), .ce = ceNew };
                                moves += printMovesRev(cubesByDepth, cnew);
                                return moves;
                            }
                        }
                    }
                }
            }
        }
    }
}

static void addCubesT(CubesAtDepth *cubesByDepth,
		int depth, int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
	unsigned cubeCountT = 0;
    const CubesAtDepth &cubesP = cubesByDepth[depth-2];
    const CubesAtDepth &cubesC = cubesByDepth[depth-1];
	for(int cornerPerm = 0; cornerPerm < 40320; ++cornerPerm) {
		const CornerPermCubes &cpermCubes = cubesC[cornerPerm];
        if( cpermCubes.cornerOrientCubeCount() == 0 )
            continue;
		cubecorner_perms ccp = cornersIdxToPerm(cornerPerm);
		for(int rd = 0; rd < RCOUNT; ++rd) {
            cubecorner_perms ccpNew = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
			unsigned short cornerPermNew = cornersPermToIdx(ccpNew);
			if( cornerPermNew % threadCount != threadNo )
                continue;
            for(int corientNo = 0; corientNo < cpermCubes.cornerOrientCubeCount(); ++corientNo) {
                const CornerOrientCubes &corientCubes = cpermCubes.cornerOrientCubesAt(corientNo);
                cubecorner_orients cco = corientCubes.cornerOrients();
                cubecorner_orients ccoNew = cubecornerOrientsCompose(cco, crotated[rd].cc);
                const CornerOrientCubes *corientCubesNewP = depth == 1 ? NULL :
                    cubesP[cornerPermNew].cornerOrientCubes(ccoNew);
                const CornerOrientCubes *corientCubesNewC =
                    cubesC[cornerPermNew].cornerOrientCubes(ccoNew);
                for(int edgeNo = 0; edgeNo < corientCubes.edgeCount(); ++edgeNo) {
                    const cubeedges ce = corientCubes.cubeedgesAt(edgeNo);
                    cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
                    if( (corientCubesNewP == NULL || !corientCubesNewP->containsCubeEdges(cenew) ) &&
                        (corientCubesNewC == NULL || !corientCubesNewC->containsCubeEdges(cenew) ) )
                    {
                        CornerOrientCubes &corientCubesNewN =
                            cubesByDepth[depth][cornerPermNew].cornerOrientCubesAddIfAbsent(ccoNew);
                        if( corientCubesNewN.addCube(cenew) )
                            ++cubeCountT;
                    }
                }
            }
		}
	}
	cubeCount->fetch_add(cubeCountT);
}

static void addCubes(CubesAtDepth *cubesByDepth, int depth,
        int threadCount, int fdReq)
{
    cubesByDepth[depth].init();
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesT, cubesByDepth, depth, t, threadCount,
                &cubeCount);
    }
    addCubesT(cubesByDepth, depth, 0, threadCount, &cubeCount);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d cubes=%u\n", fmt_time().c_str(), depth,
            cubeCount.load());
}

static void addCubesTrepr(const CubesAtDepth *cubesByDepth,
        CubesReprAtDepth *cubesReprByDepth,
		int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
	unsigned cubeCountT = 0;
    std::vector<int> otransform, otransformNew;

	for(int cornerPerm = 0; cornerPerm < 40320; ++cornerPerm) {
		const CornerPermCubes &cpermCubes = cubesByDepth[DEPTH_MAX][cornerPerm];
        if( cpermCubes.cornerOrientCubeCount() == 0 )
            continue;
		cubecorner_perms ccp = cornersIdxToPerm(cornerPerm);
        if( !(ccp == cubecornerPermsRepresentative(ccp)) )
            continue;
		for(int rd = 0; rd < RCOUNT; ++rd) {
            cubecorner_perms ccpNew = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
            unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpNew);
            if( cornerPermReprIdx % threadCount != threadNo )
                continue;
            cubecorner_perms ccpReprNew = cubecornerPermsRepresentative(ccpNew);
            unsigned short cornerPermReprNew = cornersPermToIdx(ccpReprNew);
            for(int cornerOrientNo = 0; cornerOrientNo < cpermCubes.cornerOrientCubeCount(); ++cornerOrientNo) {
                const CornerOrientCubes &corientCubes = cpermCubes.cornerOrientCubesAt(cornerOrientNo);
                cubecorner_orients cco = corientCubes.cornerOrients();
                cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(ccp, cco, otransform);
                if( !(cco == ccoRepr) )
                    continue;
                cubecorner_orients ccoNew = cubecornerOrientsCompose(cco, crotated[rd].cc);
                cubecorner_orients ccoReprNew = cubecornerOrientsRepresentative(ccpNew, ccoNew, otransformNew);
                for(int edgeNo = 0; edgeNo < corientCubes.edgeCount(); ++edgeNo) {
                    const cubeedges ce = corientCubes.cubeedgesAt(edgeNo);
                    if( !(ce == cubeedgesRepresentative(ce, otransform)) )
                        continue;
                    cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
                    cubeedges cenewRepr = cubeedgesRepresentative(cenew, otransformNew);
                    const CornerOrientCubes *orientReprCubesNewP = DEPTH_MAX == 0 ? NULL :
                        cubesByDepth[DEPTH_MAX-1][cornerPermReprNew].cornerOrientCubes(ccoReprNew);
                    if( orientReprCubesNewP != NULL && orientReprCubesNewP->containsCubeEdges(cenewRepr) )
                        continue;
                    const CornerOrientCubes *orientReprCubesNewC =
                        cubesByDepth[DEPTH_MAX][cornerPermReprNew].cornerOrientCubes(ccoReprNew);
                    if( orientReprCubesNewC != NULL && orientReprCubesNewC->containsCubeEdges(cenewRepr) )
                        continue;
                    unsigned cornerOrientIdx = cornersOrientToIdx(ccoReprNew);
                    if( cubesReprByDepth[0][cornerPermReprIdx].addCube(cornerOrientIdx, cenewRepr) )
                        ++cubeCountT;
                }
            }
        }
	}
	cubeCount->fetch_add(cubeCountT);
}

static void addCubesRepr(const CubesAtDepth *cubesByDepth,
        CubesReprAtDepth *cubesReprByDepth, int threadCount, int fdReq)
{
    cubesReprByDepth[0].init();
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesTrepr, cubesByDepth, cubesReprByDepth, t, threadCount,
                &cubeCount);
    }
    addCubesTrepr(cubesByDepth, cubesReprByDepth, 0, threadCount, &cubeCount);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d repr cubes=%u\n", fmt_time().c_str(), DEPTH_MAX+1,
            cubeCount.load());
}

static void addCubesTrepr2(const CubesAtDepth *cubesByDepth,
        CubesReprAtDepth *cubesReprByDepth,
		int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
    unsigned cubeCountT = 0;
    std::vector<int> otransform, otransformNew;

    for(int cornerPermReprIdx = 0; cornerPermReprIdx < 1844; ++cornerPermReprIdx) {
        const CornerPermReprCubes &cornerPermReprCubes = cubesReprByDepth[0][cornerPermReprIdx];
        cubecorner_perms ccp = gPermRepr[cornerPermReprIdx];
        for(int rd = 0; rd < RCOUNT; ++rd) {
            cubecorner_perms ccpNew = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
            unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpNew);
            if( cornerPermReprIdx % threadCount != threadNo )
                continue;
            cubecorner_perms ccpReprNew = cubecornerPermsRepresentative(ccpNew);
            unsigned short cornerPermReprNew = cornersPermToIdx(ccpReprNew);
            for(int cornerOrientIdx = 0; cornerOrientIdx < 2187; ++cornerOrientIdx) {
                if( cornerPermReprCubes.edgeCount(cornerOrientIdx) == 0 )
                    continue;
                cubecorner_orients cco = cornersIdxToOrient(cornerOrientIdx);
                cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(ccp, cco, otransform);
                if( !(cco == ccoRepr) )
                    continue;
                cubecorner_orients ccoNew = cubecornerOrientsCompose(cco, crotated[rd].cc);
                cubecorner_orients ccoReprNew = cubecornerOrientsRepresentative(ccpNew, ccoNew, otransformNew);
                for(int edgeNo = 0; edgeNo < cornerPermReprCubes.edgeCount(cornerOrientIdx); ++edgeNo) {
                    const cubeedges ce = cornerPermReprCubes.cubeedgesAt(cornerOrientIdx, edgeNo);
                    if( !(ce == cubeedgesRepresentative(ce, otransform)) )
                        continue;
                    cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
                    cubeedges cenewRepr = cubeedgesRepresentative(cenew, otransformNew);
                    const CornerOrientCubes *orientReprCubesNewP =
                        cubesByDepth[DEPTH_MAX][cornerPermReprNew].cornerOrientCubes(ccoReprNew);
                    if( orientReprCubesNewP != NULL && orientReprCubesNewP->containsCubeEdges(cenewRepr) )
                        continue;
                    unsigned cornerOrientIdx = cornersOrientToIdx(ccoReprNew);
                    if( cubesReprByDepth[0][cornerPermReprIdx].containsCubeEdges(cornerOrientIdx, cenewRepr) )
                        continue;
                    if( cubesReprByDepth[1][cornerPermReprIdx].addCube(cornerOrientIdx, cenewRepr) )
                        ++cubeCountT;
                }
            }
        }
    }
    cubeCount->fetch_add(cubeCountT);
}

static void addCubesRepr2(const CubesAtDepth *cubesByDepth,
        CubesReprAtDepth *cubesReprByDepth,
        int threadCount, int fdReq)
{
    cubesReprByDepth[1].init();
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesTrepr2, cubesByDepth, cubesReprByDepth,
                t, threadCount, &cubeCount);
    }
    addCubesTrepr2(cubesByDepth, cubesReprByDepth, 0, threadCount, &cubeCount);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d repr cubes=%u\n", fmt_time().c_str(), DEPTH_MAX+2,
            cubeCount.load());
}

static void addCubesTrepr3(
        CubesReprAtDepth *cubesReprByDepth,
		int depthRepr, int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
    unsigned cubeCountT = 0;
    std::vector<int> otransform, otransformNew;

    for(int cornerPermReprIdx = 0; cornerPermReprIdx < 1844; ++cornerPermReprIdx) {
        const CornerPermReprCubes &cornerPermReprCubes = cubesReprByDepth[depthRepr-1][cornerPermReprIdx];
        cubecorner_perms ccp = gPermRepr[cornerPermReprIdx];
        for(int rd = 0; rd < RCOUNT; ++rd) {
            cubecorner_perms ccpNew = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
            unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpNew);
            if( cornerPermReprIdx % threadCount != threadNo )
                continue;
            for(int cornerOrientIdx = 0; cornerOrientIdx < 2187; ++cornerOrientIdx) {
                if( cornerPermReprCubes.edgeCount(cornerOrientIdx) == 0 )
                    continue;
                cubecorner_orients cco = cornersIdxToOrient(cornerOrientIdx);
                cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(ccp, cco, otransform);
                if( !(cco == ccoRepr) )
                    continue;
                cubecorner_orients ccoNew = cubecornerOrientsCompose(cco, crotated[rd].cc);
                cubecorner_orients ccoReprNew = cubecornerOrientsRepresentative(ccpNew, ccoNew, otransformNew);
                for(int edgeNo = 0; edgeNo < cornerPermReprCubes.edgeCount(cornerOrientIdx); ++edgeNo) {
                    const cubeedges ce = cornerPermReprCubes.cubeedgesAt(cornerOrientIdx, edgeNo);
                    if( !(ce == cubeedgesRepresentative(ce, otransform)) )
                        continue;
                    cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
                    cubeedges cenewRepr = cubeedgesRepresentative(cenew, otransformNew);
                    unsigned cornerOrientIdx = cornersOrientToIdx(ccoReprNew);
                    if( !cubesReprByDepth[depthRepr-2][cornerPermReprIdx].containsCubeEdges(cornerOrientIdx, cenewRepr) &&
                            !cubesReprByDepth[depthRepr-1][cornerPermReprIdx].containsCubeEdges(cornerOrientIdx, cenewRepr) )
                    {
                        if( cubesReprByDepth[depthRepr][cornerPermReprIdx].addCube(cornerOrientIdx, cenewRepr) )
                            ++cubeCountT;
                    }
                }
            }
        }
    }
    cubeCount->fetch_add(cubeCountT);
}

static void addCubesRepr3(
        CubesReprAtDepth *cubesReprByDepth,
        int depthRepr, int threadCount, int fdReq)
{
    cubesReprByDepth[depthRepr].init();
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesTrepr3, cubesReprByDepth,
                depthRepr, t, threadCount, &cubeCount);
    }
    addCubesTrepr3(cubesReprByDepth, depthRepr, 0, threadCount, &cubeCount);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d repr cubes=%u\n", fmt_time().c_str(), DEPTH_MAX+depthRepr+1,
            cubeCount.load());
}

static void searchMovesTa(const CubesAtDepth *cubesByDepth,
		const cube *csearch, int depth, int depthMax, std::atomic_int *lastCornersPerm,
		std::atomic_bool *isFinish, int reqFd)
{
    int cornerPerm;

	while( (cornerPerm = ++*lastCornersPerm) < 40320 && !isFinish->load() ) {
        const CornerPermCubes &cpermCubes = cubesByDepth[depth][cornerPerm];
        cubecorner_perms ccp = cornersIdxToPerm(cornerPerm);
        cubecorner_perms ccpSearch = cubecornerPermsCompose(ccp, csearch->cc.getPerms());
        unsigned cornerPermSearch = cornersPermToIdx(ccpSearch);
        const CornerPermCubes &cornerPermCubesSearch = cubesByDepth[depthMax][cornerPermSearch];
        for(int cornerOrientNo = 0; cornerOrientNo < cpermCubes.cornerOrientCubeCount(); ++cornerOrientNo) {
            const CornerOrientCubes &corientCubes = cpermCubes.cornerOrientCubesAt(cornerOrientNo);
            cubecorner_orients cco = corientCubes.cornerOrients();
            cubecorner_orients ccoSearch = cubecornerOrientsCompose(cco, csearch->cc);
            const CornerOrientCubes *corientCubesSearch = cornerPermCubesSearch.cornerOrientCubes(ccoSearch);
            if( corientCubesSearch != NULL ) {
                for(int edgeNo = 0; edgeNo < corientCubes.edgeCount(); ++edgeNo) {
                    const cubeedges ce = corientCubes.cubeedgesAt(edgeNo);
                    cubeedges ceSearch = cubeedges::compose(ce, csearch->ce);
                    if( corientCubesSearch->containsCubeEdges(ceSearch) ) {
                        cube c = { .cc = cubecorners(ccp, cco), .ce = ce };
                        std::string moves = "solution:";
                        cube cnew = { .cc = cubecorners(ccpSearch, ccoSearch), .ce = ceSearch };
                        moves += printMovesRev(cubesByDepth, cnew);
                        moves += printMoves(cubesByDepth, c);
                        moves += "\n";
                        flockfile(stdout);
                        sendRespMessage(reqFd, moves.c_str());
                        funlockfile(stdout);
                        isFinish->store(true);
                        return;
                    }
                }
            }
        }
    }
}

static bool searchMovesA(const CubesAtDepth *cubesByDepth,
		const cube &csearch, int depth, int depthMax,
        int threadCount, int fdReq)
{
    std::thread threads[threadCount];
    std::atomic_bool isFinish(false);
    std::atomic_int idx(-1);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTa, cubesByDepth, &csearch, depth, depthMax,
                &idx, &isFinish, fdReq);
    }
    searchMovesTa(cubesByDepth, &csearch, depth, depthMax, &idx, &isFinish, fdReq);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    if( isFinish.load() ) {
        sendRespMessage(fdReq, "%s -- %d moves --\n", fmt_time().c_str(), depth+depthMax);
        return true;
    }
    sendRespMessage(fdReq, "%s depth %d end\n", fmt_time().c_str(), depth+depthMax);
    return false;
}

static void searchMovesTaRepr(const CubesAtDepth *cubesByDepth,
        const CubesReprAtDepth *cubesReprByDepth,
		const cube *csearch, std::atomic_int *lastCornersPerm,
		std::atomic_bool *isFinish, int reqFd)
{
    int cornerPerm;
    std::vector<int> otransform;

	while( (cornerPerm = ++*lastCornersPerm) < 40320 && !isFinish->load() ) {
        const CornerPermCubes &cpermCubes = cubesByDepth[DEPTH_MAX][cornerPerm];
        cubecorner_perms ccp = cornersIdxToPerm(cornerPerm);
        cubecorner_perms ccpSearch = cubecornerPermsCompose(ccp, csearch->cc.getPerms());
        unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpSearch);
        const CornerPermReprCubes &cpermReprCubes = cubesReprByDepth[0][cornerPermReprIdx];
        for(int cornerOrientNo = 0; cornerOrientNo < cpermCubes.cornerOrientCubeCount(); ++cornerOrientNo) {
            const CornerOrientCubes &corientCubes = cpermCubes.cornerOrientCubesAt(cornerOrientNo);
            cubecorner_orients cco = corientCubes.cornerOrients();
            cubecorner_orients ccoSearch = cubecornerOrientsCompose(cco, csearch->cc);
            cubecorner_orients ccoSearchRepr = cubecornerOrientsRepresentative(ccpSearch, ccoSearch, otransform);
            unsigned cornerOrientIdx = cornersOrientToIdx(ccoSearchRepr);
            if( cpermReprCubes.edgeCount(cornerOrientIdx) == 0 )
                continue;
            for(int edgeNo = 0; edgeNo < corientCubes.edgeCount(); ++edgeNo) {
                const cubeedges ce = corientCubes.cubeedgesAt(edgeNo);
                cubeedges ceSearch = cubeedges::compose(ce, csearch->ce);
                cubeedges ceSearchRepr = cubeedgesRepresentative(ceSearch, otransform);
                if( cpermReprCubes.containsCubeEdges(cornerOrientIdx, ceSearchRepr) ) {
                    cube c = { .cc = cubecorners(ccp, cco), .ce = ce };
                    std::string moves = "solution:";
                    cube csearch = { .cc = cubecorners(ccpSearch, ccoSearch), .ce = ceSearch };
                    moves += printMovesRevDeep(cubesByDepth, csearch);
                    moves += printMoves(cubesByDepth, c);
                    moves += "\n";
                    flockfile(stdout);
                    sendRespMessage(reqFd, moves.c_str());
                    funlockfile(stdout);
                    isFinish->store(true);
                    return;
                }
            }
        }
    }
}

static bool searchMovesArepr(const CubesAtDepth *cubesByDepth,
        const CubesReprAtDepth *cubesReprByDepth,
		const cube &csearch, int depthRepr, int threadCount, int fdReq)
{
    std::thread threads[threadCount];
    std::atomic_bool isFinish(false);
    std::atomic_int idx(-1);
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTaRepr, cubesByDepth, cubesReprByDepth+depthRepr, &csearch,
                &idx, &isFinish, fdReq);
    }
    searchMovesTaRepr(cubesByDepth, cubesReprByDepth+depthRepr, &csearch, &idx, &isFinish, fdReq);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    if( isFinish.load() ) {
        sendRespMessage(fdReq, "%s -- %d moves --\n", fmt_time().c_str(), 2*DEPTH_MAX+depthRepr+1);
        return true;
    }
    sendRespMessage(fdReq, "%s depth %d end\n", fmt_time().c_str(), 2*DEPTH_MAX+depthRepr+1);
    return false;
}

struct SearchProgress {
    static std::mutex progressMutex;
    unsigned nextCornerPerm1Idx;
    unsigned nextCornerPerm2Idx;
    bool isFinish;
    std::vector<unsigned> filledCornerPerm1;
    std::vector<unsigned> filledCornerPerm2;
    const int threadCount;
    int runningThreadCount;

    bool inc(int reqFd, unsigned *cornerPerm1Buf,
            std::vector<unsigned>::const_iterator *cornerPerm2BegBuf,
            std::vector<unsigned>::const_iterator *cornerPerm2EndBuf)
    {
        bool res;
        unsigned cornerPerm1Idx, cornerPerm2Idx;
        progressMutex.lock();
        if( cornerPerm1Buf == NULL )
            isFinish = true;
        res = !isFinish && nextCornerPerm1Idx < filledCornerPerm1.size();
        if( res ) {
            cornerPerm1Idx = nextCornerPerm1Idx;
            cornerPerm2Idx = nextCornerPerm2Idx;
            nextCornerPerm2Idx += 1000;
            if( nextCornerPerm2Idx >= filledCornerPerm2.size() ) {
                ++nextCornerPerm1Idx;
                nextCornerPerm2Idx = 0;
            }
        }else
            --runningThreadCount;
        progressMutex.unlock();
        if( res ) {
            *cornerPerm1Buf = filledCornerPerm1[cornerPerm1Idx];
            *cornerPerm2BegBuf = filledCornerPerm2.begin() + cornerPerm2Idx;
            *cornerPerm2EndBuf = cornerPerm2Idx + 1000 >= filledCornerPerm2.size() ?
                filledCornerPerm2.end() : *cornerPerm2BegBuf + 1000;
            if( cornerPerm2Idx == 0 ) {
                unsigned itemCount = filledCornerPerm1.size();
                sendRespMessage(reqFd, "progress: %d of %d, %.0f%%\n",
                        cornerPerm1Idx, itemCount, 100.0 * cornerPerm1Idx / itemCount);
            }
        }else{
            sendRespMessage(reqFd, "progress: %d threads still running\n",
                    runningThreadCount);
        }
        return res;
    }

    SearchProgress(int threadCount)
        : nextCornerPerm1Idx(0), nextCornerPerm2Idx(0), isFinish(false),
        threadCount(threadCount), runningThreadCount(threadCount)
    {
    }
};

std::mutex SearchProgress::progressMutex;

static void searchMovesTb(const CubesAtDepth *cubesByDepth,
        const CubesReprAtDepth *cubesReprByDepth,
		const cube *csearch, int depth,
        int reqFd, SearchProgress *searchProgress)
{
    unsigned cornerPerm1;
    std::vector<int> o2transform;

    std::vector<unsigned>::const_iterator cornerPerm2Beg, cornerPerm2End;
	while( searchProgress->inc(reqFd, &cornerPerm1, &cornerPerm2Beg, &cornerPerm2End) ) {
		const CornerPermCubes &cpermCubes1 = cubesByDepth[depth][cornerPerm1];
		cubecorner_perms ccp1 = cornersIdxToPerm(cornerPerm1);
		cubecorner_perms ccpSearch1 = cubecornerPermsCompose(ccp1, csearch->cc.getPerms());
        for(std::vector<unsigned>::const_iterator cornerPerm2It = cornerPerm2Beg;
                cornerPerm2It != cornerPerm2End; ++cornerPerm2It)
        {
            unsigned cornerPerm2 = *cornerPerm2It;
            const CornerPermCubes &cpermCubes2 = cubesByDepth[DEPTH_MAX][cornerPerm2];
            cubecorner_perms ccp2 = cornersIdxToPerm(cornerPerm2);
            cubecorner_perms ccpSearch2 = cubecornerPermsCompose(ccp2, ccpSearch1);
            unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpSearch2);
            const CornerPermReprCubes &cpermReprCubes = cubesReprByDepth[0][cornerPermReprIdx];
            for(int cornerOrient1No = 0; cornerOrient1No < cpermCubes1.cornerOrientCubeCount(); ++cornerOrient1No) {
                const CornerOrientCubes &corientCubes1 = cpermCubes1.cornerOrientCubesAt(cornerOrient1No);
                cubecorner_orients cco1 = corientCubes1.cornerOrients();
                cubecorner_orients ccoSearch1 = cubecornerOrientsCompose(cco1, csearch->cc);
                for(int cornerOrient2No = 0; cornerOrient2No < cpermCubes2.cornerOrientCubeCount(); ++cornerOrient2No) {
                    const CornerOrientCubes &corientCubes2 = cpermCubes2.cornerOrientCubesAt(cornerOrient2No);
                    cubecorner_orients cco2 = corientCubes2.cornerOrients();
                    cubecorner_orients ccoSearch2 = cubecornerOrientsCompose(cco2, ccpSearch1, ccoSearch1);
                    cubecorner_orients ccoSearch2Repr = cubecornerOrientsRepresentative(
                            ccpSearch2, ccoSearch2, o2transform);
                    unsigned cornerOrientIdx = cornersOrientToIdx(ccoSearch2Repr);
                    if( cpermReprCubes.edgeCount(cornerOrientIdx) == 0 )
                        continue;
                    for(int edge1No = 0; edge1No < corientCubes1.edgeCount(); ++edge1No)
                    {
                        const cubeedges ce1 = corientCubes1.cubeedgesAt(edge1No);
                        cubeedges ceSearch1 = cubeedges::compose(ce1, csearch->ce);
                        for(int edge2No = 0; edge2No < corientCubes2.edgeCount(); ++edge2No)
                        {
                            const cubeedges ce2 = corientCubes2.cubeedgesAt(edge2No);
                            cubeedges cesearch2 = cubeedges::compose(ce2, ceSearch1);
                            cubeedges cesearch2Repr = cubeedgesRepresentative(cesearch2, o2transform);
                            if( cpermReprCubes.containsCubeEdges(cornerOrientIdx, cesearch2Repr) )
                            {
                                std::string moves = "solution:";
                                cube csearch2 = {
                                    .cc = cubecorners(ccpSearch2, ccoSearch2),
                                    .ce = cesearch2
                                };
                                moves += printMovesRevDeep(cubesByDepth, csearch2);

                                cube csearch1 = {
                                    .cc = cubecorners(ccpSearch1, ccoSearch1),
                                    .ce = ceSearch1
                                };
                                cube c2 = { .cc = cubecorners(ccp2, cco2), .ce = ce2 };
                                moves += printMoves(cubesByDepth, c2);

                                cube c1 = { .cc = cubecorners(ccp1, cco1), .ce = ce1 };
                                moves += printMoves(cubesByDepth, c1);
                                moves += "\n";
                                flockfile(stdout);
                                sendRespMessage(reqFd, moves.c_str());
                                funlockfile(stdout);
                                searchProgress->inc(reqFd, NULL, NULL, NULL);
                                return;
                            }
                        }
                    }
                }
            }
        }
	}
}

static bool searchMovesB(const CubesAtDepth *cubesByDepth,
        const CubesReprAtDepth *cubesReprByDepth,
		const cube &csearch, int depth, int depthRepr, int threadCount, int fdReq)
{
    SearchProgress searchProgress(threadCount);
    for(unsigned i = 0; i < 40320; ++i) {
        if( cubesByDepth[depth][i].cornerOrientCubeCount() > 0 )
            searchProgress.filledCornerPerm1.push_back(i);
        if( cubesByDepth[DEPTH_MAX][i].cornerOrientCubeCount() > 0 )
            searchProgress.filledCornerPerm2.push_back(i);
    }
    std::thread threads[threadCount];
    for(int t = 1; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTb, cubesByDepth, cubesReprByDepth+depthRepr, &csearch, depth,
                fdReq, &searchProgress);
    }
    searchMovesTb(cubesByDepth, cubesReprByDepth+depthRepr, &csearch, depth, fdReq, &searchProgress);
    for(int t = 1; t < threadCount; ++t)
        threads[t].join();
    if( searchProgress.isFinish ) {
        sendRespMessage(fdReq, "%s -- %d moves --\n", fmt_time().c_str(), depth+2*DEPTH_MAX+depthRepr+1);
        return true;
    }
    sendRespMessage(fdReq, "%s depth %d end\n", fmt_time().c_str(), depth+2*DEPTH_MAX+depthRepr+1);
    return false;
}

static void searchMoves(const cube &csearch, int threadCount, int fdReq)
{
    static CubesAtDepth cubesByDepth[CSARR_SIZE];
    static CubesReprAtDepth cubesReprByDepth[CSREPR_SIZE];

    fmt_time(); // reset time elapsed
    if( cubesByDepth[0].empty() ) {
        cubesByDepth[0].init();
        unsigned short cornerPermSolved = cornersPermToIdx(csolved.cc.getPerms());
        CornerOrientCubes &corientCubes = cubesByDepth[0][cornerPermSolved].cornerOrientCubesAddIfAbsent(csolved.cc.getOrients());
        corientCubes.addCube(csolved.ce);
    }
	for(int depth = 1; depth <= DEPTH_MAX; ++depth) {
        if( cubesByDepth[depth].empty() )
            addCubes(cubesByDepth, depth, threadCount, fdReq);
        if( searchMovesA(cubesByDepth, csearch, depth-1, depth, threadCount, fdReq) )
            return;
        if( searchMovesA(cubesByDepth, csearch, depth, depth, threadCount, fdReq) )
            return;
    }
    if( cubesReprByDepth[0].empty() )
        addCubesRepr(cubesByDepth, cubesReprByDepth, threadCount, fdReq);
    if( searchMovesArepr(cubesByDepth, cubesReprByDepth, csearch, 0, threadCount, fdReq) )
        return;
    if( DREPR_MAX > 0 ) {
        if( cubesReprByDepth[1].empty() )
            addCubesRepr2(cubesByDepth, cubesReprByDepth, threadCount, fdReq);
        if( searchMovesArepr(cubesByDepth, cubesReprByDepth, csearch, 1, threadCount, fdReq) )
            return;
    }
    for(int depthRepr = 2; depthRepr <= DREPR_MAX; ++depthRepr) {
        if( cubesReprByDepth[depthRepr].empty() )
            addCubesRepr3(cubesReprByDepth, depthRepr, threadCount, fdReq);
        if( searchMovesArepr(cubesByDepth, cubesReprByDepth, csearch, depthRepr, threadCount, fdReq) )
            return;
    }
	for(int depth = 1; depth <= DEPTH_MAX; ++depth) {
        if( searchMovesB(cubesByDepth, cubesReprByDepth, csearch, depth, DREPR_MAX, threadCount, fdReq) )
            return;
	}
}

static void processCubeReq(int fdReq, const char *squareColors)
{
    char respHeader[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-type: text/plain\r\n"
        "Transfer-encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"; 
    write(fdReq, respHeader, strlen(respHeader));
    cube c;
	if( cubeRead(fdReq, squareColors, c) ) {
        int threadCount = sysconf(_SC_NPROCESSORS_ONLN);
        sendRespMessage(fdReq, "thread count: %d\n", threadCount);
        if( threadCount <= 0 )
            threadCount = 4;
        if( c == csolved )
            sendRespMessage(fdReq, "already solved\n");
        else
            searchMoves(c, threadCount, fdReq);
    }
    write(fdReq, "0\r\n\r\n", 5);
}

static void processRequest(int fdReq, const char *reqHeader) {
    if( strncasecmp(reqHeader, "get ", 4) ) {
        char respHeader[] =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-length: 0\r\n"
            "Allow: GET\r\n"
            "Connection: close\r\n"
            "\r\n"; 
        write(fdReq, respHeader, strlen(respHeader));
        return;
    }
    bool resourceFound = false;
    if( reqHeader[4] == '/' ) {
        if( reqHeader[5] == '?' ) {
            resourceFound = true;
            processCubeReq(fdReq, reqHeader+6);
        }else{
            const char *fnameBeg = reqHeader + 5;
            const char *fnameEnd = strchr(fnameBeg, ' ');
            if( fnameEnd != NULL ) {
                std::string fname;
                fname.assign(fnameBeg, fnameEnd - fnameBeg);
                if( fname.empty() )
                    fname = "cube.html";
                FILE *fp = fopen(fname.c_str(), "r");
                if( fp != NULL ) {
                    resourceFound = true;
                    fseek(fp, 0, SEEK_END);
                    long fsize = ftell(fp);
                    const char *fnameExt = strrchr(fname.c_str(), '.');
                    if( fnameExt == NULL )
                        fnameExt = "txt";
                    else
                        ++fnameExt;
                    const char *contentType = "application/octet-stream";
                    if( !strcasecmp(fnameExt, "html") )
                        contentType = "text/html; charset=utf-8";
                    else if( !strcasecmp(fnameExt, "css") )
                        contentType = "text/css";
                    else if( !strcasecmp(fnameExt, "js") )
                        contentType = "text/javascript";
                    else if( !strcasecmp(fnameExt, "txt") )
                        contentType = "text/plain";
                    char respHeader[4096];
                    sprintf(respHeader,
                        "HTTP/1.1 200 OK\r\n"
                        "Content-type: %s\r\n"
                        "Content-length: %ld\r\n"
                        "Connection: close\r\n"
                        "\r\n", contentType, fsize); 
                    write(fdReq, respHeader, strlen(respHeader));
                    rewind(fp);
                    char fbuf[32768];
                    long toWr = fsize;
                    while( toWr > 0 ) {
                        int toRd = toWr > sizeof(fbuf) ? sizeof(fbuf) : toWr;
                        int rd = fread(fbuf, 1, toRd, fp);
                        if( rd == 0 ) {
                            memset(fbuf, 0, toRd);
                            rd = toRd;
                        }
                        int wrTot = 0;
                        while( wrTot < rd ) {
                            int wr = write(fdReq, fbuf + wrTot, rd - wrTot);
                            if( wr < 0 ) {
                                perror("socket write");
                                return;
                            }
                            wrTot += wr;
                        }
                        toWr -= rd;
                    }
                    fclose(fp);
                }
            }
        }
    }
    if( ! resourceFound ) {
        char respHeader[] =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-length: 0\r\n"
            "Connection: close\r\n"
            "\r\n"; 
        write(fdReq, respHeader, strlen(respHeader));
    }
}

static void processConnection(int fdConn) {
    char reqHeaderBuf[16385] = { 0 }, *reqHeaderEnd;
    int rdTot = 0;
    while( true ) {
        int rd = read(fdConn, reqHeaderBuf + rdTot, sizeof(reqHeaderBuf) - rdTot - 1);
        if( rd < 0 ) {
            perror("read");
            return;
        }
        if( rd == 0 ) {
            printf("connection closed by peer\n");
            return;
        }
        write(STDOUT_FILENO, reqHeaderBuf+rdTot, rd);
        rdTot += rd;
        if( (reqHeaderEnd = strstr(reqHeaderBuf, "\r\n\r\n")) != NULL ) {
            printf("---- sending response ----\n");
            reqHeaderEnd[2] = '\0';
            processRequest(fdConn, reqHeaderBuf);
            return;
        }
        if( rdTot == sizeof(reqHeaderBuf)-1 ) {
            fprintf(stderr, "request header too long\n");
            return;
        }
    }
}

int main(int argc, char *argv[]) {
    int listenfd, acceptfd, opton = 1;
    struct sockaddr_in addr;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket");
        return 1;
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opton, sizeof(opton));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    if( bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0 ) {
        perror("bind");
        return 1;
    }
    if( listen(listenfd, 1) < 0 ) {
        perror("listen");
        return 1;
    }
    permReprInit();
    while( (acceptfd = accept(listenfd, NULL, NULL)) >= 0 ) {
        //setsockopt(acceptfd, IPPROTO_TCP, TCP_NODELAY, &opton, sizeof(opton));
        processConnection(acceptfd);
        close(acceptfd);
    }
    perror("accept");
	return 1;
}

