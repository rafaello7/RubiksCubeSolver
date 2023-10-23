let startTime, lastTime;

function dolog(section, text) {
    let el = document.querySelector(`#${section}`);
    if( section == 'depth' )
        document.querySelector('#progress').textContent = '';
    if( startTime ) {
        let curTime = Date.now();
        if( curTime - lastTime >= 100 ) {
            let dig1 = curTime - lastTime < 10000 ? 1 : 0;
            let dig2 = curTime - startTime < 10000 ? 1 : 0;
            text = `${((curTime-lastTime)/1000).toFixed(dig1)}/${((curTime-startTime)/1000).toFixed(dig2)}s ${text}`;
        }
        if( section == 'depth' )
            lastTime = curTime;
    }
    if( section == 'progress' ) {
        el.textContent = text;
    }else{
        let d = document.createElement('div');
        d.textContent =  text;
        document.querySelector(`#${section}`).append(d);
        d.scrollIntoView();
    }
}

//enum cubecolor {
const CYELLOW = 0;
const CORANGE = 1;
const CBLUE = 2;
const CRED = 3;
const CGREEN = 4;
const CWHITE = 5;
const CCOUNT = 6;

const cubeCornerColors = [
  [ CBLUE,   CORANGE, CYELLOW ], [ CBLUE,  CYELLOW, CRED ],
  [ CBLUE,   CWHITE,  CORANGE ], [ CBLUE,  CRED,    CWHITE ],
  [ CGREEN,  CYELLOW, CORANGE ], [ CGREEN, CRED,    CYELLOW ],
  [ CGREEN,  CORANGE, CWHITE  ], [ CGREEN, CWHITE,  CRED ]
];

const cubeEdgeColors = [
  [ CBLUE,   CYELLOW ],
  [ CORANGE, CBLUE   ], [ CRED,    CBLUE ],
  [ CBLUE,   CWHITE  ],
  [ CYELLOW, CORANGE ], [ CYELLOW, CRED ],
  [ CWHITE,  CORANGE ], [ CWHITE,  CRED ],
  [ CGREEN,  CYELLOW ],
  [ CORANGE, CGREEN  ], [ CRED,    CGREEN ],
  [ CGREEN,  CWHITE ]
];

const R120 = [ 1, 2, 0 ];
const R240 = [ 2, 0, 1 ];

//enum rotate_dir {
const ORANGECW = 0;
const ORANGE180 = 1;
const ORANGECCW = 2;
const REDCW = 3;
const RED180 = 4;
const REDCCW = 5;
const YELLOWCW = 6;
const YELLOW180 = 7;
const YELLOWCCW = 8;
const WHITECW = 9;
const WHITE180 = 10;
const WHITECCW = 11;
const GREENCW = 12;
const GREEN180 = 13;
const GREENCCW = 14;
const BLUECW = 15;
const BLUE180 = 16;
const BLUECCW = 17;
const RCOUNT = 18;

function rotateDirName(rd) {
	switch( rd ) {
	case ORANGECW:  return "Orange \u21b7";
	case ORANGE180: return "Orange 180\u00b0";
	case ORANGECCW: return "Orange \u21b6";
	case REDCW:  return "Red \u21b7";
	case RED180: return "Red 180\u00b0";
	case REDCCW: return "Red \u21b6";
	case YELLOWCW:  return "Yellow \u21b7";
	case YELLOW180: return "Yellow 180\u00b0";
	case YELLOWCCW: return "Yellow \u21b6";
	case WHITECW:  return "White \u21b7";
	case WHITE180: return "White 180\u00b0";
	case WHITECCW: return "White \u21b6";
	case GREENCW:  return "Green \u21b7";
	case GREEN180: return "Green 180\u00b0";
	case GREENCCW: return "Green \u21b6";
	case BLUECW:  return "Blue \u21b7";
	case BLUE180: return "Blue 180\u00b0";
	case BLUECCW: return "Blue \u21b6";
	}
	return '' + rd;
}

