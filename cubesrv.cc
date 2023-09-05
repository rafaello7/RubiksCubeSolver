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
	static time_t tprev = time(NULL);
	time_t t = time(NULL);
    char buf[20] = { 0 };
    if( t - tprev > 1 )
        sprintf(buf, "%ds", (int)(t-tprev));
	tprev = t;
	return buf;
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
		unsigned char edge2perm = ce2.edges >> 5 * i & 0xf;
		unsigned char edge2orient = ce2.edges >> 5 * i & 0x10;
        unsigned char edge1item = ce1.edges >> 5 * edge2perm & 0x1f;
		res.edges |= (unsigned long)(edge1item ^ edge2orient) << 5*i;
	}
	return res;
}

struct cube {
	struct cubecorners cc;
	struct cubeedges ce;

	bool operator==(const cube &c) const;
	bool operator<(const cube &c) const;
};

bool cube::operator==(const cube &c) const
{ 
	return cc == c.cc && ce == c.ce;
}

bool cube::operator<(const cube &c) const
{ 
	return cc < c.cc || cc == c.cc && ce < c.ce;
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

static cubeedges cubeedgesRepresentative(const cubeedges &ce, std::vector<int> &transform)
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

static unsigned short cornersPermToIdx(const cubecorner_perms &ccp)
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

enum { CSARR_MAX = 7, CSREPR_MAX = 3 };

static int depthMaxInitFn() {
    long pageSize = sysconf(_SC_PAGESIZE);
    long pageCount = sysconf(_SC_PHYS_PAGES);
    long memSizeGB = pageSize * pageCount / 1073741824;
    int depth = std::min((int)CSARR_MAX-1, 8 - CSREPR_MAX + (memSizeGB >= 10));
    printf("page size: %ld, count: %ld, mem GB: %ld, depth: %d\n",
            pageSize, pageCount, memSizeGB, depth);
    return depth;
}

const int DEPTH_MAX = depthMaxInitFn();

class CubeSetArray {
	struct cubeset {
		cubecorner_orients orients;
		std::vector<cubeedges> items;

		cubeset(cubecorner_orients);
		void swap(cubeset &cs) {
			std::swap(orients, cs.orients);
			items.swap(cs.items);
		}
	};
	std::vector<cubeset> m_cubesetArr;

public:
	CubeSetArray();
	~CubeSetArray();
	bool addCube(unsigned orientNo, const cubeedges&);
	int cornerOrientIdx(const cubecorner_orients&) const;
	int cornerOrientIdxAddIfAbsent(cubecorner_orients);
	bool containsCubeEdges(int orientNo, const cubeedges&) const;
	int cornerOrientCount() const;
	cubecorner_orients cornerOrientsAt(int orientNo) const;
	int edgeCount(int orientNo) const;
	cubeedges cubeedgesAt(int orientNo, int edgeNo) const;
};

CubeSetArray::cubeset::cubeset(cubecorner_orients orients)
	: orients(orients)
{
}

CubeSetArray::CubeSetArray()
{
}

CubeSetArray::~CubeSetArray()
{
}

bool CubeSetArray::addCube(unsigned orientNo, const cubeedges &ce)
{
	cubeset &cs = m_cubesetArr[orientNo];
	unsigned lo = 0, hi = cs.items.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( cs.items[mi] < ce )
			lo = mi + 1;
		else
			hi = mi;
	}
	if( lo < cs.items.size() && cs.items[lo] == ce )
		return false;
	if( lo == cs.items.size() ) {
		cs.items.push_back(ce);
	}else{
		hi = cs.items.size();
		cs.items.push_back(cs.items[hi-1]);
		while( --hi > lo )
			cs.items[hi] = cs.items[hi-1];
		cs.items[hi] = ce;
	}
	return true;
}

int CubeSetArray::cornerOrientIdx(const cubecorner_orients &cco) const
{
	unsigned lo = 0, hi = m_cubesetArr.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_cubesetArr[mi].orients < cco )
			lo = mi + 1;
		else
			hi = mi;
	}
	return lo < m_cubesetArr.size() && cco == m_cubesetArr[lo].orients ?
		lo : -1;
}

int CubeSetArray::cornerOrientIdxAddIfAbsent(cubecorner_orients cco)
{
	unsigned lo = 0, hi = m_cubesetArr.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( m_cubesetArr[mi].orients < cco )
			lo = mi + 1;
		else
			hi = mi;
	}
	if( lo == m_cubesetArr.size() || cco < m_cubesetArr[lo].orients ) {
		m_cubesetArr.push_back(cubeset(cco));
		hi = m_cubesetArr.size();
		while( --hi > lo )
			m_cubesetArr[hi].swap(m_cubesetArr[hi-1]);
	}
	return lo;
}

bool CubeSetArray::containsCubeEdges(int orientNo, const cubeedges &ce) const
{
	const cubeset &cs = m_cubesetArr[orientNo];
	unsigned lo = 0, hi = cs.items.size();
	while( lo < hi ) {
		unsigned mi = (lo+hi) / 2;
		if( cs.items[mi] < ce )
			lo = mi + 1;
		else
			hi = mi;
	}
	return lo < cs.items.size() && cs.items[lo] == ce;
}