function rotateDirReverse(rd) {
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

class cubecorner_perms {
	#perms;

	constructor(corner0perm = 0, corner1perm = 0, corner2perm = 0,
			corner3perm = 0, corner4perm = 0, corner5perm = 0,
			corner6perm = 0, corner7perm = 0)
    {
        if( corner0perm instanceof cubecorner_perms ) {
            this.#perms = corner0perm.#perms;
        }else{
            this.#perms = corner0perm | corner1perm << 3 | corner2perm << 6 |
                corner3perm << 9 | corner4perm << 12 | corner5perm << 15 |
                corner6perm << 18 | corner7perm << 21;
        }
    }

	setAt(idx, perm) {
        this.#perms &= ~(7 << 3*idx);
        this.#perms |= perm << 3*idx;
	}
	getAt(idx) { return this.#perms >>> 3*idx & 7; }
    equals(ccp) {
        return this.#perms == ccp.#perms;
    }
    isLessThan(ccp) {
        return this.#perms < ccp.#perms;
    }
    getPermVal() { return this.#perms; }
}

class cubecorner_orients {
	#orients;

	constructor(corner0orient = 0, corner1orient = 0, corner2orient = 0,
        corner3orient = 0, corner4orient = 0, corner5orient = 0,
        corner6orient = 0, corner7orient = 0)
    {
        if( corner0orient instanceof cubecorner_orients ) {
            this.#orients = corner0orient.#orients;
        }else{
            this.#orients = corner0orient | corner1orient << 2 | corner2orient << 4 |
                corner3orient << 6 | corner4orient << 8 | corner5orient << 10 |
                corner6orient << 12 | corner7orient << 14;
        }
    }

	setAt(/*unsigned*/ idx, /*unsigned char*/ orient) {
		this.#orients &= ~(3 << 2*idx);
		this.#orients |= orient << 2*idx;
	}
	getAt(idx) { return this.#orients >> 2*idx & 3; }
	get() { return this.#orients; }
	set(orts) { this.#orients = orts; }
	equals(cco) { return this.#orients == cco.#orients; }
	isLessThan(cco) { return this.#orients < cco.#orients; }
};

class cubecorners {
    #perms;
    #orients;
	constructor(corner0perm, corner0orient, corner1perm, corner1orient,
			corner2perm, corner2orient, corner3perm, corner3orient,
			corner4perm, corner4orient, corner5perm, corner5orient,
			corner6perm, corner6orient, corner7perm, corner7orient)
    {
        if( corner0orient instanceof cubecorner_orients ) {
            this.#perms = new cubecorner_perms(corner0perm);
            this.#orients = new cubecorner_orients(corner0orient);
        }else{
            this.#perms = new cubecorner_perms(corner0perm, corner1perm, corner2perm,
                corner3perm, corner4perm, corner5perm, corner6perm, corner7perm);
	        this.#orients = new cubecorner_orients(corner0orient, corner1orient, corner2orient,
			corner3orient, corner4orient, corner5orient, corner6orient, corner7orient);
        }
    }

	getPermAt(idx) { return this.#perms.getAt(idx); }
	setPermAt(idx, perm) { this.#perms.setAt(idx, perm); }
	getOrientAt(idx) { return this.#orients.getAt(idx); }
	setOrientAt(idx, orient) { this.#orients.setAt(idx, orient); }
	getPerms() { return this.#perms; }
	getOrients() { return this.#orients; }
	setOrients(orts) { this.#orients.set(orts); }
	equals(cc) {
		return this.#perms.equals(cc.#perms) && this.#orients.equals(cc.#orients);
	}
	isLessThan(cc) {
		return this.#perms.isLessThan(cc.#perms) || this.#perms.equals(cc.#perms) && this.#orients.isLessThan(cc.#orients);
	}
}

class cubeedges {
	#edges;
	constructor(edge0perm = 0, edge0orient = 0, edge1perm = 0, edge1orient = 0,
			edge2perm = 0, edge2orient = 0, edge3perm = 0, edge3orient = 0,
			edge4perm = 0, edge4orient = 0, edge5perm = 0, edge5orient = 0,
			edge6perm = 0, edge6orient = 0, edge7perm = 0, edge7orient = 0,
			edge8perm = 0, edge8orient = 0, edge9perm = 0, edge9orient = 0,
			edge10perm = 0, edge10orient = 0, edge11perm = 0, edge11orient = 0)
    {
        if( edge0perm instanceof cubeedges ) {
            this.#edges = edge0perm.#edges;
        }else{
            this.#edges = BigInt(edge0perm)    | BigInt(edge0orient) << 4n |
                BigInt(edge1perm) << 5n   | BigInt(edge1orient) << 9n |
                BigInt(edge2perm) << 10n  | BigInt(edge2orient) << 14n |
                BigInt(edge3perm) << 15n  | BigInt(edge3orient) << 19n |
                BigInt(edge4perm) << 20n  | BigInt(edge4orient) << 24n |
                BigInt(edge5perm) << 25n  | BigInt(edge5orient) << 29n |
                BigInt(edge6perm) << 30n  | BigInt(edge6orient) << 34n |
                BigInt(edge7perm) << 35n  | BigInt(edge7orient) << 39n |
                BigInt(edge8perm) << 40n  | BigInt(edge8orient) << 44n |
                BigInt(edge9perm) << 45n  | BigInt(edge9orient) << 49n |
                BigInt(edge10perm) << 50n | BigInt(edge10orient) << 54n |
                BigInt(edge11perm) << 55n | BigInt(edge11orient) << 59n;
        }
    }

	set(idx, perm, orient) {
		this.#edges &= ~(0x1Fn << BigInt(5*idx));
		this.#edges |= BigInt(orient<<4 | perm) << BigInt(5*idx);
	}
	setPermAt(idx, perm) {
		this.#edges &= ~(0xFn << BigInt(5*idx));
		this.#edges |= BigInt(perm) << BigInt(5*idx);
	}
	setOrientAt(idx, orient) {
		this.#edges &= ~(0x1n << BigInt(5*idx+4));
		this.#edges |= BigInt(orient) << BigInt(5*idx+4);
	}
    static compose(ce1, ce2) {
        let res = new cubeedges();
        for(let i = 0n; i < 12n; ++i) {
            let edge2perm = ce2.#edges >> 5n * i & 0xfn;
            let edge2orient = ce2.#edges >> 5n * i & 0x10n;
            let edge1item = ce1.#edges >> 5n * edge2perm & 0x1fn;
            res.#edges |= (edge1item ^ edge2orient) << 5n*i;
        }
        return res;
    }

    static transform(ce, idx) {
        let cetrans = ctransformed[idx].ce;
        let cetransRev = ctransformed[transformReverse(idx)].ce;
        let res = new cubeedges();
        for(let i = 0n; i < 12n; ++i) {
            let ctrRevItem = cetransRev.#edges >> 5n * i;
            let ctrRevPerm = ctrRevItem & 0xfn;
            let ceItem = ce.#edges >> 5n * ctrRevPerm;
            let cePerm = ceItem & 0xfn;
            let ctransItem = cetrans.#edges >> 5n * cePerm & 0x1fn;
            let orient = (ceItem ^ ctrRevItem) & 0x10n;
            res.#edges |= (ctransItem ^ orient) << 5n*i;
        }
        return res;
    }

	edgeN(idx) { return Number(this.#edges >> BigInt(5 * idx) & 0xfn); }
	edgeR(idx) { return Number(this.#edges >> BigInt(5*idx+4) & 1n); }
	equals(ce) { return this.#edges == ce.#edges; }
	isLessThan(ce) { return this.#edges < ce.#edges; }
    get() { return this.#edges; }
};

class cube {
	/*cubecorners*/ cc;
	/*cubeedges*/ ce;

    constructor(cc, ce) {
        if( cc == undefined )
            cc = new cubecorners();
        if( ce == undefined )
            ce = new cubeedges();
        this.cc = cc;
        this.ce = ce;
    }

    equals(c) { 
        return this.cc.equals(c.cc) && this.ce.equals(c.ce);
    }

    isLessThan(c) { 
        return this.cc.isLessThan(c.cc) || this.cc.equals(c.cc) && this.ce.isLessThan(c.ce);
    }
};


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
// |         | c  e  c |         | c  e  c |
// | e     e |         | e     e |         |
// |         | c  e  c |         | c  e  c |
// |_________|_________|_________|_________|
//           |  WHITE  |
//           |         |
//           | e     e |
//           |         |
//           |_________|

const csolved = new cube(new cubecorners(
		0, 0,  1, 0,
		2, 0,  3, 0,
		4, 0,  5, 0,
		6, 0,  7, 0
	),
    new cubeedges(
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
);

function isCubeSolvable(c)
{
    let isSwapsOdd = false;
    let permsScanned = 0;
    for(let i = 0; i < 8; ++i) {
        if( permsScanned & 1 << i )
            continue;
        permsScanned |= 1 << i;
        let p = i;
        while( (p = c.cc.getPermAt(p)) != i ) {
            if( permsScanned & 1 << p ) {
                dolog('err', `corner perm ${p} is twice\n`);
                return false;
            }
            permsScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
    permsScanned = 0;
    for(let i = 0; i < 12; ++i) {
        if( permsScanned & 1 << i )
            continue;
        permsScanned |= 1 << i;
        let p = i;
        while( (p = c.ce.edgeN(p)) != i ) {
            if( permsScanned & 1 << p ) {
                dolog('err', `edge perm ${p} is twice\n`);
                return false;
            }
            permsScanned |= 1 << p;
            isSwapsOdd = !isSwapsOdd;
        }
    }
	if( isSwapsOdd ) {
		dolog('err', "cube unsolvable due to permutation parity\n");
		return false;
	}
	let sumOrient = 0;
	for(let i = 0; i < 8; ++i)
		sumOrient += c.cc.getOrientAt(i);
	if( sumOrient % 3 ) {
		dolog('err', "cube unsolvable due to corner orientations\n");
		return false;
	}
	sumOrient = 0;
	for(let i = 0; i < 12; ++i)
		sumOrient += c.ce.edgeR(i);
	if( sumOrient % 2 ) {
		dolog('err', "cube unsolvable due to edge orientations\n");
		return false;
	}
    return true;
}

/*cubecorner_perms*/ function cornersIdxToPerm(idx) {
	let unused = 0x76543210;
	let ccp = new cubecorner_perms();

	for(let cornerIdx = 8; cornerIdx > 0; --cornerIdx) {
		let p = idx % cornerIdx * 4;
		ccp.setAt(8-cornerIdx, unused >> p & 0xf);
		let m = (1 << p) - 1;
		unused = unused & m | unused >> 4 & ~m;
		idx = Math.floor(idx/cornerIdx);
	}
	return ccp;
}

/*
struct elemLoc {
	int wall;
    int row;
	int col;
};
*/

function cubeFromColorsOnWalls(text)
{
	const /*elemLoc*/ cornerLocMap = [
		[ { wall: 2, row: 0, col: 0 }, { wall: 1, row: 0, col: 2 }, { wall: 0, row: 2, col: 0 } ],
		[ { wall: 2, row: 0, col: 2 }, { wall: 0, row: 2, col: 2 }, { wall: 3, row: 0, col: 0 } ],

		[ { wall: 2, row: 2, col: 0 }, { wall: 5, row: 0, col: 0 }, { wall: 1, row: 2, col: 2 } ],
		[ { wall: 2, row: 2, col: 2 }, { wall: 3, row: 2, col: 0 }, { wall: 5, row: 0, col: 2 } ],

		[ { wall: 4, row: 0, col: 2 }, { wall: 0, row: 0, col: 0 }, { wall: 1, row: 0, col: 0 } ],
		[ { wall: 4, row: 0, col: 0 }, { wall: 3, row: 0, col: 2 }, { wall: 0, row: 0, col: 2 } ],

		[ { wall: 4, row: 2, col: 2 }, { wall: 1, row: 2, col: 0 }, { wall: 5, row: 2, col: 0 } ],
		[ { wall: 4, row: 2, col: 0 }, { wall: 5, row: 2, col: 2 }, { wall: 3, row: 2, col: 2 } ],
	];
	const /*elemLoc*/ edgeLocMap = [
		[ { wall: 2, row: 0, col: 1 }, { wall: 0, row: 2, col: 1 } ],
		[ { wall: 1, row: 1, col: 2 }, { wall: 2, row: 1, col: 0 } ],
		[ { wall: 3, row: 1, col: 0 }, { wall: 2, row: 1, col: 2 } ],
		[ { wall: 2, row: 2, col: 1 }, { wall: 5, row: 0, col: 1 } ],

		[ { wall: 0, row: 1, col: 0 }, { wall: 1, row: 0, col: 1 } ],
		[ { wall: 0, row: 1, col: 2 }, { wall: 3, row: 0, col: 1 } ],
		[ { wall: 5, row: 1, col: 0 }, { wall: 1, row: 2, col: 1 } ],
		[ { wall: 5, row: 1, col: 2 }, { wall: 3, row: 2, col: 1 } ],

		[ { wall: 4, row: 0, col: 1 }, { wall: 0, row: 0, col: 1 } ],
		[ { wall: 1, row: 1, col: 0 }, { wall: 4, row: 1, col: 2 } ],
		[ { wall: 3, row: 1, col: 2 }, { wall: 4, row: 1, col: 0 } ],
		[ { wall: 4, row: 2, col: 1 }, { wall: 5, row: 2, col: 1 } ],
	];
	const colorLetters = "YOBRGW";
	let walls = [ [[], [], []], [[], [], []], [[], [], []], [[], [], []], [[], [], []], [[], [], []] ]; //[6][3][3];

    if( text.includes('\n') ) {
        let wno = 0, rno = 0;
        let textlines = text.split('\n');
        let lineNo = 0;
        while( lineNo < textlines.length ) {
            let line = textlines[lineNo];
            ++lineNo;
            let sqcount = 0, cno = 0;
            while( sqcount < (wno == 1 ? 12 : 3) ) {
                if( cno == line.length ) {
                    dolog('err', `too few letters at line ${lineNo}\n`);
                    return null;
                }
                if( ! /\s/.test(line[cno]) ) {
                    let lpos = colorLetters.indexOf(line[cno].toUpperCase());
                    if(lpos < 0) {
                        dolog('err', `bad letter at line ${lineNo} column ${cno}\nwno=${wno} rno=${rno}\n`);
                        return null;
                    }
                    walls[wno + Math.floor(sqcount / 3)][rno][sqcount % 3] = lpos;
                    ++sqcount;
                }
                ++cno;
            }
            if( rno == 2 ) {
                if( wno == 5 )
                    break;
                wno = wno == 0 ? 1 : 5;
                rno = 0;
            }else
                ++rno;
        }
        if( lineNo < 9 ) {
            dolog('err', "not enough values\n");
            return null;
        }
    }else{
        if( text.length < 54 ) {
            dolog('err', `to few letters\n`);
            return null;
        }
        for(let cno = 0; cno < 54; ++cno ) {
            let lpos = colorLetters.indexOf(text[cno].toUpperCase());
            if(lpos < 0) {
                dolog('err', `bad letter at column ${cno}\n`);
                return null;
            }
            walls[Math.floor(cno/9)][Math.floor(cno/3) % 3][cno%3] = lpos;
        }
    }
	for(let i = 0; i < 6; ++i) {
		if( walls[i][1][1] != i ) {
			dolog('err', `bad orientation: wall=${i} exp=${colorLetters[i]} is=${colorLetters[walls[i][1][1]]}\n`);
			return null;
		}
	}
	let c = new cube();
	for(let i = 0; i < 8; ++i) {
		let match = false;
		for(let n = 0; n < 8; ++n) {
			const elemColors = cubeCornerColors[n];
			match = true;
			for(let r = 0; r < 3 && match; ++r) {
				const /*elemLoc*/ el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[r] )
					match = false;
			}
			if( match ) {
				c.cc.setPermAt(i, n);
				c.cc.setOrientAt(i, 0);
				break;
			}
			match = true;
			for(let r = 0; r < 3 && match; ++r) {
				const /*elemLoc*/ el = cornerLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[R120[r]] )
					match = false;
			}
			if( match ) {
				c.cc.setPermAt(i, n);
				c.cc.setOrientAt(i, 1);
				break;
			}
			match = true;
			for(let r = 0; r < 3 && match; ++r) {
				const el = cornerLocMap[i][r];
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
			dolog('err', `corner ${i} not found\n`);
			return null;
		}
		for(let j = 0; j < i; ++j) {
			if( c.cc.getPermAt(i) == c.cc.getPermAt(j) ) {
				dolog('err', `corner ${c.cc.getPermAt(i)} is twice: at ${j} and ${i}\n`);
				return null;
			}
		}
	}
	for(let i = 0; i < 12; ++i) {
		let match = false;
		for(let n = 0; n < 12; ++n) {
			const elemColors = cubeEdgeColors[n];
			match = true;
			for(let r = 0; r < 2 && match; ++r) {
				const el = edgeLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[r] )
					match = false;
			}
			if( match ) {
				c.ce.set(i, n, 0);
				break;
			}
			match = true;
			for(let r = 0; r < 2 && match; ++r) {
				const el = edgeLocMap[i][r];
				if( walls[el.wall][el.row][el.col] != elemColors[1-r] )
					match = false;
			}
			if( match ) {
				c.ce.set(i, n, 1);
				break;
			}
		}
		if( ! match ) {
			dolog('err', `edge ${i} not found\n`);
			return null;
		}
		for(let j = 0; j < i; ++j) {
			if( c.ce.edgeN(i) == c.ce.edgeN(j) ) {
				dolog('err', `edge ${c.ce.edgeN(i)} is twice: at ${j} and ${i}\n`);
				return null;
			}
		}
	}
    for(let i = 0; i < 12; ++i)
    if( ! isCubeSolvable(c) )
        return null;
	return c;
}

function cubeToParamText(c)
{
	const colorChars = 'YOBRGW';
    let res = '';

	res += colorChars[cubeCornerColors[c.cc.getPermAt(4)][R120[c.cc.getOrientAt(4)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(8)][1-c.ce.edgeR(8)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(5)][R240[c.cc.getOrientAt(5)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(4)][c.ce.edgeR(4)]];
    res += 'Y';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(5)][c.ce.edgeR(5)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(0)][R240[c.cc.getOrientAt(0)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(0)][1-c.ce.edgeR(0)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(1)][R120[c.cc.getOrientAt(1)]]];

    res += colorChars[cubeCornerColors[c.cc.getPermAt(4)][R240[c.cc.getOrientAt(4)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(4)][1-c.ce.edgeR(4)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(0)][R120[c.cc.getOrientAt(0)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(9)][c.ce.edgeR(9)]];
    res += 'O';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(1)][c.ce.edgeR(1)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(6)][R120[c.cc.getOrientAt(6)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(6)][1-c.ce.edgeR(6)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(2)][R240[c.cc.getOrientAt(2)]]];

    res += colorChars[cubeCornerColors[c.cc.getPermAt(0)][c.cc.getOrientAt(0)]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(0)][c.ce.edgeR(0)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(1)][c.cc.getOrientAt(1)]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(1)][1-c.ce.edgeR(1)]];
    res += 'B';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(2)][1-c.ce.edgeR(2)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(2)][c.cc.getOrientAt(2)]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(3)][c.ce.edgeR(3)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(3)][c.cc.getOrientAt(3)]];

    res += colorChars[cubeCornerColors[c.cc.getPermAt(1)][R240[c.cc.getOrientAt(1)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(5)][1-c.ce.edgeR(5)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(5)][R120[c.cc.getOrientAt(5)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(2)][c.ce.edgeR(2)]];
    res += 'R';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(10)][c.ce.edgeR(10)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(3)][R120[c.cc.getOrientAt(3)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(7)][1-c.ce.edgeR(7)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(7)][R240[c.cc.getOrientAt(7)]]];

    res += colorChars[cubeCornerColors[c.cc.getPermAt(5)][c.cc.getOrientAt(5)]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(8)][c.ce.edgeR(8)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(4)][c.cc.getOrientAt(4)]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(10)][1-c.ce.edgeR(10)]];
    res += 'G';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(9)][1-c.ce.edgeR(9)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(7)][c.cc.getOrientAt(7)]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(11)][c.ce.edgeR(11)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(6)][c.cc.getOrientAt(6)]];

    res += colorChars[cubeCornerColors[c.cc.getPermAt(2)][R120[c.cc.getOrientAt(2)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(3)][1-c.ce.edgeR(3)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(3)][R240[c.cc.getOrientAt(3)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(6)][c.ce.edgeR(6)]];
    res += 'W';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(7)][c.ce.edgeR(7)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(6)][R240[c.cc.getOrientAt(6)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(11)][1-c.ce.edgeR(11)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(7)][R120[c.cc.getOrientAt(7)]]];
    return res;
}

function cubeToSaveText(c)
{
	const colorChars = 'YOBRGW';
    let res = '    ';

	res += colorChars[cubeCornerColors[c.cc.getPermAt(4)][R120[c.cc.getOrientAt(4)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(8)][1-c.ce.edgeR(8)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(5)][R240[c.cc.getOrientAt(5)]]];
    res += '\n    ';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(4)][c.ce.edgeR(4)]];
    res += 'Y';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(5)][c.ce.edgeR(5)]];
    res += '\n    ';
    res += colorChars[cubeCornerColors[c.cc.getPermAt(0)][R240[c.cc.getOrientAt(0)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(0)][1-c.ce.edgeR(0)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(1)][R120[c.cc.getOrientAt(1)]]];
    res += '\n';

    res += colorChars[cubeCornerColors[c.cc.getPermAt(4)][R240[c.cc.getOrientAt(4)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(4)][1-c.ce.edgeR(4)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(0)][R120[c.cc.getOrientAt(0)]]];
    res += ' ';
    res += colorChars[cubeCornerColors[c.cc.getPermAt(0)][c.cc.getOrientAt(0)]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(0)][c.ce.edgeR(0)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(1)][c.cc.getOrientAt(1)]];
    res += ' ';
    res += colorChars[cubeCornerColors[c.cc.getPermAt(1)][R240[c.cc.getOrientAt(1)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(5)][1-c.ce.edgeR(5)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(5)][R120[c.cc.getOrientAt(5)]]];
    res += ' ';
    res += colorChars[cubeCornerColors[c.cc.getPermAt(5)][c.cc.getOrientAt(5)]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(8)][c.ce.edgeR(8)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(4)][c.cc.getOrientAt(4)]];
    res += '\n';

    res += colorChars[cubeEdgeColors[c.ce.edgeN(9)][c.ce.edgeR(9)]];
    res += 'O';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(1)][c.ce.edgeR(1)]];
    res += ' ';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(1)][1-c.ce.edgeR(1)]];
    res += 'B';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(2)][1-c.ce.edgeR(2)]];
    res += ' ';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(2)][c.ce.edgeR(2)]];
    res += 'R';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(10)][c.ce.edgeR(10)]];
    res += ' ';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(10)][1-c.ce.edgeR(10)]];
    res += 'G';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(9)][1-c.ce.edgeR(9)]];
    res += '\n';

    res += colorChars[cubeCornerColors[c.cc.getPermAt(6)][R120[c.cc.getOrientAt(6)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(6)][1-c.ce.edgeR(6)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(2)][R240[c.cc.getOrientAt(2)]]];
    res += ' ';
    res += colorChars[cubeCornerColors[c.cc.getPermAt(2)][c.cc.getOrientAt(2)]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(3)][c.ce.edgeR(3)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(3)][c.cc.getOrientAt(3)]];
    res += ' ';
    res += colorChars[cubeCornerColors[c.cc.getPermAt(3)][R120[c.cc.getOrientAt(3)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(7)][1-c.ce.edgeR(7)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(7)][R240[c.cc.getOrientAt(7)]]];
    res += ' ';
    res += colorChars[cubeCornerColors[c.cc.getPermAt(7)][c.cc.getOrientAt(7)]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(11)][c.ce.edgeR(11)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(6)][c.cc.getOrientAt(6)]];
    res += '\n    ';

    res += colorChars[cubeCornerColors[c.cc.getPermAt(2)][R120[c.cc.getOrientAt(2)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(3)][1-c.ce.edgeR(3)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(3)][R240[c.cc.getOrientAt(3)]]];
    res += '\n    ';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(6)][c.ce.edgeR(6)]];
    res += 'W';
    res += colorChars[cubeEdgeColors[c.ce.edgeN(7)][c.ce.edgeR(7)]];
    res += '\n    ';
    res += colorChars[cubeCornerColors[c.cc.getPermAt(6)][R240[c.cc.getOrientAt(6)]]];
    res += colorChars[cubeEdgeColors[c.ce.edgeN(11)][1-c.ce.edgeR(11)]];
    res += colorChars[cubeCornerColors[c.cc.getPermAt(7)][R120[c.cc.getOrientAt(7)]]];
    res += '\n';
    return res;
}