int CubeSetArray::cornerOrientCount() const
{
	return m_cubesetArr.size();
}

int CubeSetArray::edgeCount(int orientNo) const
{
	const cubeset &cs = m_cubesetArr[orientNo];
	return cs.items.size();
}

cubecorner_orients CubeSetArray::cornerOrientsAt(int orientNo) const
{
	return m_cubesetArr[orientNo].orients;
}

cubeedges CubeSetArray::cubeedgesAt(int orientNo, int edgeNo) const
{
	const cubeset &cs = m_cubesetArr[orientNo];
	return cs.items[edgeNo];
}

class CubeSetReprArray {
	std::vector<cubeedges> m_items[2187];

public:
	CubeSetReprArray();
	~CubeSetReprArray();
	bool addCube(unsigned orientIdx, const cubeedges&);
	bool containsCubeEdges(unsigned orientIdx, const cubeedges&) const;
    int edgeCount(unsigned orientIdx) const { return m_items[orientIdx].size(); }
	cubeedges cubeedgesAt(unsigned orientIdx, int edgeNo) const;
};

CubeSetReprArray::CubeSetReprArray()
{
}

CubeSetReprArray::~CubeSetReprArray()
{
}

bool CubeSetReprArray::addCube(unsigned orientIdx, const cubeedges &ce)
{
    std::vector<cubeedges> &items = m_items[orientIdx];
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

bool CubeSetReprArray::containsCubeEdges(unsigned orientIdx, const cubeedges &ce) const
{
    const std::vector<cubeedges> &items = m_items[orientIdx];
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

cubeedges CubeSetReprArray::cubeedgesAt(unsigned orientIdx, int edgeNo) const
{
	return m_items[orientIdx][edgeNo];
}

static std::string printMoves(const std::vector<CubeSetArray> *csArr, const cube &c)
{
	std::vector<int> rotateDirs;
	unsigned short cornersPerm = cornersPermToIdx(c.cc.getPerms());
	int depth = 0;
	while( true ) {
		int orientNo = csArr[depth][cornersPerm].cornerOrientIdx(c.cc.getOrients());
		if( orientNo >= 0 && csArr[depth][cornersPerm].containsCubeEdges(orientNo, c.ce) )
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
			int orientNo = csArr[depth][cornersPerm].cornerOrientIdx(cc1.cc.getOrients());
			if( orientNo >= 0 && csArr[depth][cornersPerm].containsCubeEdges(orientNo, cc1.ce) )
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

static std::string printMovesRev(const std::vector<CubeSetArray> *csArr,
		const cube &c)
{
	unsigned short cornersPerm = cornersPermToIdx(c.cc.getPerms());
	int depth = 0;
	while( true ) {
		int orientNo = csArr[depth][cornersPerm].cornerOrientIdx(c.cc.getOrients());
		if( orientNo >= 0 && csArr[depth][cornersPerm].containsCubeEdges(orientNo, c.ce) )
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
			int orientNo = csArr[depth][cornersPerm].cornerOrientIdx(cc1.cc.getOrients());
			if( orientNo >= 0 && csArr[depth][cornersPerm].containsCubeEdges(orientNo, cc1.ce) )
				break;
			++cm;
		}
        res += " ";
        res += rotateDirName(cm);
		cc = cc1;
	}
    return res;
}

static std::string printMovesRevDeep(const std::vector<CubeSetArray> *cubeSets,
		const cube &c)
{
    for(int depthMax = 1;; ++depthMax) {
        for(int depth = depthMax - 1; depth <= depthMax; ++depth) {
            for(int perm = 0; perm < 40320; ++perm) {
                const CubeSetArray &csArr = cubeSets[depth][perm];
                cubecorner_perms ccp = cornersIdxToPerm(perm);
                cubecorner_perms ccpNew = cubecornerPermsCompose(c.cc.getPerms(), ccp);
                unsigned permNew = cornersPermToIdx(ccpNew);
                const CubeSetArray &csArrNew = cubeSets[depthMax][permNew];
                for(int orientNo = 0; orientNo < csArr.cornerOrientCount(); ++orientNo) {
                    cubecorner_orients cco = csArr.cornerOrientsAt(orientNo);
                    cubecorner_orients ccoNew = cubecornerOrientsCompose(c.cc.getOrients(),
                            ccp, cco);
                    int cubesetNewIdx = csArrNew.cornerOrientIdx(ccoNew);
                    if( cubesetNewIdx >= 0 ) {
                        for(int edgeNo = 0; edgeNo < csArr.edgeCount(orientNo); ++edgeNo) {
                            const cubeedges ce = csArr.cubeedgesAt(orientNo, edgeNo);
                            cubeedges ceNew = cubeedges::compose(c.ce, ce);
                            if( csArrNew.containsCubeEdges(cubesetNewIdx, ceNew) ) {
                                cube c = { .cc = cubecorners(ccp, cco), .ce = ce };
                                std::string moves = printMoves(cubeSets, c);
                                cube cnew = { .cc = cubecorners(ccpNew, ccoNew), .ce = ceNew };
                                moves += printMovesRev(cubeSets, cnew);
                                return moves;
                            }
                        }
                    }
                }
            }
        }
    }
}

static void addCubesT(std::vector<CubeSetArray> *cubeSets,
		int depth, int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
	unsigned cubeCountT = 0;
	for(int perm = 0; perm < 40320; ++perm) {
		CubeSetArray &csArr = cubeSets[depth-1][perm];
		cubecorner_perms ccp = cornersIdxToPerm(perm);
		cubecorner_perms ccpNew[RCOUNT];
		unsigned short permNew[RCOUNT];
		bool isProc[RCOUNT];
		for(int rd = 0; rd < RCOUNT; ++rd) {
			ccpNew[rd] = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
			permNew[rd] = cornersPermToIdx(ccpNew[rd]);
			isProc[rd] = permNew[rd] % threadCount == threadNo;
		}
		for(int orientNo = 0; orientNo < csArr.cornerOrientCount(); ++orientNo) {
			cubecorner_orients cco = csArr.cornerOrientsAt(orientNo);
			cubecorner_orients ccoNew[RCOUNT];
			int orientNoNewP[RCOUNT];
			int orientNoNewC[RCOUNT];
			for(int rd = 0; rd < RCOUNT; ++rd) {
				if( !isProc[rd] )
					continue;
				ccoNew[rd] = cubecornerOrientsCompose(cco, crotated[rd].cc);
				orientNoNewP[rd] = depth == 1 ? -1 :
					cubeSets[depth-2][permNew[rd]].cornerOrientIdx(ccoNew[rd]);
				orientNoNewC[rd] = cubeSets[depth-1][permNew[rd]].cornerOrientIdx(ccoNew[rd]);
			}
			for(int edgeNo = 0; edgeNo < csArr.edgeCount(orientNo); ++edgeNo) {
				const cubeedges ce = csArr.cubeedgesAt(orientNo, edgeNo);
				for(int rd = 0; rd < RCOUNT; ++rd) {
					if( !isProc[rd] )
						continue;
					cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
					if( orientNoNewP[rd] != -1 &&
							cubeSets[depth-2][permNew[rd]].containsCubeEdges(
								orientNoNewP[rd], cenew) )
						continue;
					if( orientNoNewC[rd] != -1 &&
							cubeSets[depth-1][permNew[rd]].containsCubeEdges(
								orientNoNewC[rd], cenew) )
						continue;
					int orientNoNewN =
						cubeSets[depth][permNew[rd]].cornerOrientIdxAddIfAbsent(ccoNew[rd]);
					if( cubeSets[depth][permNew[rd]].addCube(orientNoNewN, cenew) ) {
						++cubeCountT;
					}
				}
			}
		}
	}
	cubeCount->fetch_add(cubeCountT);
}

static void addCubes(std::vector<CubeSetArray> *cubeSets, int depth,
        int threadCount, int fdReq)
{
    cubeSets[depth].resize(40320);
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 0; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesT, cubeSets, depth, t, threadCount,
                &cubeCount);
    }
    for(int t = 0; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d cubes=%u\n", fmt_time().c_str(), depth,
            cubeCount.load());
}

static void addCubesTrepr(const std::vector<CubeSetArray> *cubeSets,
        std::vector<CubeSetReprArray> *cubeSetsRepr,
		int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
	unsigned cubeCountT = 0;
	for(int perm = 0; perm < 40320; ++perm) {
		const CubeSetArray &csArr = cubeSets[DEPTH_MAX][perm];
		cubecorner_perms ccp = cornersIdxToPerm(perm);
        if( !(ccp == cubecornerPermsRepresentative(ccp)) )
            continue;
		cubecorner_perms ccpNew[RCOUNT];
		cubecorner_perms ccpReprNew[RCOUNT];
		unsigned short permReprNew[RCOUNT];
		bool isProc[RCOUNT];
		for(int rd = 0; rd < RCOUNT; ++rd) {
			ccpNew[rd] = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
            ccpReprNew[rd] = cubecornerPermsRepresentative(ccpNew[rd]);
			permReprNew[rd] = cornersPermToIdx(ccpReprNew[rd]);
			isProc[rd] = permReprNew[rd] % threadCount == threadNo;
		}
		for(int orientNo = 0; orientNo < csArr.cornerOrientCount(); ++orientNo) {
			cubecorner_orients cco = csArr.cornerOrientsAt(orientNo);
			std::vector<int> otransform;
            cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(ccp, cco, otransform);
            if( !(cco == ccoRepr) )
                continue;
			cubecorner_orients ccoNew[RCOUNT];
			cubecorner_orients ccoReprNew[RCOUNT];
			std::vector<int> otransformNew[RCOUNT];
			for(int rd = 0; rd < RCOUNT; ++rd) {
				if( !isProc[rd] )
					continue;
				ccoNew[rd] = cubecornerOrientsCompose(cco, crotated[rd].cc);
                ccoReprNew[rd] = cubecornerOrientsRepresentative(ccpNew[rd], ccoNew[rd], otransformNew[rd]);
			}
			for(int edgeNo = 0; edgeNo < csArr.edgeCount(orientNo); ++edgeNo) {
				const cubeedges ce = csArr.cubeedgesAt(orientNo, edgeNo);
                if( !(ce == cubeedgesRepresentative(ce, otransform)) )
                    continue;
				for(int rd = 0; rd < RCOUNT; ++rd) {
					if( !isProc[rd] )
						continue;
					cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
                    cubeedges cenewRepr = cubeedgesRepresentative(cenew, otransformNew[rd]);
                    int orientReprNoNewP = DEPTH_MAX == 0 ? -1 :
                        cubeSets[DEPTH_MAX-1][permReprNew[rd]].cornerOrientIdx(ccoReprNew[rd]);
					if( orientReprNoNewP != -1 &&
							cubeSets[DEPTH_MAX-1][permReprNew[rd]].containsCubeEdges(
								orientReprNoNewP, cenewRepr) )
						continue;
                    int orientReprNoNewC = cubeSets[DEPTH_MAX][permReprNew[rd]].cornerOrientIdx(ccoReprNew[rd]);
					if( orientReprNoNewC != -1 &&
							cubeSets[DEPTH_MAX][permReprNew[rd]].containsCubeEdges(
								orientReprNoNewC, cenewRepr) )
						continue;
                    unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpNew[rd]);
                    unsigned cornerOrientIdx = cornersOrientToIdx(ccoReprNew[rd]);
                    if( (*cubeSetsRepr)[cornerPermReprIdx].addCube(cornerOrientIdx, cenewRepr) ) {
                        ++cubeCountT;
                    }
				}
			}
		}
	}
	cubeCount->fetch_add(cubeCountT);
}

static void addCubesRepr(const std::vector<CubeSetArray> *cubeSets,
        std::vector<CubeSetReprArray> *cubeSetsRepr, int threadCount, int fdReq)
{
    cubeSetsRepr[0].resize(1844);
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 0; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesTrepr, cubeSets, cubeSetsRepr, t, threadCount,
                &cubeCount);
    }
    for(int t = 0; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d repr cubes=%u\n", fmt_time().c_str(), DEPTH_MAX+1,
            cubeCount.load());
}

static void addCubesTrepr2(const std::vector<CubeSetArray> *cubeSets,
        std::vector<CubeSetReprArray> *cubeSetsRepr,
		int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
	unsigned cubeCountT = 0;
	for(int permReprIdx = 0; permReprIdx < 1844; ++permReprIdx) {
		const CubeSetReprArray &csArr = cubeSetsRepr[0][permReprIdx];
		cubecorner_perms ccp = gPermRepr[permReprIdx];
		cubecorner_perms ccpNew[RCOUNT];
		cubecorner_perms ccpReprNew[RCOUNT];
		unsigned short permReprNew[RCOUNT];
		bool isProc[RCOUNT];
		for(int rd = 0; rd < RCOUNT; ++rd) {
			ccpNew[rd] = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
            ccpReprNew[rd] = cubecornerPermsRepresentative(ccpNew[rd]);
			permReprNew[rd] = cornersPermToIdx(ccpReprNew[rd]);
			isProc[rd] = permReprNew[rd] % threadCount == threadNo;
		}
		for(int orientIdx = 0; orientIdx < 2187; ++orientIdx) {
			cubecorner_orients cco = cornersIdxToOrient(orientIdx);
			std::vector<int> otransform;
            cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(ccp, cco, otransform);
            if( !(cco == ccoRepr) )
                continue;
			cubecorner_orients ccoNew[RCOUNT];
			cubecorner_orients ccoReprNew[RCOUNT];
			std::vector<int> otransformNew[RCOUNT];
			for(int rd = 0; rd < RCOUNT; ++rd) {
				if( !isProc[rd] )
					continue;
				ccoNew[rd] = cubecornerOrientsCompose(cco, crotated[rd].cc);
                ccoReprNew[rd] = cubecornerOrientsRepresentative(ccpNew[rd], ccoNew[rd], otransformNew[rd]);
			}
			for(int edgeNo = 0; edgeNo < csArr.edgeCount(orientIdx); ++edgeNo) {
				const cubeedges ce = csArr.cubeedgesAt(orientIdx, edgeNo);
                if( !(ce == cubeedgesRepresentative(ce, otransform)) )
                    continue;
				for(int rd = 0; rd < RCOUNT; ++rd) {
					if( !isProc[rd] )
						continue;
					cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
                    cubeedges cenewRepr = cubeedgesRepresentative(cenew, otransformNew[rd]);
                    int orientReprNoNewP =
                        cubeSets[DEPTH_MAX][permReprNew[rd]].cornerOrientIdx(ccoReprNew[rd]);
					if( orientReprNoNewP != -1 &&
							cubeSets[DEPTH_MAX][permReprNew[rd]].containsCubeEdges(
								orientReprNoNewP, cenewRepr) )
						continue;
                    unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpNew[rd]);
                    unsigned cornerOrientIdx = cornersOrientToIdx(ccoReprNew[rd]);
					if( cubeSetsRepr[0][cornerPermReprIdx].containsCubeEdges(cornerOrientIdx, cenewRepr) )
						continue;
                    if( cubeSetsRepr[1][cornerPermReprIdx].addCube(cornerOrientIdx, cenewRepr) ) {
                        ++cubeCountT;
                    }
				}
			}
		}
	}
	cubeCount->fetch_add(cubeCountT);
}