async function searchMoves(c) {
    try {
        startTime = lastTime = Date.now();
        moves = [];
        moveidx = moveidxgoal = -1;
        document.querySelectorAll(`#movebuttons > button`).forEach( (btn) => { btn.classList.remove('currentmv'); });
        movebuttonini.classList.add('currentmv');
        document.querySelectorAll('.movebtn').forEach(function(el) { el.textContent = ''; });

        let qparam=cubeToParamText(c);
        let a = await fetch('/?' + qparam);
        let b = a.body.getReader();
        const utf8Decoder = new TextDecoder("utf-8");
        let msg = '';
        while(true) {
            let c = await b.read();
            if( c.done )
                break;
            msg += utf8Decoder.decode(c.value);
            let lineEnd;
            while( (lineEnd = msg.indexOf('\n')) > 0 ) {
                let line = msg.substring(0, lineEnd);
                msg = msg.substring(lineEnd+1);
                if( line.startsWith("setup: ") ) {
                    dolog('err', `${line.substring(7)}\n`);
                }else if( line.startsWith("solution: ") ) {
                    dolog('solutions', `${line}\n`);
                    if( moves.length == 0 ) {
                        let movesStr = line.split(' ');
                        let mvs = [];
                        for(let i = 1; i < movesStr.length; ++i) {
                            switch( movesStr[i] ) {
                                case "orange-cw": mvs.push(ORANGECW); break;
                                case "orange-180": mvs.push(ORANGE180); break;
                                case "orange-ccw": mvs.push(ORANGECCW); break;
                                case "red-cw": mvs.push(REDCW); break;
                                case "red-180": mvs.push(RED180); break;
                                case "red-ccw": mvs.push(REDCCW); break;
                                case "yellow-cw": mvs.push(YELLOWCW); break;
                                case "yellow-180": mvs.push(YELLOW180); break;
                                case "yellow-ccw": mvs.push(YELLOWCCW); break;
                                case "white-cw": mvs.push(WHITECW); break;
                                case "white-180": mvs.push(WHITE180); break;
                                case "white-ccw": mvs.push(WHITECCW); break;
                                case "green-cw": mvs.push(GREENCW); break;
                                case "green-180": mvs.push(GREEN180); break;
                                case "green-ccw": mvs.push(GREENCCW); break;
                                case "blue-cw": mvs.push(BLUECW); break;
                                case "blue-180": mvs.push(BLUE180); break;
                                case "blue-ccw": mvs.push(BLUECCW); break;
                            }
                        }
                        moves = mvs;
                        for(let i = 0; i < moves.length; ++i) {
                            let btn = document.querySelector(`#movebutton${i}`);
                            btn.textContent = rotateDirName(moves[i]);
                        }
                        moveidxgoal = moves.length - 1;
                        rewind = 10;
                    }
                }else if( line.startsWith("progress: ") )
                    dolog('progress', `${line.substring(10)}\n`);
                else
                    dolog('depth', `${line}\n`);
            }
        }
    }catch(e) {
        dolog('depth', e.message);
    }
    document.querySelectorAll('.manipmodeonly,.changingcube').forEach((e) => {e.disabled = false;});
    startTime = undefined;
}

class TransformMatrix {
    #x;
    #y;
    #z;
    constructor(x1, x2, x3, y1, y2, y3, z1, z2, z3) {
        this.#x = [x1, x2, x3];
        this.#y = [y1, y2, y3];
        this.#z = [z1, z2, z3];
    }
    static multiply(a, b) {
        return new TransformMatrix(
            a.#x[0] * b.#x[0] + a.#x[1] * b.#y[0] + a.#x[2] * b.#z[0],
            a.#x[0] * b.#x[1] + a.#x[1] * b.#y[1] + a.#x[2] * b.#z[1],
            a.#x[0] * b.#x[2] + a.#x[1] * b.#y[2] + a.#x[2] * b.#z[2],
            a.#y[0] * b.#x[0] + a.#y[1] * b.#y[0] + a.#y[2] * b.#z[0],
            a.#y[0] * b.#x[1] + a.#y[1] * b.#y[1] + a.#y[2] * b.#z[1],
            a.#y[0] * b.#x[2] + a.#y[1] * b.#y[2] + a.#y[2] * b.#z[2],
            a.#z[0] * b.#x[0] + a.#z[1] * b.#y[0] + a.#z[2] * b.#z[0],
            a.#z[0] * b.#x[1] + a.#z[1] * b.#y[1] + a.#z[2] * b.#z[1],
            a.#z[0] * b.#x[2] + a.#z[1] * b.#y[2] + a.#z[2] * b.#z[2]);
    }

    static multiplyRnd(a, b) {
        return new TransformMatrix(
            Math.round(a.#x[0] * b.#x[0] + a.#x[1] * b.#y[0] + a.#x[2] * b.#z[0]),
            Math.round(a.#x[0] * b.#x[1] + a.#x[1] * b.#y[1] + a.#x[2] * b.#z[1]),
            Math.round(a.#x[0] * b.#x[2] + a.#x[1] * b.#y[2] + a.#x[2] * b.#z[2]),
            Math.round(a.#y[0] * b.#x[0] + a.#y[1] * b.#y[0] + a.#y[2] * b.#z[0]),
            Math.round(a.#y[0] * b.#x[1] + a.#y[1] * b.#y[1] + a.#y[2] * b.#z[1]),
            Math.round(a.#y[0] * b.#x[2] + a.#y[1] * b.#y[2] + a.#y[2] * b.#z[2]),
            Math.round(a.#z[0] * b.#x[0] + a.#z[1] * b.#y[0] + a.#z[2] * b.#z[0]),
            Math.round(a.#z[0] * b.#x[1] + a.#z[1] * b.#y[1] + a.#z[2] * b.#z[1]),
            Math.round(a.#z[0] * b.#x[2] + a.#z[1] * b.#y[2] + a.#z[2] * b.#z[2]));
    }

    multiplyWith(b) { return TransformMatrix.multiply(this, b); }
    multiplyRndWith(b) { return TransformMatrix.multiplyRnd(this, b); }
    equals(b) {
        return this.#x[0] == b.#x[0] && this.#x[1] == b.#x[1] && this.#x[2] == b.#x[2] && 
            this.#y[0] == b.#y[0] && this.#y[1] == b.#y[1] && this.#y[2] == b.#y[2] && 
            this.#z[0] == b.#z[0] && this.#z[1] == b.#z[1] && this.#z[2] == b.#z[2];
    }

    isIn(searchArr) {
        for(let item of searchArr) {
            if( item.equals(this) )
                return true;
        }
        return false;
    }

    toMatrix3dParam() {
        return this.#x.join() + ',0,' + this.#y.join() + ',0,' +
            this.#z.join() + ',0,0,0,0,1';
    }
}
 
let cubeimgmx = new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1);

// Maps corner permutation + orientation to a transformation matrix
let cornersmx = [
    [
        new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1),
        new TransformMatrix(0, 0, -1,  1, 0, 0,  0, -1, 0),
        new TransformMatrix(0, 1, 0,  0, 0, -1,  -1, 0, 0),
    ],[
        new TransformMatrix(0, 1, 0,  -1, 0, 0,  0, 0, 1),
        new TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0),
        new TransformMatrix(-1, 0, 0,  0, 0, -1,  0, -1, 0),
    ],[
        new TransformMatrix(0, -1, 0,  1, 0, 0,  0, 0, 1),
        new TransformMatrix(0, 0, -1,  0, -1, 0,  -1, 0, 0),
        new TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0),
    ],[
        new TransformMatrix(-1, 0, 0,  0, -1, 0,  0, 0, 1),
        new TransformMatrix(0, 0, -1,  -1, 0, 0,  0, 1, 0),
        new TransformMatrix(0, -1, 0,  0, 0, -1,  1, 0, 0),
    ],[
        new TransformMatrix(0, 1, 0,  1, 0, 0,  0, 0, -1),
        new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0),
        new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0),
    ],[
        new TransformMatrix(-1, 0, 0,  0, 1, 0,  0, 0, -1),
        new TransformMatrix(0, 0, 1,  -1, 0, 0,  0, -1, 0),
        new TransformMatrix(0, 1, 0,  0, 0, 1,  1, 0, 0),
    ],[
        new TransformMatrix(1, 0, 0,  0, -1, 0,  0, 0, -1),
        new TransformMatrix(0, 0, 1,  1, 0, 0,  0, 1, 0),
        new TransformMatrix(0, -1, 0,  0, 0, 1,  -1, 0, 0),
    ],[
        new TransformMatrix(0, -1, 0,  -1, 0, 0,  0, 0, -1),
        new TransformMatrix(0, 0, 1,  0, -1, 0,  1, 0, 0),
        new TransformMatrix(-1, 0, 0,  0, 0, 1,  0, 1, 0),
    ]
];

// Maps edge permutation + orientation to a transformation matrix
let edgesmx = [
    [
        new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1),
        new TransformMatrix(-1, 0, 0,  0, 0, -1,  0, -1, 0),
    ],[
        new TransformMatrix(0, 1, 0,  0, 0, -1,  -1, 0, 0),
        new TransformMatrix(0, -1, 0,  1, 0, 0,  0, 0, 1),
    ],[
        new TransformMatrix(0, -1, 0,  0, 0, -1,  1, 0, 0),
        new TransformMatrix(0, 1, 0,  -1, 0, 0,  0, 0, 1),
    ],[
        new TransformMatrix(-1, 0, 0,  0, -1, 0,  0, 0, 1),
        new TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0),
    ],[
        new TransformMatrix(0, 0, -1,  1, 0, 0,  0, -1, 0),
        new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0),
    ],[
        new TransformMatrix(0, 0, 1,  -1, 0, 0,  0, -1, 0),
        new TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0),
    ],[
        new TransformMatrix(0, 0, 1,  1, 0, 0,  0, 1, 0),
        new TransformMatrix(0, 0, -1,  0, -1, 0,  -1, 0, 0),
    ],[
        new TransformMatrix(0, 0, -1,  -1, 0, 0,  0, 1, 0),
        new TransformMatrix(0, 0, 1,  0, -1, 0,  1, 0, 0),
    ],[
        new TransformMatrix(-1, 0, 0,  0, 1, 0,  0, 0, -1),
        new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0),
    ],[
        new TransformMatrix(0, -1, 0,  0, 0, 1,  -1, 0, 0),
        new TransformMatrix(0, 1, 0,  1, 0, 0,  0, 0, -1),
    ],[
        new TransformMatrix(0, 1, 0,  0, 0, 1,  1, 0, 0),
        new TransformMatrix(0, -1, 0,  -1, 0, 0,  0, 0, -1),
    ],[
        new TransformMatrix(1, 0, 0,  0, -1, 0,  0, 0, -1),
        new TransformMatrix(-1, 0, 0,  0, 0, 1,  0, 1, 0),
    ]
];

// possible matrix values for corners on blue wall
let cbluemx = [
    new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1),
    new TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0),
    new TransformMatrix(-1, 0, 0,  0, -1, 0,  0, 0, 1),
    new TransformMatrix(-1, 0, 0,  0, 0, -1,  0, -1, 0),

    new TransformMatrix(0, 1, 0,  -1, 0, 0,  0, 0, 1),
    new TransformMatrix(0, 1, 0,  0, 0, -1,  -1, 0, 0),
    new TransformMatrix(0, -1, 0,  1, 0, 0,  0, 0, 1),
    new TransformMatrix(0, -1, 0,  0, 0, -1,  1, 0, 0),

    new TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0),
    new TransformMatrix(0, 0, -1,  1, 0, 0,  0, -1, 0),
    new TransformMatrix(0, 0, -1,  0, -1, 0,  -1, 0, 0),
    new TransformMatrix(0, 0, -1,  -1, 0, 0,  0, 1, 0),
];

// possible matrix values for edges on blue wall
let ebluemx = [
    new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1),
    new TransformMatrix(-1, 0, 0,  0, 0, -1,  0, -1, 0),

    new TransformMatrix(0, 1, 0,  -1, 0, 0,  0, 0, 1),
    new TransformMatrix(0, -1, 0,  0, 0, -1,  1, 0, 0),

    new TransformMatrix(-1, 0, 0,  0, -1, 0,  0, 0, 1),
    new TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0),

    new TransformMatrix(0, -1, 0,  1, 0, 0,  0, 0, 1),
    new TransformMatrix(0, 1, 0,  0, 0, -1,  -1, 0, 0)
];

// possible matrix values for corners on red wall
let credmx = [
    new TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0),
    new TransformMatrix(0, 1, 0,  0, 0, 1,  1, 0, 0),
    new TransformMatrix(0, 0, 1,  0, -1, 0,  1, 0, 0),
    new TransformMatrix(0, -1, 0,  0, 0, -1,  1, 0, 0),
    new TransformMatrix(-1, 0, 0,  0, 1, 0,  0, 0, -1),
    new TransformMatrix(-1, 0, 0,  0, 0, 1,  0, 1, 0),
    new TransformMatrix(-1, 0, 0,  0, -1, 0,  0, 0, 1),
    new TransformMatrix(-1, 0, 0,  0, 0, -1,  0, -1, 0),
    new TransformMatrix(0, 0, 1,  -1, 0, 0,  0, -1, 0),
    new TransformMatrix(0, -1, 0,  -1, 0, 0,  0, 0, -1),
    new TransformMatrix(0, 0, -1,  -1, 0, 0,  0, 1, 0),
    new TransformMatrix(0, 1, 0,  -1, 0, 0,  0, 0, 1),
];

// possible matrix values for edges on red wall
let eredmx = [
    new TransformMatrix(0, 0, 1,  -1, 0, 0,  0, -1, 0),
    new TransformMatrix(0, -1, 0,  -1, 0, 0,  0, 0, -1),
    new TransformMatrix(0, 0, -1,  -1, 0, 0,  0, 1, 0),
    new TransformMatrix(0, 1, 0,  -1, 0, 0,  0, 0, 1),
    new TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0),
    new TransformMatrix(0, 1, 0,  0, 0, 1,  1, 0, 0),
    new TransformMatrix(0, 0, 1,  0, -1, 0,  1, 0, 0),
    new TransformMatrix(0, -1, 0,  0, 0, -1,  1, 0, 0),
];

// possible matrix values for corners on yellow wall
let cyellowmx = [
    new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0),
    new TransformMatrix(0, 1, 0,  1, 0, 0,  0, 0, -1),
    new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0),
    new TransformMatrix(0, 0, 1,  -1, 0, 0,  0, -1, 0),
    new TransformMatrix(0, 1, 0,  0, 0, 1,  1, 0, 0),
    new TransformMatrix(-1, 0, 0,  0, 1, 0,  0, 0, -1),
    new TransformMatrix(0, 0, -1,  1, 0, 0,  0, -1, 0),
    new TransformMatrix(0, 1, 0,  0, 0, -1,  -1, 0, 0),
    new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1),
    new TransformMatrix(-1, 0, 0,  0, 0, -1,  0, -1, 0),
    new TransformMatrix(0, 1, 0,  -1, 0, 0,  0, 0, 1),
    new TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0),
];

// possible matrix values for edges on yellow wall
let eyellowmx = [
    new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0),
    new TransformMatrix(-1, 0, 0,  0, 1, 0,  0, 0, -1),
    new TransformMatrix(0, 0, 1,  -1, 0, 0,  0, -1, 0),
    new TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0),
    new TransformMatrix(-1, 0, 0,  0, 0, -1,  0, -1, 0),
    new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1),
    new TransformMatrix(0, 0, -1,  1, 0, 0,  0, -1, 0),
    new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0),
];

// possible matrix values for corners on green wall
let cgreenmx = [
    new TransformMatrix(-1, 0, 0,  0, 1, 0,  0, 0, -1),
    new TransformMatrix(0, 0, 1,  -1, 0, 0,  0, -1, 0),
    new TransformMatrix(0, 1, 0,  0, 0, 1,  1, 0, 0),
    new TransformMatrix(0, 1, 0,  1, 0, 0,  0, 0, -1),
    new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0),
    new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0),
    new TransformMatrix(0, -1, 0,  -1, 0, 0,  0, 0, -1),
    new TransformMatrix(0, 0, 1,  0, -1, 0,  1, 0, 0),
    new TransformMatrix(-1, 0, 0,  0, 0, 1,  0, 1, 0),
    new TransformMatrix(1, 0, 0,  0, -1, 0,  0, 0, -1),
    new TransformMatrix(0, 0, 1,  1, 0, 0,  0, 1, 0),
    new TransformMatrix(0, -1, 0,  0, 0, 1,  -1, 0, 0),
];