static void addCubesRepr2(const std::vector<CubeSetArray> *cubeSets,
        std::vector<CubeSetReprArray> *cubeSetsRepr,
        int threadCount, int fdReq)
{
    cubeSetsRepr[1].resize(1844);
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 0; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesTrepr2, cubeSets, cubeSetsRepr,
                t, threadCount, &cubeCount);
    }
    for(int t = 0; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d repr cubes=%u\n", fmt_time().c_str(), DEPTH_MAX+2,
            cubeCount.load());
}

static void addCubesTreprN(
        std::vector<CubeSetReprArray> *cubeSetsRepr,
		int depthRepr, int threadNo, int threadCount, std::atomic_uint *cubeCount)
{
	unsigned cubeCountT = 0;
	for(int permReprIdx = 0; permReprIdx < 1844; ++permReprIdx) {
		const CubeSetReprArray &csArr = cubeSetsRepr[depthRepr-1][permReprIdx];
		cubecorner_perms ccp = gPermRepr[permReprIdx];
		cubecorner_perms ccpNew[RCOUNT];
		bool isProc[RCOUNT];
		for(int rd = 0; rd < RCOUNT; ++rd) {
			ccpNew[rd] = cubecornerPermsCompose(ccp, crotated[rd].cc.getPerms());
            cubecorner_perms ccpReprNew = cubecornerPermsRepresentative(ccpNew[rd]);
			unsigned short permReprNew = cornersPermToIdx(ccpReprNew);
			isProc[rd] = permReprNew % threadCount == threadNo;
		}
		for(int orientIdx = 0; orientIdx < 2187; ++orientIdx) {
			cubecorner_orients cco = cornersIdxToOrient(orientIdx);
			std::vector<int> otransform;
            cubecorner_orients ccoRepr = cubecornerOrientsRepresentative(ccp, cco, otransform);
            if( !(cco == ccoRepr) )
                continue;
			cubecorner_orients ccoReprNew[RCOUNT];
			std::vector<int> otransformNew[RCOUNT];
			for(int rd = 0; rd < RCOUNT; ++rd) {
				if( !isProc[rd] )
					continue;
				cubecorner_orients ccoNew = cubecornerOrientsCompose(cco, crotated[rd].cc);
                ccoReprNew[rd] = cubecornerOrientsRepresentative(ccpNew[rd], ccoNew, otransformNew[rd]);
			}
			for(int edgeNo = 0; edgeNo < csArr.edgeCount(orientIdx); ++edgeNo) {
				const cubeedges ce = csArr.cubeedgesAt(orientIdx, edgeNo);
                if( !(ce == cubeedgesRepresentative(ce, otransform)) )
                    continue;
				for(int rd = 0; rd < RCOUNT; ++rd) {
					if( !isProc[rd] )
						continue;
					cubeedges cenew = cubeedges::compose(ce, crotated[rd].ce);
                    cubeedges cenewRepr = cubeedgesRepresentative(cenew, otransformNew[rd]);
                    unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpNew[rd]);
                    unsigned cornerOrientIdx = cornersOrientToIdx(ccoReprNew[rd]);
					if( cubeSetsRepr[depthRepr-2][cornerPermReprIdx].containsCubeEdges(cornerOrientIdx, cenewRepr) )
						continue;
					if( cubeSetsRepr[depthRepr-1][cornerPermReprIdx].containsCubeEdges(cornerOrientIdx, cenewRepr) )
						continue;
                    if( cubeSetsRepr[depthRepr][cornerPermReprIdx].addCube(cornerOrientIdx, cenewRepr) ) {
                        ++cubeCountT;
                    }
				}
			}
		}
	}
	cubeCount->fetch_add(cubeCountT);
}