// possible matrix values for edges on green wall
let egreenmx = [
    new TransformMatrix(-1, 0, 0,  0, 1, 0,  0, 0, -1),
    new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0),
    new TransformMatrix(0, 1, 0,  1, 0, 0,  0, 0, -1),
    new TransformMatrix(0, -1, 0,  0, 0, 1,  -1, 0, 0),
    new TransformMatrix(1, 0, 0,  0, -1, 0,  0, 0, -1),
    new TransformMatrix(-1, 0, 0,  0, 0, 1,  0, 1, 0),
    new TransformMatrix(0, -1, 0,  -1, 0, 0,  0, 0, -1),
    new TransformMatrix(0, 1, 0,  0, 0, 1,  1, 0, 0),
];

// possible matrix values for corners on white wall
let cwhitemx = [
    new TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0),
    new TransformMatrix(0, -1, 0,  1, 0, 0,  0, 0, 1),
    new TransformMatrix(0, 0, -1,  0, -1, 0,  -1, 0, 0),
    new TransformMatrix(0, 0, -1,  -1, 0, 0,  0, 1, 0),
    new TransformMatrix(0, -1, 0,  0, 0, -1,  1, 0, 0),
    new TransformMatrix(-1, 0, 0,  0, -1, 0,  0, 0, 1),
    new TransformMatrix(0, 0, 1,  1, 0, 0,  0, 1, 0),
    new TransformMatrix(0, -1, 0,  0, 0, 1,  -1, 0, 0),
    new TransformMatrix(1, 0, 0,  0, -1, 0,  0, 0, -1),
    new TransformMatrix(-1, 0, 0,  0, 0, 1,  0, 1, 0),
    new TransformMatrix(0, -1, 0,  -1, 0, 0,  0, 0, -1),
    new TransformMatrix(0, 0, 1,  0, -1, 0,  1, 0, 0),
];

// possible matrix values for edges on white wall
let ewhitemx = [
    new TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0),
    new TransformMatrix(-1, 0, 0,  0, -1, 0,  0, 0, 1),
    new TransformMatrix(0, 0, -1,  -1, 0, 0,  0, 1, 0),
    new TransformMatrix(0, 0, 1,  0, -1, 0,  1, 0, 0),
    new TransformMatrix(-1, 0, 0,  0, 0, 1,  0, 1, 0),
    new TransformMatrix(1, 0, 0,  0, -1, 0,  0, 0, -1),
    new TransformMatrix(0, 0, 1,  1, 0, 0,  0, 1, 0),
    new TransformMatrix(0, 0, -1,  0, -1, 0,  -1, 0, 0),
];

// possible matrix values for corners on orange wall
let corangemx = [
    new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0),
    new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0),
    new TransformMatrix(0, 1, 0,  1, 0, 0,  0, 0, -1),
    new TransformMatrix(0, 1, 0,  0, 0, -1,  -1, 0, 0),
    new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1),
    new TransformMatrix(0, 0, -1,  1, 0, 0,  0, -1, 0),
    new TransformMatrix(0, -1, 0,  0, 0, 1,  -1, 0, 0),
    new TransformMatrix(1, 0, 0,  0, -1, 0,  0, 0, -1),
    new TransformMatrix(0, 0, 1,  1, 0, 0,  0, 1, 0),
    new TransformMatrix(0, 0, -1,  0, -1, 0,  -1, 0, 0),
    new TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0),
    new TransformMatrix(0, -1, 0,  1, 0, 0,  0, 0, 1),
];

// possible matrix values for edges on orange wall
let eorangemx = [
    new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0),
    new TransformMatrix(0, 0, -1,  1, 0, 0,  0, -1, 0),
    new TransformMatrix(0, 1, 0,  0, 0, -1,  -1, 0, 0),
    new TransformMatrix(0, -1, 0,  1, 0, 0,  0, 0, 1),
    new TransformMatrix(0, 0, -1,  0, -1, 0,  -1, 0, 0),
    new TransformMatrix(0, 0, 1,  1, 0, 0,  0, 1, 0),
    new TransformMatrix(0, -1, 0,  0, 0, 1,  -1, 0, 0),
    new TransformMatrix(0, 1, 0,  1, 0, 0,  0, 0, -1),
];

// The cube in corner/edge transformation matrixes format
class CubeTrMatrix {
    #corners;
    #edges;
    #middles;

    static solved;
    static {
        CubeTrMatrix.solved = new CubeTrMatrix();
        CubeTrMatrix.solved.#corners = [
            cornersmx[0][0], cornersmx[1][0], cornersmx[2][0], cornersmx[3][0],
            cornersmx[4][0], cornersmx[5][0], cornersmx[6][0], cornersmx[7][0]
        ];
        CubeTrMatrix.solved.#edges = [
            edgesmx[0][0], edgesmx[1][0], edgesmx[2][0], edgesmx[3][0],
            edgesmx[4][0], edgesmx[5][0], edgesmx[6][0], edgesmx[7][0],
            edgesmx[8][0], edgesmx[9][0], edgesmx[10][0], edgesmx[11][0]
        ];
        CubeTrMatrix.solved.#middles = [
            new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0), // yellow
            new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0), // orange
            new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1), // blue
            new TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0), // red
            new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1), // green
            new TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0), // white
        ];
    }

    equals(cubemx2, includingMiddle) {
        for(let i = 0; i < 8; ++i)
            if( !this.#corners[i].equals(cubemx2.#corners[i] ) )
                return false;
        for(let i = 0; i < 12; ++i)
            if( !this.#edges[i].equals(cubemx2.#edges[i] ) )
                return false;
        if( includingMiddle ) {
            for(let i = 0; i < 6; ++i)
                if( !this.#middles[i].equals(cubemx2.#middles[i] ) )
                    return false;
        }
        return true;
    }

    static fromCube(c, cubemxmid) {
        let rescubemx = new CubeTrMatrix();
        rescubemx.#corners = [];
        for(let i = 0; i < 8; ++i) {
            let cno = c.cc.getPermAt(i);
            rescubemx.#corners[cno] = cornersmx[i][c.cc.getOrientAt(i)];
        }
        rescubemx.#edges = [];
        for(let i = 0; i < 12; ++i) {
            let eno = c.ce.edgeN(i);
            rescubemx.#edges[eno] = edgesmx[i][c.ce.edgeR(i)];
        }
        rescubemx.#middles = cubemxmid.#middles;
        return rescubemx;
    }

    toCube() {
        let cc = new cubecorners();
        for(let i = 0; i < 8; ++i) {
            for(let cno = 0; cno < 8; ++cno) {
                for(let orient = 0; orient < 3; ++orient) {
                    if( this.#corners[cno].equals(cornersmx[i][orient]) ) {
                        cc.setPermAt(i, cno);
                        cc.setOrientAt(i, orient);
                    }
                }
            }
        }
        let ce = new cubeedges();
        for(let i = 0; i < 12; ++i) {
            for(let eno = 0; eno < 12; ++eno) {
                for(let orient = 0; orient < 2; ++orient) {
                    if( this.#edges[eno].equals(edgesmx[i][orient]) ) {
                        ce.set(i, eno, orient);
                    }
                }
            }
        }
        return new cube(cc, ce);
    }

    static fromScrambleStr(s) {
        const rotateMapFromExtFmt = new Map([
            [ 'L1', ORANGECW ],
            [ 'L2', ORANGE180],
            [ 'L3', ORANGECCW],
            [ 'R1', REDCW    ],
            [ 'R2', RED180   ],
            [ 'R3', REDCCW   ],
            [ 'U1', YELLOWCW ],
            [ 'U2', YELLOW180],
            [ 'U3', YELLOWCCW],
            [ 'D1', WHITECW  ],
            [ 'D2', WHITE180 ],
            [ 'D3', WHITECCW ],
            [ 'B1', GREENCW  ],
            [ 'B2', GREEN180 ],
            [ 'B3', GREENCCW ],
            [ 'F1', BLUECW   ],
            [ 'F2', BLUE180  ],
            [ 'F3', BLUECCW  ],
        ]);

        let rescubemx = CubeTrMatrix.solved;
        for(let i = 0; i+1 < s.length; i+=2) {
            let mappedVal = rotateMapFromExtFmt.get(s.substring(i, i+2));
            if( mappedVal != undefined )
                rescubemx = rescubemx.rotateWall(mappedVal)
            else
                dolog('err', `Unkown move: ${s.substring(i, i+2)}`);
        }
        return rescubemx;
    }

    static fromString(s) {
        let rescubemx;
        if( /[Yy]/.test(s) ) {
            let c = cubeFromColorsOnWalls(s);
            if( c )
                rescubemx = CubeTrMatrix.fromCube(c, curcubemx);
        }else{
            rescubemx = CubeTrMatrix.fromScrambleStr(s);
        }
        return rescubemx;
    }

    #rotateWallInt(ccolormx, ecolormx, colornum, transform, wcolor) {
        let rescubemx = new CubeTrMatrix();
        rescubemx.#corners = [];
        for(let i = 0; i < 8; ++i) {
            if( this.#corners[i].isIn(ccolormx) )
                rescubemx.#corners[i] = this.#corners[i].multiplyRndWith(transform);
            else
                rescubemx.#corners[i] = this.#corners[i];
        }
        rescubemx.#edges = [];
        for(let i = 0; i < 12; ++i) {
            if( this.#edges[i].isIn(ecolormx) )
                rescubemx.#edges[i] = this.#edges[i].multiplyRndWith(transform);
            else
                rescubemx.#edges[i] = this.#edges[i];
        }
        rescubemx.#middles = [];
        for(let i = 0; i < 6; ++i)
            rescubemx.#middles[i] = this.#middles[i];
        rescubemx.#middles[colornum] = this.#middles[colornum].multiplyRndWith(transform);
        return rescubemx;
    }

    rotateWall(rotateDir) {
        let rescubemx = this;

        switch( rotateDir ) {
            case ORANGECW:
                rescubemx = this.#rotateWallInt(corangemx, eorangemx, 1,
                    new TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0), worange);
                break;
            case ORANGE180:
                rescubemx = this.#rotateWallInt(corangemx, eorangemx, 1,
                    new TransformMatrix(1, 0, 0,  0, -1, 0,  0, 0, -1), worange);
                break;
            case ORANGECCW:
                rescubemx = this.#rotateWallInt(corangemx, eorangemx, 1,
                    new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0), worange);
                break;
            case REDCW:
                rescubemx = this.#rotateWallInt(credmx, eredmx, 3,
                    new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0), wred);
                break;
            case RED180:
                rescubemx = this.#rotateWallInt(credmx, eredmx, 3,
                    new TransformMatrix(1, 0, 0,  0, -1, 0,  0, 0, -1), wred);
                break;
            case REDCCW:
                rescubemx = this.#rotateWallInt(credmx, eredmx, 3,
                    new TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0), wred);
                break;
            case YELLOWCW:
                rescubemx = this.#rotateWallInt(cyellowmx, eyellowmx, 0,
                    new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0), wyellow);
                break;
            case YELLOW180:
                rescubemx = this.#rotateWallInt(cyellowmx, eyellowmx, 0,
                    new TransformMatrix(-1, 0, 0,  0, 1, 0,  0, 0, -1), wyellow);
                break;
            case YELLOWCCW:
                rescubemx = this.#rotateWallInt(cyellowmx, eyellowmx, 0,
                    new TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0), wyellow);
                break;
            case WHITECW:
                rescubemx = this.#rotateWallInt(cwhitemx, ewhitemx, 5,
                    new TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0), wwhite);
                break;
            case WHITE180:
                rescubemx = this.#rotateWallInt(cwhitemx, ewhitemx, 5,
                    new TransformMatrix(-1, 0, 0,  0, 1, 0,  0, 0, -1), wwhite);
                break;
            case WHITECCW:
                rescubemx = this.#rotateWallInt(cwhitemx, ewhitemx, 5,
                    new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0), wwhite);
                break;
            case GREENCW:
                rescubemx = this.#rotateWallInt(cgreenmx, egreenmx, 4,
                    new TransformMatrix(0, -1, 0,  1, 0, 0,  0, 0, 1), wgreen);
                break;
            case GREEN180:
                rescubemx = this.#rotateWallInt(cgreenmx, egreenmx, 4,
                    new TransformMatrix(-1, 0, 0,  0, -1, 0,  0, 0, 1), wgreen);
                break;
            case GREENCCW:
                rescubemx = this.#rotateWallInt(cgreenmx, egreenmx, 4,
                    new TransformMatrix(0, 1, 0,  -1, 0, 0,  0, 0, 1), wgreen);
                break;
            case BLUECW:
                rescubemx = this.#rotateWallInt(cbluemx, ebluemx, 2,
                    new TransformMatrix(0, 1, 0,  -1, 0, 0,  0, 0, 1), wblue);
                break;
            case BLUE180:
                rescubemx = this.#rotateWallInt(cbluemx, ebluemx, 2,
                    new TransformMatrix(-1, 0, 0,  0, -1, 0,  0, 0, 1), wblue);
                break;
            case BLUECCW:
                rescubemx = this.#rotateWallInt(cbluemx, ebluemx, 2,
                    new TransformMatrix(0, -1, 0,  1, 0, 0,  0, 0, 1), wblue);
                break;
        }
        return rescubemx;
    }

    applyToCubeImg() {
        let corners = [ c1, c2, c3, c4, c5, c6, c7, c8 ];
        let edges = [ e1, e2, e3, e4, e5, e6, e7, e8, e9, ea, eb, ec ];
        let wcolors = [ wyellow, worange, wblue, wred, wgreen, wwhite ];
        for(let i = 0; i < 8; ++i)
            corners[i].style.transform = `matrix3d(${this.#corners[i].toMatrix3dParam()})`;
        for(let i = 0; i < 12; ++i)
            edges[i].style.transform = `matrix3d(${this.#edges[i].toMatrix3dParam()})`;
        for(let i = 0; i < 6; ++i)
            wcolors[i].style.transform = `matrix3d(${this.#middles[i].toMatrix3dParam()})`;
    }
}