static void addCubesReprN(
        std::vector<CubeSetReprArray> *cubeSetsRepr,
        int depthRepr, int threadCount, int fdReq)
{
    cubeSetsRepr[depthRepr].resize(1844);
    std::thread threads[threadCount];
    std::atomic_uint cubeCount(0);
    for(int t = 0; t < threadCount; ++t) {
        threads[t] = std::thread(addCubesTreprN, cubeSetsRepr,
                depthRepr, t, threadCount, &cubeCount);
    }
    for(int t = 0; t < threadCount; ++t)
        threads[t].join();
    sendRespMessage(fdReq, "%s depth %d repr cubes=%u\n", fmt_time().c_str(), DEPTH_MAX+depthRepr+1,
            cubeCount.load());
}

static void searchMovesTa(const std::vector<CubeSetArray> *cubeSets,
		const cube *csearch, int depth, int depthMax, std::atomic_int *lastCornersPerm,
		std::atomic_bool *isFinish, int reqFd)
{
    int perm;

	while( (perm = ++*lastCornersPerm) < 40320 && !isFinish->load() ) {
        const CubeSetArray &csArr = cubeSets[depth][perm];
        cubecorner_perms ccp = cornersIdxToPerm(perm);
        cubecorner_perms ccpNew = cubecornerPermsCompose(csearch->cc.getPerms(), ccp);
        unsigned permNew = cornersPermToIdx(ccpNew);
        const CubeSetArray &csArrNew = cubeSets[depthMax][permNew];
        for(int orientNo = 0; orientNo < csArr.cornerOrientCount(); ++orientNo) {
            cubecorner_orients cco = csArr.cornerOrientsAt(orientNo);
            cubecorner_orients ccoNew = cubecornerOrientsCompose(csearch->cc.getOrients(),
                    ccp, cco);
            int cubesetNewIdx = csArrNew.cornerOrientIdx(ccoNew);
            if( cubesetNewIdx >= 0 ) {
                for(int edgeNo = 0; edgeNo < csArr.edgeCount(orientNo); ++edgeNo) {
                    const cubeedges ce = csArr.cubeedgesAt(orientNo, edgeNo);
                    cubeedges ceNew = cubeedges::compose(csearch->ce, ce);
                    if( csArrNew.containsCubeEdges(cubesetNewIdx, ceNew) ) {
                        cube c = { .cc = cubecorners(ccp, cco), .ce = ce };
                        std::string moves = "solution:";
                        moves += printMoves(cubeSets, c);
                        cube cnew = { .cc = cubecorners(ccpNew, ccoNew), .ce = ceNew };
                        //cubePrint(cnew);
                        moves += printMovesRev(cubeSets, cnew);
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

static bool searchMovesA(const std::vector<CubeSetArray> *cubeSets,
		const cube &csearch, int depth, int depthMax,
        int threadCount, int fdReq)
{
    std::thread threads[threadCount];
    std::atomic_bool isFinish(false);
    std::atomic_int idx(-1);
    for(int t = 0; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTa, cubeSets, &csearch, depth, depthMax,
                &idx, &isFinish, fdReq);
    }
    for(int t = 0; t < threadCount; ++t)
        threads[t].join();
    if( isFinish.load() ) {
        sendRespMessage(fdReq, "%s -- %d moves --\n", fmt_time().c_str(), depth+depthMax);
        return true;
    }
    sendRespMessage(fdReq, "%s depth %d end\n", fmt_time().c_str(), depth+depthMax);
    return false;
}

static void searchMovesTaRepr(const std::vector<CubeSetArray> *cubeSets,
        const std::vector<CubeSetReprArray> *cubeSetsRepr,
		const cube *csearch, std::atomic_int *lastCornersPerm,
		std::atomic_bool *isFinish, int reqFd)
{
    int perm;

	while( (perm = ++*lastCornersPerm) < 40320 && !isFinish->load() ) {
        const CubeSetArray &csArr = cubeSets[DEPTH_MAX][perm];
        cubecorner_perms ccp = cornersIdxToPerm(perm);
        cubecorner_perms ccpNew = cubecornerPermsCompose(csearch->cc.getPerms(), ccp);
        unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpNew);
        const CubeSetReprArray &csArrRepr = (*cubeSetsRepr)[cornerPermReprIdx];
        for(int orientNo = 0; orientNo < csArr.cornerOrientCount(); ++orientNo) {
            cubecorner_orients cco = csArr.cornerOrientsAt(orientNo);
            cubecorner_orients ccoNew = cubecornerOrientsCompose(csearch->cc.getOrients(),
                    ccp, cco);
			std::vector<int> otransform;
            cubecorner_orients ccoNewRepr = cubecornerOrientsRepresentative(ccpNew, ccoNew, otransform);
            unsigned cornerOrientIdx = cornersOrientToIdx(ccoNewRepr);
            if( csArrRepr.edgeCount(cornerOrientIdx) == 0 )
                continue;
            for(int edgeNo = 0; edgeNo < csArr.edgeCount(orientNo); ++edgeNo) {
                const cubeedges ce = csArr.cubeedgesAt(orientNo, edgeNo);
                cubeedges ceNew = cubeedges::compose(csearch->ce, ce);
                cubeedges ceNewRepr = cubeedgesRepresentative(ceNew, otransform);
                if( csArrRepr.containsCubeEdges(cornerOrientIdx, ceNewRepr) ) {
                    flockfile(stdout);
                    cube c = { .cc = cubecorners(ccp, cco), .ce = ce };
                    std::string moves = "solution:";
                    moves += printMoves(cubeSets, c);
                    cube cnew = { .cc = cubecorners(ccpNew, ccoNew), .ce = ceNew };
                    //cubePrint(cnew);
                    moves += printMovesRev(cubeSets, cnew);
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

static bool searchMovesArepr(const std::vector<CubeSetArray> *cubeSets,
        const std::vector<CubeSetReprArray> *cubeSetsRepr,
		const cube &csearch, int threadCount, int fdReq)
{
    std::thread threads[threadCount];
    std::atomic_bool isFinish(false);
    std::atomic_int idx(-1);
    for(int t = 0; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTaRepr, cubeSets, cubeSetsRepr, &csearch,
                &idx, &isFinish, fdReq);
    }
    for(int t = 0; t < threadCount; ++t)
        threads[t].join();
    if( isFinish.load() ) {
        sendRespMessage(fdReq, "%s -- %d moves --\n", fmt_time().c_str(), 2*DEPTH_MAX+1);
        return true;
    }
    sendRespMessage(fdReq, "%s depth %d end\n", fmt_time().c_str(), 2*DEPTH_MAX+1);
    return false;
}

struct SearchProgress {
    static std::mutex progressMutex;
    int nonEmptyNo;
    int nonEmptyCount;
    bool isFinish;
    int threadCount;
    int runningThreadCount;

    bool inc(int reqFd, bool isNonEmpty, bool isFini, bool decThreadCount) {
        bool res = false;

        progressMutex.lock();
        if( isFini )
            isFinish = true;
        if( isFinish || runningThreadCount < threadCount )
            decThreadCount = true;
        if( decThreadCount )
            --runningThreadCount;
        if( isNonEmpty )
            ++nonEmptyNo;
        if( isFinish ) {
            sendRespMessage(reqFd, "progress: %d threads still running\n",
                    runningThreadCount);
        }else if( isNonEmpty && nonEmptyNo % 20 == 0 ) {
            sendRespMessage(reqFd, "progress: %d of %d, %.0f%%\n",
                    nonEmptyNo, nonEmptyCount, 100.0 * nonEmptyNo / nonEmptyCount);
        }
        res = decThreadCount;
        progressMutex.unlock();
        return res;
    }

    SearchProgress(int nonEmptyCount, int threadCount)
        : nonEmptyNo(0), nonEmptyCount(nonEmptyCount), isFinish(false),
        threadCount(threadCount), runningThreadCount(threadCount)
    {
    }
};

std::mutex SearchProgress::progressMutex;

static void searchMovesTb(const std::vector<CubeSetArray> *cubeSets,
        const std::vector<CubeSetReprArray> *cubeSetsRepr,
		const cube *csearch, int depth, std::atomic_int *lastCornersPerm,
		int reqFd, SearchProgress *searchProgress)
{
	int curPerm1;
	while( (curPerm1 = ++*lastCornersPerm) < 40320 ) {
		const CubeSetArray &csArr1 = cubeSets[depth][curPerm1];
		cubecorner_perms ccp1 = cornersIdxToPerm(curPerm1);
		cubecorner_perms ccpSearch1 = cubecornerPermsCompose(csearch->cc.getPerms(), ccp1);
		for(int orient1No = 0; orient1No < csArr1.cornerOrientCount(); ++orient1No) {
		   	cubecorner_orients cco1 = csArr1.cornerOrientsAt(orient1No);
			cubecorner_orients ccoSearch1 = cubecornerOrientsCompose(csearch->cc.getOrients(),
					ccp1, cco1);
			for(int curPerm2 = 0; curPerm2 < 40320; ++curPerm2) {
				const CubeSetArray &csArr2 = cubeSets[DEPTH_MAX][curPerm2];
				cubecorner_perms ccp2 = cornersIdxToPerm(curPerm2);
				cubecorner_perms ccpSearch2 = cubecornerPermsCompose(ccpSearch1, ccp2);
                unsigned cornerPermReprIdx = cubecornerPermRepresentativeIdx(ccpSearch2);
                const CubeSetReprArray &csArrRepr = (*cubeSetsRepr)[cornerPermReprIdx];
				for(int orient2No = 0; orient2No < csArr2.cornerOrientCount(); ++orient2No) {
					cubecorner_orients cco2 = csArr2.cornerOrientsAt(orient2No);
					cubecorner_orients ccoSearch2 = cubecornerOrientsCompose(ccoSearch1,
							ccp2, cco2);
					std::vector<int> o2transform;
					cubecorner_orients ccoSearch2Repr = cubecornerOrientsRepresentative(
                            ccpSearch2, ccoSearch2, o2transform);
                    unsigned cornerOrientIdx = cornersOrientToIdx(ccoSearch2Repr);
                    if( csArrRepr.edgeCount(cornerOrientIdx) == 0 )
                        continue;
                    for(int edge1No = 0; edge1No < csArr1.edgeCount(orient1No); ++edge1No)
                    {
                        const cubeedges ce1 = csArr1.cubeedgesAt(orient1No, edge1No);
                        cubeedges ceSearch1 = cubeedges::compose(csearch->ce, ce1);
                        for(int edge2No = 0; edge2No < csArr2.edgeCount(orient2No); ++edge2No)
                        {
                            const cubeedges ce2 = csArr2.cubeedgesAt(orient2No, edge2No);
                            cubeedges cesearch2 = cubeedges::compose(ceSearch1, ce2);
                            cubeedges cesearch2Repr = cubeedgesRepresentative(cesearch2, o2transform);
                            if( csArrRepr.containsCubeEdges(cornerOrientIdx, cesearch2Repr) )
                            {
                                cube c1 = { .cc = cubecorners(ccp1, cco1), .ce = ce1 };
                                std::string moves = "solution:";
                                moves += printMoves(cubeSets, c1);
                                cube csearch1 = {
                                    .cc = cubecorners(ccpSearch1, ccoSearch1),
                                    .ce = ceSearch1
                                };
                                //cubePrint(csearch1);
                                cube c2 = { .cc = cubecorners(ccp2, cco2), .ce = ce2 };
                                moves += printMoves(cubeSets, c2);
                                cube csearch2 = {
                                    .cc = cubecorners(ccpSearch2, ccoSearch2),
                                    .ce = cesearch2
                                };
                                //cubePrint(csearch2);
                                moves += printMovesRevDeep(cubeSets, csearch2);
                                moves += "\n";
                                flockfile(stdout);
                                sendRespMessage(reqFd, moves.c_str());
                                funlockfile(stdout);
                                searchProgress->inc(reqFd, true, true, true);
                                return;
                            }
                        }
					}
				}
			}
		}
        if( searchProgress->inc(reqFd, csArr1.cornerOrientCount() > 0, false, false) )
            return;
	}
    searchProgress->inc(reqFd, false, false, true);
}

static bool searchMovesB(const std::vector<CubeSetArray> *cubeSets,
        const std::vector<CubeSetReprArray> *cubeSetsRepr,
		const cube &csearch, int depth, int depthRepr, int threadCount, int fdReq)
{
    int nonEmptyCount = 0;
    for(int i = 0; i < 40320; ++i)
        if( cubeSets[depth][i].cornerOrientCount() > 0 )
            ++nonEmptyCount;
    std::thread threads[threadCount];
    std::atomic_int idx(-1);
    SearchProgress searchProgress(nonEmptyCount, threadCount);
    for(int t = 0; t < threadCount; ++t) {
        threads[t] = std::thread(searchMovesTb, cubeSets, cubeSetsRepr+depthRepr, &csearch, depth,
                &idx, fdReq, &searchProgress);
    }
    for(int t = 0; t < threadCount; ++t)
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
	static std::vector<CubeSetArray> cubeSets[CSARR_MAX];
    static std::vector<CubeSetReprArray> cubeSetsRepr[CSREPR_MAX];

    fmt_time(); // reset time elapsed
    if( cubeSets[0].empty() ) {
        cubeSets[0].resize(40320);
        unsigned short permSolved = cornersPermToIdx(csolved.cc.getPerms());
        int orientNo = cubeSets[0][permSolved].cornerOrientIdxAddIfAbsent(csolved.cc.getOrients());
        cubeSets[0][permSolved].addCube(orientNo, csolved.ce);
    }
	std::atomic_uint cubeCount(0);
	unsigned short permSearch = cornersPermToIdx(csearch.cc.getPerms());
	for(int depth = 1; depth <= DEPTH_MAX; ++depth) {
        if( cubeSets[depth].empty() )
            addCubes(cubeSets, depth, threadCount, fdReq);
        if( searchMovesA(cubeSets, csearch, depth-1, depth, threadCount, fdReq) )
            return;
        if( searchMovesA(cubeSets, csearch, depth, depth, threadCount, fdReq) )
            return;
    }
    if( cubeSetsRepr[0].empty() )
        addCubesRepr(cubeSets, cubeSetsRepr, threadCount, fdReq);
    if( searchMovesArepr(cubeSets, cubeSetsRepr, csearch, threadCount, fdReq) )
        return;
    if( searchMovesB(cubeSets, cubeSetsRepr, csearch, 1, 0, threadCount, fdReq) )
        return;
    if( CSREPR_MAX > 1 ) {
        if( cubeSetsRepr[1].empty() )
            addCubesRepr2(cubeSets, cubeSetsRepr, threadCount, fdReq);
        if( searchMovesB(cubeSets, cubeSetsRepr, csearch, 1, 1, threadCount, fdReq) )
            return;
    }
    for(int depthRepr = 2; depthRepr < CSREPR_MAX; ++depthRepr) {
        if( cubeSetsRepr[depthRepr].empty() )
            addCubesReprN(cubeSetsRepr, depthRepr, threadCount, fdReq);
        if( searchMovesB(cubeSets, cubeSetsRepr, csearch, 1, depthRepr, threadCount, fdReq) )
            return;
    }
	for(int depth = 2; depth <= DEPTH_MAX; ++depth) {
        if( searchMovesB(cubeSets, cubeSetsRepr, csearch, depth, CSREPR_MAX-1, threadCount, fdReq) )
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