let curcubemx = CubeTrMatrix.solved;

const STYLEAPPLY_IMMEDIATE = 1;
const STYLEAPPLY_DEFER = 2;

function rotateCurWall(rotateDir, styleApply) {
    function rotateCurWallInt(rotateDir, styleApply) {
        curcubemx = curcubemx.rotateWall(rotateDir);
        if( styleApply === STYLEAPPLY_DEFER )
            setTimeout(() => curcubemx.applyToCubeImg(), 50);
        else
            curcubemx.applyToCubeImg();
    }
    function rotateCurWallInt180(rotateDirCW) {
        if( styleApply === undefined ) {
            // minimize split rotation of pieces
            rotateCurWallInt(rotateDirCW, STYLEAPPLY_IMMEDIATE);
            rotateCurWallInt(rotateDirCW, STYLEAPPLY_DEFER);
        }else{
            rotateCurWallInt(rotateDir, styleApply);
        }
    }

    switch( rotateDir ) {
        case ORANGE180:
            rotateCurWallInt180(ORANGECW);
            break;
        case RED180:
            rotateCurWallInt180(REDCW);
            break;
        case YELLOW180:
            rotateCurWallInt180(YELLOWCW);
            break;
        case WHITE180:
            rotateCurWallInt180(WHITECW);
            break;
        case GREEN180:
            rotateCurWallInt180(GREENCW);
            break;
        case BLUE180:
            rotateCurWallInt180(BLUECW);
            break;
        default:
            rotateCurWallInt(rotateDir,
                styleApply == undefined ? STYLEAPPLY_IMMEDIATE : styleApply);
            break;
    }
}

function rotateBlueWall90() { rotateCurWall(BLUECW); }
function rotateBlueWall180() { rotateCurWall(BLUE180); }
function rotateBlueWall270() { rotateCurWall(BLUECCW); }
function rotateRedWall90() { rotateCurWall(REDCW); }
function rotateRedWall180() { rotateCurWall(RED180); }
function rotateRedWall270() { rotateCurWall(REDCCW); }
function rotateYellowWall90() { rotateCurWall(YELLOWCW); }
function rotateYellowWall180() { rotateCurWall(YELLOW180); }
function rotateYellowWall270() { rotateCurWall(YELLOWCCW); }
function rotateGreenWall90() { rotateCurWall(GREENCW); }
function rotateGreenWall180() { rotateCurWall(GREEN180); }
function rotateGreenWall270() { rotateCurWall(GREENCCW); }
function rotateOrangeWall90() { rotateCurWall(ORANGECW); }
function rotateOrangeWall180() { rotateCurWall(ORANGE180); }
function rotateOrangeWall270() { rotateCurWall(ORANGECCW); }
function rotateWhiteWall90() { rotateCurWall(WHITECW); }
function rotateWhiteWall180() { rotateCurWall(WHITE180); }
function rotateWhiteWall270() { rotateCurWall(WHITECCW); }

let moves = [], moveidx = -1, moveidxgoal = -1, rewind = 0;

let lastMoveTime = 0;

setInterval(function () {
    let curTime = Date.now();
    if( curTime < lastMoveTime + 600 )
        return;
    if( moveidx < moveidxgoal ) {
        ++moveidx;
        document.querySelectorAll(`#movebuttons > button`).forEach( (btn) => { btn.classList.remove('currentmv'); });
        document.querySelector(`#movebutton${moveidx}`).classList.add('currentmv');
        if( moveidx < moves.length ) {
            rotateCurWall(moves[moveidx]);
        }
        lastMoveTime = curTime;
    }else if( moveidx > moveidxgoal ) {
        while( moveidx > moveidxgoal ) {
            if( moveidx < moves.length ) {
                rotateCurWall(rotateDirReverse(moves[moveidx]), STYLEAPPLY_IMMEDIATE);
            }
            --moveidx;
        }
        document.querySelectorAll(`#movebuttons > button`).forEach( (btn) => { btn.classList.remove('currentmv'); });
        if( moveidx >= 0 )
            document.querySelector(`#movebutton${moveidx}`).classList.add('currentmv');
        else
            movebuttonini.classList.add('currentmv');
        lastMoveTime = curTime;
    }else if( moveidx == moveidxgoal && rewind ) {
        if( --rewind == 0 )
            moveidxgoal = -1;
    }
}, 100);

let fixedCornerColors = [
    [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1],
    [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1]
];
let fixedEdgeColors = [
    [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1],
    [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1]
];

class AllowedPerm {
    perm = -1;
    orients = [];

    copy() {
        let res = new AllowedPerm();
        res.perm = this.perm;
        res.orients = this.orients.slice();
        return res;
    }
}

const PERMPARITY_ODD = 1;
const PERMPARITY_EVEN = 2;

function getPermArrParity(allowedPermArr) {
    isPermSwapsOdd = false;
    let permsScanned = [];
    for(let i = 0; i < allowedPermArr.length; ++i) {
        if( !permsScanned[i] ) {
            permsScanned[i] = true;
            let p = i;
            while( (p = allowedPermArr[p].perm) != i ) {
                permsScanned[p] = true;
                isPermSwapsOdd = !isPermSwapsOdd;
            }
        }
    }
    return isPermSwapsOdd ? PERMPARITY_ODD : PERMPARITY_EVEN;
}

class AllowedCornerColors {
    permParity = 0;
    parityOdd = [
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
    ];
    parityEven = [
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
    ];
    parityAny = [
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
        [new Set(), new Set(), new Set()],
    ];
    sampleCubecorners = [];

    add(cornerNo, squareNo, permParity, color) {
        this.permParity |= permParity;
        this.parityAny[cornerNo][squareNo].add(color);
        if( permParity & PERMPARITY_ODD )
            this.parityOdd[cornerNo][squareNo].add(color);
        if( permParity & PERMPARITY_EVEN )
            this.parityEven[cornerNo][squareNo].add(color);
    }
    get(cornerNo, squareNo, permParity) {
        let rset = permParity == PERMPARITY_EVEN ? this.parityEven :
            permParity == PERMPARITY_ODD ? this.parityOdd : this.parityAny;
        return rset[cornerNo][squareNo];
    }
}

class AllowedEdgeColors {
    permParity = 0;
    parityOdd = [
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()]
    ];
    parityEven = [
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()]
    ];
    parityAny = [
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()],
        [new Set(), new Set()], [new Set(), new Set()]
    ];
    sampleCubeedges = [];

    add(cornerNo, squareNo, permParity, color) {
        this.permParity |= permParity;
        this.parityAny[cornerNo][squareNo].add(color);
        if( permParity & PERMPARITY_ODD )
            this.parityOdd[cornerNo][squareNo].add(color);
        if( permParity & PERMPARITY_EVEN )
            this.parityEven[cornerNo][squareNo].add(color);
    }
    get(edgeNo, squareNo, permParity) {
        let rset = permParity == PERMPARITY_EVEN ? this.parityEven :
            permParity == PERMPARITY_ODD ? this.parityOdd : this.parityAny;
        return rset[edgeNo][squareNo];
    }
}

function *cornerPermOnFixedColors(
    occupCorners = [false, false, false, false, false, false, false, false],
    perm = [new AllowedPerm(), new AllowedPerm(), new AllowedPerm(), new AllowedPerm(),
            new AllowedPerm(), new AllowedPerm(), new AllowedPerm(), new AllowedPerm()],
    filledCount = 0)
{
    while( filledCount < 8 && fixedCornerColors[filledCount][0] == -1 &&
            fixedCornerColors[filledCount][1] == -1 &&
            fixedCornerColors[filledCount][2] == -1)
        ++filledCount;
    if( filledCount == 8 ) {
        let yieldPerm = [];
        let twoOrientPerms = [];
        let unsetPerms = [];
        let singleOrientPermSum = 0;
        let freeCorners = [];
        for(let cno = 0; cno < 8; ++cno) {
            let allowedPerm = yieldPerm[cno] = perm[cno].copy();
            if( allowedPerm.perm >= 0 ) {
                switch( allowedPerm.orients.length ) {
                    case 1:
                        singleOrientPermSum += allowedPerm.orients[0];
                        break;
                    case 2:
                        twoOrientPerms.push(allowedPerm);
                        break;
                }
            }else
                unsetPerms.push(allowedPerm);
            if( !occupCorners[cno] )
                freeCorners.push(cno);
        }
        if( unsetPerms.length == 2 && twoOrientPerms.length == 0 ) {
            let allowedPerm1 = unsetPerms[0];
            let allowedPerm2 = unsetPerms[1];
            allowedPerm1.orients = allowedPerm2.orients = [ 0, 1, 2 ];
            allowedPerm1.perm = freeCorners[0];
            allowedPerm2.perm = freeCorners[1];
            yield {
                perm: yieldPerm,
                freeCorners: [],
                permParity: getPermArrParity(yieldPerm)
            };
            allowedPerm1.perm = freeCorners[1];
            allowedPerm2.perm = freeCorners[0];
            yield {
                perm: yieldPerm,
                freeCorners: [],
                permParity: getPermArrParity(yieldPerm)
            };
        }else{
            if( unsetPerms.length == 1 && twoOrientPerms.length == 0 ) {
                let freeVal = freeCorners[0];
                freeCorners.splice(0, 1);
                let allowedPerm = unsetPerms[0];
                allowedPerm.perm = freeVal;
                allowedPerm.orients.push((15-singleOrientPermSum)%3);
                unsetPerms.length = 0;
            }else if( unsetPerms.length == 0 ) {
                if( twoOrientPerms.length == 1 ) {
                    let orients = twoOrientPerms[0].orients;
                    if( (singleOrientPermSum + orients[1]) % 3 != 0 )
                        orients.splice(1, 1);
                    if( (singleOrientPermSum + orients[0]) % 3 != 0 )
                        orients.splice(0, 1);
                    // only one orient should remain
                    twoOrientPerms.length = 0;
                }else if( twoOrientPerms.length == 2 ) {
                    let orients1 = twoOrientPerms[0].orients;
                    let orients2 = twoOrientPerms[1].orients;
                    if( (singleOrientPermSum + orients1[1] + orients2[0]) % 3 != 0 &&
                            (singleOrientPermSum + orients1[1] + orients2[1]) % 3 != 0)
                        orients1.splice(1, 1);
                    if( (singleOrientPermSum + orients1[0] + orients2[0]) % 3 != 0 &&
                            (singleOrientPermSum + orients1[0] + orients2[1]) % 3 != 0)
                        orients1.splice(0, 1);
                    if( orients1.length == 1 ) {    // if one orient was removed
                        if( (singleOrientPermSum + orients1[0] + orients2[1]) % 3 != 0 )
                            orients2.splice(1, 1);
                        if( (singleOrientPermSum + orients1[0] + orients2[0]) % 3 != 0 )
                            orients2.splice(0, 1);
                        // only one orient should remain
                        twoOrientPerms.length = 0;
                    }
                }else if( twoOrientPerms.length == 0 ) {
                    if( singleOrientPermSum % 3 )
                        return;
                }
            }
            let permParity = PERMPARITY_ODD | PERMPARITY_EVEN;
            if( unsetPerms.length == 0 && twoOrientPerms.length == 0 )
                permParity = getPermArrParity(yieldPerm);
            yield {
                perm: yieldPerm,
                freeCorners,
                permParity
            };
        }
    }else{
        let allowedPerm = perm[filledCount];
        for(let cno = 0; cno < 8; ++cno) {
            if( !occupCorners[cno] ) {
                allowedPerm.perm = cno;
                for(let orient = 0; orient < 3; ++orient) {
                    let isMatching = true;
                    for(let j = 0; j < 3; ++j) {
                        let owall = (orient+j) % 3;
                        if( fixedCornerColors[filledCount][j] != -1 &&
                                fixedCornerColors[filledCount][j] != cubeCornerColors[cno][owall] )
                            isMatching = false;
                    }
                    if( isMatching )
                        allowedPerm.orients.push(orient);
                }
                if( allowedPerm.orients.length > 0 ) {
                    occupCorners[cno] = true;
                    yield *cornerPermOnFixedColors(occupCorners, perm, filledCount+1);
                    occupCorners[cno] = false;
                    allowedPerm.orients.length = 0;
                }
            }
        }
        allowedPerm.perm = -1;
    }
}

function getAllowedCornerColors() {
    let allowedCornerColors = new AllowedCornerColors();
    for(let po of cornerPermOnFixedColors()) {
        let cc = null;
        if( allowedCornerColors.sampleCubecorners[po.permParity] == undefined )
            allowedCornerColors.sampleCubecorners[po.permParity] = cc = new cubecorners();
        for(let i = 0; i < 8; ++i) {
            let allowedPerm = po.perm[i];
            if( allowedPerm.perm >= 0 ) {
                if( cc )
                    cc.setPermAt(i, allowedPerm.perm);
                for(let orient of allowedPerm.orients) {
                    if( cc )
                        cc.setOrientAt(i, orient);
                    for(let j = 0; j < 3; ++j) {
                        let owall = (orient+j) % 3;
                        allowedCornerColors.add(i, j, po.permParity, cubeCornerColors[allowedPerm.perm][owall]);
                    }
                }
            }else{
                for(let cno of po.freeCorners) {
                    for(let orient = 0; orient < 3; ++orient) {
                        for(let j = 0; j < 3; ++j) {
                            let owall = (orient+j) % 3;
                            allowedCornerColors.add(i, j, po.permParity, cubeCornerColors[cno][owall]);
                        }
                    }
                }
            }
        }
    }
    return allowedCornerColors;
}

function *edgePermOnFixedColors(
    occupEdges = [false, false, false, false, false, false, false, false, false, false, false, false],
    perm = [new AllowedPerm(), new AllowedPerm(), new AllowedPerm(), new AllowedPerm(),
            new AllowedPerm(), new AllowedPerm(), new AllowedPerm(), new AllowedPerm(),
            new AllowedPerm(), new AllowedPerm(), new AllowedPerm(), new AllowedPerm()],
    filledCount = 0)
{
    while( filledCount < 12 && fixedEdgeColors[filledCount][0] == -1 &&
            fixedEdgeColors[filledCount][1] == -1 )
        ++filledCount;
    if( filledCount == 12 ) {
        let yieldPerm = [];
        let unsetPerms = [];
        let orientSum = 0;
        let freeEdges = [];
        for(let eno = 0; eno < 12; ++eno) {
            let allowedPerm = yieldPerm[eno] = perm[eno].copy();
            if( allowedPerm.perm >= 0 )
                orientSum += allowedPerm.orients[0];
            else
                unsetPerms.push(allowedPerm);
            if( !occupEdges[eno] )
                freeEdges.push(eno);
        }
        if( unsetPerms.length == 2 ) {
            let allowedPerm1 = unsetPerms[0];
            let allowedPerm2 = unsetPerms[1];
            allowedPerm1.orients = allowedPerm2.orients = [ 0, 1 ];
            allowedPerm1.perm = freeEdges[0];
            allowedPerm2.perm = freeEdges[1];
            yield {
                perm: yieldPerm,
                freeEdges: [],
                permParity: getPermArrParity(yieldPerm)
            };
            allowedPerm1.perm = freeEdges[1];
            allowedPerm2.perm = freeEdges[0];
            yield {
                perm: yieldPerm,
                freeEdges: [],
                permParity: getPermArrParity(yieldPerm)
            };
        }else{
            if( unsetPerms.length == 1 ) {
                let freeVal = freeEdges[0];
                freeEdges.splice(0, 1);
                let allowedPerm = unsetPerms[0];
                allowedPerm.perm = freeVal;
                allowedPerm.orients.push(orientSum % 2);
                unsetPerms.length = 0;
            }else if( unsetPerms.length == 0 ) {
                if( orientSum % 2 )
                    return;
            }
            let permParity = PERMPARITY_ODD | PERMPARITY_EVEN;
            if( unsetPerms.length == 0 )
                permParity = getPermArrParity(yieldPerm);
            yield {
                perm: yieldPerm,
                freeEdges,
                permParity
            };
        }
    }else{
        let allowedPerm = perm[filledCount];
        for(let eno = 0; eno < 12; ++eno) {
            if( !occupEdges[eno] ) {
                allowedPerm.perm = eno;
                for(let orient = 0; orient < 2; ++orient) {
                    let isMatching = true;
                    for(let j = 0; j < 2; ++j) {
                        let owall = (orient+j) % 2;
                        if( fixedEdgeColors[filledCount][j] != -1 &&
                                fixedEdgeColors[filledCount][j] != cubeEdgeColors[eno][owall] )
                            isMatching = false;
                    }
                    if( isMatching )
                        allowedPerm.orients.push(orient);
                }
                if( allowedPerm.orients.length > 0 ) {
                    occupEdges[eno] = true;
                    yield *edgePermOnFixedColors(occupEdges, perm, filledCount+1);
                    occupEdges[eno] = false;
                    allowedPerm.orients.length = 0;
                }
            }
        }
        allowedPerm.perm = -1;
    }
}

function getAllowedEdgeColors() {
    let allowedEdgeColors = new AllowedEdgeColors();
    for(let po of edgePermOnFixedColors()) {
        let ce = null;
        if( allowedEdgeColors.sampleCubeedges[po.permParity] == undefined )
            allowedEdgeColors.sampleCubeedges[po.permParity] = ce = new cubeedges();
        for(let i = 0; i < 12; ++i) {
            let allowedPerm = po.perm[i];
            if( allowedPerm.perm >= 0 ) {
                if( ce )
                    ce.setPermAt(i, allowedPerm.perm);
                for(let orient of allowedPerm.orients) {
                    if( ce )
                        ce.setOrientAt(i, orient);
                    for(let j = 0; j < 2; ++j) {
                        let owall = (orient+j) % 2;
                        allowedEdgeColors.add(i, j, po.permParity, cubeEdgeColors[allowedPerm.perm][owall]);
                    }
                }
            }else{
                for(let eno of po.freeEdges) {
                    for(let orient = 0; orient < 2; ++orient) {
                        for(let j = 0; j < 2; ++j) {
                            let owall = (orient+j) % 2;
                            allowedEdgeColors.add(i, j, po.permParity, cubeEdgeColors[eno][owall]);
                        }
                    }
                }
            }
        }
    }
    return allowedEdgeColors;
}

function selectColors(ev) {
    const colorClassNames = [ 'cyellow', 'corange', 'cblue', 'cred', 'cgreen', 'cwhite' ];
    const cornerIds = ['c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7', 'c8'];
    const edgeIds = [ 'e1', 'e2', 'e3', 'e4', 'e5', 'e6', 'e7', 'e8', 'e9', 'ea', 'eb', 'ec' ];
    const cornerSquareClasses = [ 'csq1', 'csq2', 'csq3' ];
    const edgeSquareClasses = [ 'esq1', 'esq2' ];
    let thisElem = ev.target;
    if( ev.ctrlKey ) {
        if( thisElem.parentElement.parentElement.classList.contains('corner') ) {
            let cornerNo = cornerIds.indexOf(thisElem.parentElement.parentElement.id);
            fixedCornerColors[cornerNo][0] = -1;
            fixedCornerColors[cornerNo][1] = -1;
            fixedCornerColors[cornerNo][2] = -1;
        }else{
            let edgeNo = edgeIds.indexOf(thisElem.parentElement.parentElement.id);
            fixedEdgeColors[edgeNo][0] = -1;
            fixedEdgeColors[edgeNo][1] = -1;
        }
    }else{
        if( thisElem.parentElement.parentElement.classList.contains('corner') ) {
            let cornerNo = cornerIds.indexOf(thisElem.parentElement.parentElement.id);
            let squareNo = 0;
            while( !thisElem.parentElement.classList.contains(cornerSquareClasses[squareNo]) )
                ++squareNo;
            let wallColor = -1;
            if( fixedCornerColors[cornerNo][squareNo] == -1 ) {
                wallColor = 0;
                while( !thisElem.classList.contains(colorClassNames[wallColor]) )
                    ++wallColor;
            }
            fixedCornerColors[cornerNo][squareNo] = wallColor;
        }else{
            let edgeNo = edgeIds.indexOf(thisElem.parentElement.parentElement.id);
            let squareNo = 0;
            while( !thisElem.parentElement.classList.contains(edgeSquareClasses[squareNo]) )
                ++squareNo;
            let wallColor = -1;
            if( fixedEdgeColors[edgeNo][squareNo] == -1 ) {
                wallColor = 0;
                while( !thisElem.classList.contains(colorClassNames[wallColor]) )
                    ++wallColor;
            }
            fixedEdgeColors[edgeNo][squareNo] = wallColor;
        }
    }
    document.querySelectorAll('.csel').forEach( (el) => { el.classList.add('cnone'); } );
    let isFilled = true;
    let allowedCornerColors = getAllowedCornerColors();
    let allowedEdgeColors = getAllowedEdgeColors();
    let permParity = allowedEdgeColors.permParity & allowedCornerColors.permParity;
    for(let cno = 0; cno < 8; ++cno) {
        for(let sqno = 0; sqno < 3; ++sqno) {
            let colorSet = allowedCornerColors.get(cno, sqno, permParity);
            for(let colorVal of colorSet) {
                let cselElem = document.querySelector(
                    `#${cornerIds[cno]} > .${cornerSquareClasses[sqno]} > .${colorClassNames[colorVal]}`);
                cselElem.classList.remove('cnone');
            }
            if( colorSet.size > 1 )
                isFilled = false;
        }
    }
    for(let eno = 0; eno < 12; ++eno) {
        for(let sqno = 0; sqno < 2; ++sqno) {
            let colorSet = allowedEdgeColors.get(eno, sqno, permParity);
            for(let colorVal of colorSet) {
                let selector = `#${edgeIds[eno]} > .${edgeSquareClasses[sqno]} > .${colorClassNames[colorVal]}`;
                let cselElem = document.querySelector(selector);
                cselElem.classList.remove('cnone');
            }
            if( colorSet.size > 1 )
                isFilled = false;
        }
    }
    applybtn.disabled = ! isFilled;
}

function areCubeColorsFilled() {
    let allowedCornerColors = getAllowedCornerColors();
    let allowedEdgeColors = getAllowedEdgeColors();
    let permParity = allowedEdgeColors.permParity & allowedCornerColors.permParity;
    for(let cno = 0; cno < 8; ++cno) {
        for(let sqno = 0; sqno < 3; ++sqno) {
            if( allowedCornerColors.get(cno, sqno, permParity).size > 1 )
                return false;
        }
    }
    for(let eno = 0; eno < 12; ++eno) {
        for(let sqno = 0; sqno < 2; ++sqno) {
            if( allowedEdgeColors.get(eno, sqno, permParity).size > 1 )
                return false;
        }
    }
    return true;
}

let cmanipulate = CubeTrMatrix.solved;

function enterEditMode() {
    cmanipulate = curcubemx;
    curcubemx = CubeTrMatrix.solved;
    curcubemx.applyToCubeImg();
    cubeimg.classList.add('editmode');
    document.querySelectorAll('.editmodeonly').forEach( (el) => { el.disabled = false; });
    document.querySelectorAll('.manipmodeonly').forEach( (el) => { el.disabled = true; });
    applybtn.disabled = ! areCubeColorsFilled();
}

function doReset() {
    if( cubeimg.classList.contains('editmode') ) {
        fixedCornerColors = [
            [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1],
            [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1]
        ];
        fixedEdgeColors = [
            [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1],
            [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1]
        ];
        document.querySelectorAll('.csel').forEach( (el) => { el.classList.remove('cnone'); } );
    }else{
        curcubemx = CubeTrMatrix.solved;
        curcubemx.applyToCubeImg();
    }
}

function enterManipulateMode() {
    cubeimg.classList.remove('editmode');
    curcubemx = cmanipulate;
    curcubemx.applyToCubeImg();
    document.querySelectorAll('.editmodeonly').forEach( (el) => { el.disabled = true; });
    document.querySelectorAll('.manipmodeonly').forEach( (el) => { el.disabled = false; });
    applybtn.disabled = true;
}

function applyEdit() {
    let allowedCornerColors = getAllowedCornerColors();
    let allowedEdgeColors = getAllowedEdgeColors();
    let permParity = allowedEdgeColors.permParity & allowedCornerColors.permParity;
    let cc = allowedCornerColors.sampleCubecorners[permParity];
    let ce = allowedEdgeColors.sampleCubeedges[permParity];
	cmanipulate = CubeTrMatrix.fromCube(new cube(cc, ce), curcubemx);
    manipmodebtn.checked = true;
    enterManipulateMode();
}

function searchSolution() {
    document.querySelectorAll('.log').forEach((e) => {e.textContent = ''});
    let c = curcubemx.toCube();
    localStorage.setItem('cube', `${c.cc.getPerms().getPermVal()} ${c.cc.getOrients().get()} ${c.ce.get()}`);
    if( c.equals(csolved) )
        dolog('err', "already solved\n");
    else{
        document.querySelectorAll('.manipmodeonly,.changingcube').forEach((e) => {e.disabled = true;});
        searchMoves(c);
    }
}

function loadFromFile() {
    showOpenFilePicker({
        types: [ {
            description: 'saved cube',
            accept: { 'text/plain': [ '.txt' ] }
        } ]
    }).then(async function ( [fileHandle] ) {
        let file = await fileHandle.getFile();
        let val = await file.text();
        curcubemx = CubeTrMatrix.fromString(val);
        curcubemx.applyToCubeImg();
    }, function() {});
}

function saveToFile() {
    let fileSeq = localStorage.getItem('fileseq');
    if( !fileSeq )
        fileSeq = '1';
    let fname = 'c' + fileSeq + '.txt';
    showSaveFilePicker({
        suggestedName: fname,
        types: [ {
            description: 'saved cube',
            accept: { 'text/plain': [ '.txt' ] }
        } ]
    }).then(async function(fileHandle) {
        const fout = await fileHandle.createWritable();
        let c = curcubemx.toCube();
        let saveText = cubeToSaveText(c);
        await fout.write(saveText);
        await fout.close();
        localStorage.setItem('fileseq', +fileSeq+1);
    }, function() {});
}

function loadFromInput() {
    err.textContent = '';
    let cubemx = CubeTrMatrix.fromString(loadinput.value);
    if( cubemx ) {
        curcubemx = cubemx;
        curcubemx.applyToCubeImg();
    }
}

onload = () => {
    if( window['showOpenFilePicker'] == undefined ) {
        loadsavebtns.style.display = 'none';
        document.querySelector('#cubefile').addEventListener('change', (ev) => {
            let file = ev.target.files[0];
            file.text().then((val) => {
                curcubemx = CubeTrMatrix.fromString(val);
                curcubemx.applyToCubeImg();
            });
        });
    }else{
        loadonlybtn.style.display = 'none';
        loadfromfilebtn.addEventListener('click', loadFromFile);
        savetofilebtn.addEventListener('click', saveToFile);
    }
    loadfrominputbtn.addEventListener('click', loadFromInput);
    loadinput.addEventListener('change', loadFromInput);
    document.querySelector('#rxubutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = cubeimgmx.multiplyWith(new TransformMatrix(1, 0, 0,  0, c, s,  0, -s, c));
        cubeimg.style.transform = `matrix3d(${cubeimgmx.toMatrix3dParam()})`;
    });
    document.querySelector('#rxdbutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = cubeimgmx.multiplyWith(new TransformMatrix(1, 0, 0,  0, c, -s,  0, s, c));
        cubeimg.style.transform = `matrix3d(${cubeimgmx.toMatrix3dParam()})`;
    });
    document.querySelector('#rylbutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = cubeimgmx.multiplyWith(new TransformMatrix(c, 0, s,  0, 1, 0,  -s, 0, c));
        cubeimg.style.transform = `matrix3d(${cubeimgmx.toMatrix3dParam()})`;
    });
    document.querySelector('#ryrbutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = cubeimgmx.multiplyWith(new TransformMatrix(c, 0, -s,  0, 1, 0,  s, 0, c));
        cubeimg.style.transform = `matrix3d(${cubeimgmx.toMatrix3dParam()})`;
    });
    document.querySelector('#rzlbutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = cubeimgmx.multiplyWith(new TransformMatrix(c, -s, 0,  s, c, 0,  0, 0, 1));
        cubeimg.style.transform = `matrix3d(${cubeimgmx.toMatrix3dParam()})`;
    });
    document.querySelector('#rzrbutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = cubeimgmx.multiplyWith(new TransformMatrix(c, s, 0,  -s, c, 0,  0, 0, 1));
        cubeimg.style.transform = `matrix3d(${cubeimgmx.toMatrix3dParam()})`;
    });
    movebuttonini.addEventListener('click', () => { moveidxgoal = -1; });
    for(let i = 0; i < 20; ++i)
        document.querySelector(`#movebutton${i}`).addEventListener('click', () => {
            if( i < moves.length ) {
                moveidxgoal = i;
                rewind = 0;
            }
        });
    rbluewall90.addEventListener('click', rotateBlueWall90);
    rbluewall180.addEventListener('click', rotateBlueWall180);
    rbluewall270.addEventListener('click', rotateBlueWall270);
    rredwall90.addEventListener('click', rotateRedWall90);
    rredwall180.addEventListener('click', rotateRedWall180);
    rredwall270.addEventListener('click', rotateRedWall270);
    ryellowwall90.addEventListener('click', rotateYellowWall90);
    ryellowwall180.addEventListener('click', rotateYellowWall180);
    ryellowwall270.addEventListener('click', rotateYellowWall270);
    rgreenwall90.addEventListener('click', rotateGreenWall90);
    rgreenwall180.addEventListener('click', rotateGreenWall180);
    rgreenwall270.addEventListener('click', rotateGreenWall270);
    rorangewall90.addEventListener('click', rotateOrangeWall90);
    rorangewall180.addEventListener('click', rotateOrangeWall180);
    rorangewall270.addEventListener('click', rotateOrangeWall270);
    rwhitewall90.addEventListener('click', rotateWhiteWall90);
    rwhitewall180.addEventListener('click', rotateWhiteWall180);
    rwhitewall270.addEventListener('click', rotateWhiteWall270);
    document.querySelectorAll('.csel').forEach( (elem) => { elem.addEventListener('click', selectColors); });
    editmodebtn.addEventListener('change', enterEditMode);
    resetbtn.addEventListener('click', doReset);
    applybtn.addEventListener('click', applyEdit);
    manipmodebtn.addEventListener('change', enterManipulateMode);
    solvebtn.addEventListener('click', searchSolution);
    let crestore = localStorage.getItem('cube');
    if( crestore ) {
        let cr = crestore.split(' ');
        let c = new cube(new cubecorners(+cr[0], +cr[1]), new cubeedges(BigInt(cr[2])));
        curcubemx = CubeTrMatrix.fromCube(c, curcubemx);
        curcubemx.applyToCubeImg();
    }
}
