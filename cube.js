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

let rotateMapFromExtFmt = new Map([
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

let rotateMapToExtFmt = new Map([...rotateMapFromExtFmt.entries()].map(([a,b]) => { return [b, a]; }));

class cubecorner_perms {
	/*unsigned*/ #perms;

	constructor(/*unsigned*/ corner0perm, /*unsigned*/ corner1perm, /*unsigned*/ corner2perm,
			/*unsigned*/ corner3perm, /*unsigned*/ corner4perm, /*unsigned*/ corner5perm,
			/*unsigned*/ corner6perm, /*unsigned*/ corner7perm)
    {
        if( corner0perm instanceof cubecorner_perms ) {
            this.#perms = corner0perm.#perms;
        }else{
            if( corner0perm == undefined )
                corner0perm = 0;
            if( corner1perm == undefined )
                corner1perm = 0;
            if( corner2perm == undefined )
                corner2perm = 0;
            if( corner3perm == undefined )
                corner3perm = 0;
            if( corner4perm == undefined )
                corner4perm = 0;
            if( corner5perm == undefined )
                corner5perm = 0;
            if( corner6perm == undefined )
                corner6perm = 0;
            if( corner7perm == undefined )
                corner7perm = 0;
            this.#perms = corner0perm | corner1perm << 3 | corner2perm << 6 |
                corner3perm << 9 | corner4perm << 12 | corner5perm << 15 |
                corner6perm << 18 | corner7perm << 21;
        }
    }

	setAt(/*unsigned*/ idx, /*unsigned char*/ perm) {
        this.#perms &= ~(7 << 3*idx);
        this.#perms |= perm << 3*idx;
	}
	getAt(/*unsigned*/ idx) { return this.#perms >>> 3*idx & 7; }
    equals(ccp) {
        return this.#perms == ccp.#perms;
    }
    isLessThan(ccp) {
        return this.#perms < ccp.#perms;
    }
    getPermVal() { return this.#perms; }
}

class cubecorner_orients {
	/*unsigned short*/ #orients;

	constructor(/*unsigned*/ corner0orient, /*unsigned*/ corner1orient,
			/*unsigned*/ corner2orient, /*unsigned*/ corner3orient,
			/*unsigned*/ corner4orient, /*unsigned*/ corner5orient,
			/*unsigned*/ corner6orient, /*unsigned*/ corner7orient)
    {
        if( corner0orient instanceof cubecorner_orients ) {
            this.#orients = corner0orient.#orients;
        }else{
            if( corner0orient == undefined )
                corner0orient = 0;
            if( corner1orient == undefined )
                corner1orient = 0;
            if( corner2orient == undefined )
                corner2orient = 0;
            if( corner3orient == undefined )
                corner3orient = 0;
            if( corner4orient == undefined )
                corner4orient = 0;
            if( corner5orient == undefined )
                corner5orient = 0;
            if( corner6orient == undefined )
                corner6orient = 0;
            if( corner7orient == undefined )
                corner7orient = 0;
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
	constructor(/*unsigned*/ corner0perm, /*unsigned*/ corner0orient,
			/*unsigned*/ corner1perm, /*unsigned*/ corner1orient,
			/*unsigned*/ corner2perm, /*unsigned*/ corner2orient,
			/*unsigned*/ corner3perm, /*unsigned*/ corner3orient,
			/*unsigned*/ corner4perm, /*unsigned*/ corner4orient,
			/*unsigned*/ corner5perm, /*unsigned*/ corner5orient,
			/*unsigned*/ corner6perm, /*unsigned*/ corner6orient,
			/*unsigned*/ corner7perm, /*unsigned*/ corner7orient)
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
	/*unsigned long*/ #edges;
	constructor(/*unsigned*/ edge0perm, /*unsigned*/ edge0orient,
			/*unsigned*/ edge1perm, /*unsigned*/ edge1orient,
			/*unsigned*/ edge2perm, /*unsigned*/ edge2orient,
			/*unsigned*/ edge3perm, /*unsigned*/ edge3orient,
			/*unsigned*/ edge4perm, /*unsigned*/ edge4orient,
			/*unsigned*/ edge5perm, /*unsigned*/ edge5orient,
			/*unsigned*/ edge6perm, /*unsigned*/ edge6orient,
			/*unsigned*/ edge7perm, /*unsigned*/ edge7orient,
			/*unsigned*/ edge8perm, /*unsigned*/ edge8orient,
			/*unsigned*/ edge9perm, /*unsigned*/ edge9orient,
			/*unsigned*/ edge10perm, /*unsigned*/ edge10orient,
			/*unsigned*/ edge11perm, /*unsigned*/ edge11orient)
    {
        if( edge0perm instanceof cubeedges ) {
            this.#edges = edge0perm.#edges;
        }else{
            if( edge0perm == undefined )
                edge0perm = 0;
            if( edge0orient == undefined )
                edge0orient = 0;
            if( edge1perm == undefined )
                edge1perm = 0;
            if( edge1orient == undefined )
                edge1orient = 0;
            if( edge2perm == undefined )
                edge2perm = 0;
            if( edge2orient == undefined )
                edge2orient = 0;
            if( edge3perm == undefined )
                edge3perm = 0;
            if( edge3orient == undefined )
                edge3orient = 0;
            if( edge4perm == undefined )
                edge4perm = 0;
            if( edge4orient == undefined )
                edge4orient = 0;
            if( edge5perm == undefined )
                edge5perm = 0;
            if( edge5orient == undefined )
                edge5orient = 0;
            if( edge6perm == undefined )
                edge6perm = 0;
            if( edge6orient == undefined )
                edge6orient = 0;
            if( edge7perm == undefined )
                edge7perm = 0;
            if( edge7orient == undefined )
                edge7orient = 0;
            if( edge8perm == undefined )
                edge8perm = 0;
            if( edge8orient == undefined )
                edge8orient = 0;
            if( edge9perm == undefined )
                edge9perm = 0;
            if( edge9orient == undefined )
                edge9orient = 0;
            if( edge10perm == undefined )
                edge10perm = 0;
            if( edge10orient == undefined )
                edge10orient = 0;
            if( edge11perm == undefined )
                edge11perm = 0;
            if( edge11orient == undefined )
                edge11orient = 0;
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

	set(/*unsigned*/ idx, /*unsigned char*/ perm, /*unsigned char*/ orient) {
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

function cubeReadFromFile(text)
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
            dolog('err', `too few letters\n`);
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
                    let movesExtFmtStr = mvs.toReversed().map((m) => {
                        return rotateMapToExtFmt.get(rotateDirReverse(m));
                    }).join('');
                    cubelistItemFill(movesExtFmtStr);
                    if( moves.length == 0 ) {
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
 
let cubeimgmx = [ [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ];

let cornersmx = [
    [
        [ [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
        [ [0, 0, -1, 0], [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
        [ [0, 1, 0, 0], [0, 0, -1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
        [ [0, 0, -1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
        [ [-1, 0, 0, 0], [0, 0, -1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
        [ [0, 0, -1, 0], [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
        [ [1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
        [ [0, 0, -1, 0], [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
        [ [0, -1, 0, 0], [0, 0, -1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
        [ [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
        [ [1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
        [ [0, 0, 1, 0], [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
        [ [0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
        [ [0, 0, 1, 0], [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
        [ [0, -1, 0, 0], [0, 0, 1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
        [ [0, 0, 1, 0], [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
        [ [-1, 0, 0, 0], [0, 0, 1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    ]
];

let edgesmx = [
    [
        [ [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
        [ [-1, 0, 0, 0], [0, 0, -1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, 1, 0, 0], [0, 0, -1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
        [ [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, -1, 0, 0], [0, 0, -1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
        [ [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    ],[
        [ [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
        [ [1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, 0, -1, 0], [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
        [ [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, 0, 1, 0], [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
        [ [0, 0, -1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, 0, 1, 0], [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
        [ [0, 0, -1, 0], [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, 0, -1, 0], [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
        [ [0, 0, 1, 0], [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
        [ [1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, -1, 0, 0], [0, 0, 1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
        [ [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    ],[
        [ [0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
        [ [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    ],[
        [ [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
        [ [-1, 0, 0, 0], [0, 0, 1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    ]
];

// corner/edge matrixes on particular walls
let cbluemx = [
    [ [ 1,  0,  0, 0], [ 0,  1,  0, 0], [ 0,  0,  1, 0], [0, 0, 0, 1] ],
    [ [ 1,  0,  0, 0], [ 0,  0, -1, 0], [ 0,  1,  0, 0], [0, 0, 0, 1] ],
    [ [-1,  0,  0, 0], [ 0, -1,  0, 0], [ 0,  0,  1, 0], [0, 0, 0, 1] ],
    [ [-1,  0,  0, 0], [ 0,  0, -1, 0], [ 0, -1,  0, 0], [0, 0, 0, 1] ],

    [ [ 0,  1,  0, 0], [-1,  0,  0, 0], [ 0,  0,  1, 0], [0, 0, 0, 1] ],
    [ [ 0,  1,  0, 0], [ 0,  0, -1, 0], [-1,  0,  0, 0], [0, 0, 0, 1] ],
    [ [ 0, -1,  0, 0], [ 1,  0,  0, 0], [ 0,  0,  1, 0], [0, 0, 0, 1] ],
    [ [ 0, -1,  0, 0], [ 0,  0, -1, 0], [ 1,  0,  0, 0], [0, 0, 0, 1] ],

    [ [ 0,  0, -1, 0], [ 0,  1,  0, 0], [ 1,  0,  0, 0], [0, 0, 0, 1] ],
    [ [ 0,  0, -1, 0], [ 1,  0,  0, 0], [ 0, -1,  0, 0], [0, 0, 0, 1] ],
    [ [ 0,  0, -1, 0], [ 0, -1,  0, 0], [-1,  0,  0, 0], [0, 0, 0, 1] ],
    [ [ 0,  0, -1, 0], [-1,  0,  0, 0], [ 0,  1,  0, 0], [0, 0, 0, 1] ],
];

let ebluemx = [
    [ [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 0, -1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],

    [ [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [0, 0, -1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],

    [ [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],

    [ [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [0, 0, -1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ]
];

let credmx = [
    [ [0, 0, -1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [0, 0, -1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 0, 1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 0, -1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
];

let eredmx = [
    [ [0, 0, 1, 0], [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [0, 0, -1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
];

let cyellowmx = [
    [ [1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [0, 0, -1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 0, -1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
];

let eyellowmx = [
    [ [1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 0, -1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
];

let cgreenmx = [
    [ [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 0, 1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [0, 0, 1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
];

let egreenmx = [
    [ [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [0, 0, 1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 0, 1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
];

let cwhitemx = [
    [ [1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [0, 0, -1, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [0, 0, 1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 0, 1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
];

let ewhitemx = [
    [ [1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [-1, 0, 0, 0], [0, 0, 1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
];

let corangemx = [
    [ [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [0, 0, -1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [0, 0, 1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
];

let eorangemx = [
    [ [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [0, 0, -1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ],
    [ [0, 0, -1, 0], [0, -1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, 0, 1, 0], [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1] ],
    [ [0, -1, 0, 0], [0, 0, 1, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ],
    [ [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ],
];

function multiplyMatrixes(a, b) {
    let res = [ [], [], [], [] ];
    for(let y = 0; y < 4; ++y) {
        for(let x = 0; x < 4; ++x) {
            let val = 0;
            for(let i = 0; i < 4; ++i)
                val += a[y][i] * b[i][x];
            res[y][x] = val;
        }
    }
    return res;
}

function multiplyMatrixesRnd(a, b) {
    let res = [ [], [], [], [] ];
    for(let y = 0; y < 4; ++y) {
        for(let x = 0; x < 4; ++x) {
            let val = 0;
            for(let i = 0; i < 4; ++i)
                val += a[y][i] * b[i][x];
            res[y][x] = Math.round(val);
        }
    }
    return res;
}

function matrixesEqual(a, b) {
    for(let y = 0; y < 4; ++y) {
        for(let x = 0; x < 4; ++x) {
            if( a[y][x] != b[y][x] )
                return false;
        }
    }
    return true;
}

function hasMatrix(toFind, searchArr) {
    for(let i = 0; i < searchArr.length; ++i) {
        let isEqual = true;
        for(let y = 0; y < 4 && isEqual; ++y) {
            for(let x = 0; x < 4 && isEqual; ++x) {
                if( toFind[y][x] != searchArr[i][y][x] )
                    isEqual = false;
            }
        }
        if( isEqual )
            return true;
    }
    return false;
}

function cubeMxEqual(cubemx1, cubemx2, includingMiddle) {
    for(let i = 0; i < 8; ++i)
        if( !matrixesEqual(cubemx1.corners[i], cubemx2.corners[i] ) )
            return false;
    for(let i = 0; i < 12; ++i)
        if( !matrixesEqual(cubemx1.edges[i], cubemx2.edges[i] ) )
            return false;
    if( includingMiddle ) {
        for(let i = 0; i < 6; ++i)
            if( !matrixesEqual(cubemx1.middles[i], cubemx2.middles[i] ) )
                return false;
    }
    return true;
}

function cubeMxCopy(cubemx) {
    let rescubemx = {
        corners: [],
        edges: [],
        middles: [],
    };
    for(let i = 0; i < 8; ++i)
        rescubemx.corners[i] = cubemx.corners[i];
    for(let i = 0; i < 12; ++i)
        rescubemx.edges[i] = cubemx.edges[i];
    for(let i = 0; i < 6; ++i)
        rescubemx.middles[i] = cubemx.middles[i];
    return rescubemx;
}

function rotateWall(cubemx, rotateDir) {
    let rescubemx = cubemx;
    function rotateWallInt(ccolormx, ecolormx, colornum, transform, wcolor) {
        rescubemx = cubeMxCopy(cubemx);
        for(let i = 0; i < 8; ++i) {
            if( hasMatrix(cubemx.corners[i], ccolormx) )
                rescubemx.corners[i] = multiplyMatrixesRnd(cubemx.corners[i], transform);
        }
        for(let i = 0; i < 12; ++i) {
            if( hasMatrix(cubemx.edges[i], ecolormx) )
                rescubemx.edges[i] = multiplyMatrixesRnd(cubemx.edges[i], transform);
        }
        rescubemx.middles[colornum] = multiplyMatrixesRnd(cubemx.middles[colornum], transform);
    }

    switch( rotateDir ) {
        case ORANGECW:
            rotateWallInt(corangemx, eorangemx, 1,
                [ [1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ], worange);
            break;
        case ORANGE180:
            rotateWallInt(corangemx, eorangemx, 1,
                [ [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ], worange);
            break;
        case ORANGECCW:
            rotateWallInt(corangemx, eorangemx, 1,
                [ [1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ], worange);
            break;
        case REDCW:
            rotateWallInt(credmx, eredmx, 3,
                [ [1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ], wred);
            break;
        case RED180:
            rotateWallInt(credmx, eredmx, 3,
                [ [1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ], wred);
            break;
        case REDCCW:
            rotateWallInt(credmx, eredmx, 3,
                [ [1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ], wred);
            break;
        case YELLOWCW:
            rotateWallInt(cyellowmx, eyellowmx, 0,
                [ [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ], wyellow);
            break;
        case YELLOW180:
            rotateWallInt(cyellowmx, eyellowmx, 0,
                [ [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ], wyellow);
            break;
        case YELLOWCCW:
            rotateWallInt(cyellowmx, eyellowmx, 0,
                [ [0, 0, -1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ], wyellow);
            break;
        case WHITECW:
            rotateWallInt(cwhitemx, ewhitemx, 5,
                [ [0, 0, -1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ], wwhite);
            break;
        case WHITE180:
            rotateWallInt(cwhitemx, ewhitemx, 5,
                [ [-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1] ], wwhite);
            break;
        case WHITECCW:
            rotateWallInt(cwhitemx, ewhitemx, 5,
                [ [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ], wwhite);
            break;
        case GREENCW:
            rotateWallInt(cgreenmx, egreenmx, 4,
                [ [0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ], wgreen);
            break;
        case GREEN180:
            rotateWallInt(cgreenmx, egreenmx, 4,
                [ [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ], wgreen);
            break;
        case GREENCCW:
            rotateWallInt(cgreenmx, egreenmx, 4,
                [ [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ], wgreen);
            break;
        case BLUECW:
            rotateWallInt(cbluemx, ebluemx, 2,
                [[0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]], wblue);
            break;
        case BLUE180:
            rotateWallInt(cbluemx, ebluemx, 2,
                [[-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]], wblue);
            break;
        case BLUECCW:
            rotateWallInt(cbluemx, ebluemx, 2,
                [[0, -1, 0, 0], [1, 0, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]], wblue);
            break;
    }
    return rescubemx;
}

let solvedcubemx = {
    corners: [
        cornersmx[0][0], cornersmx[1][0], cornersmx[2][0], cornersmx[3][0],
        cornersmx[4][0], cornersmx[5][0], cornersmx[6][0], cornersmx[7][0]
    ],
    edges: [
        edgesmx[0][0], edgesmx[1][0], edgesmx[2][0], edgesmx[3][0],
        edgesmx[4][0], edgesmx[5][0], edgesmx[6][0], edgesmx[7][0],
        edgesmx[8][0], edgesmx[9][0], edgesmx[10][0], edgesmx[11][0]
    ],
    middles: [
        [ [1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1] ], // yellow
        [ [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 1] ], // orange
        [ [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ], // blue
        [ [0, 0, -1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1] ], // red
        [ [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1] ], // green
        [ [1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1] ], // white
    ]
};

let curcubemx = cubeMxCopy(solvedcubemx);

function updateStylesTransforms()
{
    let corners = [ c1, c2, c3, c4, c5, c6, c7, c8 ];
    let edges = [ e1, e2, e3, e4, e5, e6, e7, e8, e9, ea, eb, ec ];
    let wcolors = [ wyellow, worange, wblue, wred, wgreen, wwhite ];
    for(let i = 0; i < 8; ++i)
        corners[i].style.transform = `matrix3d(${curcubemx.corners[i].join()})`;
    for(let i = 0; i < 12; ++i)
        edges[i].style.transform = `matrix3d(${curcubemx.edges[i].join()})`;
    for(let i = 0; i < 6; ++i)
        wcolors[i].style.transform = `matrix3d(${curcubemx.middles[i].join()})`;
}

function cubeToCubeMx(c) {
    let rescubemx = { corners: [], edges: [], middles: curcubemx.middles };
    for(let i = 0; i < 8; ++i) {
        let cno = c.cc.getPermAt(i);
        rescubemx.corners[cno] = cornersmx[i][c.cc.getOrientAt(i)];
    }
    for(let i = 0; i < 12; ++i) {
        let eno = c.ce.edgeN(i);
        rescubemx.edges[eno] = edgesmx[i][c.ce.edgeR(i)];
    }
    return rescubemx;
}

function cubePrint(c) {
    curcubemx = cubeToCubeMx(c);
    updateStylesTransforms();
}

function cubeimgToCube() {
    let cc = new cubecorners();
    for(let i = 0; i < 8; ++i) {
        for(let cno = 0; cno < 8; ++cno) {
            for(let orient = 0; orient < 3; ++orient) {
                if( matrixesEqual(curcubemx.corners[cno], cornersmx[i][orient]) ) {
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
                if( matrixesEqual(curcubemx.edges[eno], edgesmx[i][orient]) ) {
                    ce.set(i, eno, orient);
                }
            }
        }
    }
    return new cube(cc, ce);
}

const STYLEAPPLY_IMMEDIATE = 1;
const STYLEAPPLY_DEFER = 2;

function rotateCurWall(rotateDir, styleApply) {
    function rotateCurWallInt(rotateDir, styleApply) {
        curcubemx = rotateWall(curcubemx, rotateDir);
        if( styleApply === STYLEAPPLY_DEFER )
            setTimeout(updateStylesTransforms, 50);
        else
            updateStylesTransforms();
    }
    function rotateCurWallInt180(rotateDirCW) {
        if( styleApply === undefined ) {
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
    cubelistItemFind();
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

let fixedCornerColors = [ [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1] ];
let fixedEdgeColors = [ [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1] ];

function getAllowedCornerColors(ccref) {
    let allowedCornerColors = [ [[], [], []], [[], [], []], [[], [], []], [[], [], []], [[], [], []], [[], [], []], [[], [], []], [[], [], []] ];
    let ccpres = null, ccores = new cubecorner_orients();
    for(let perm = 0; perm < 40320; ++perm) {
        let ccp = cornersIdxToPerm(perm);
        let isPermAllowed = true;
        for(let i = 0; i < 8 && isPermAllowed; ++i) {
            let cno = ccp.getAt(i);
            let hasAllowedOrient = false;
            for(let orient = 0; orient < 3 && !hasAllowedOrient; ++orient) {
                let isAllowed = true;
                for(let j = 0; j < 3; ++j) {
                    let owall = (orient+j) % 3;
                    if( fixedCornerColors[i][j] != -1 && fixedCornerColors[i][j] != cubeCornerColors[cno][owall] )
                        isAllowed = false;
                }
                if( isAllowed )
                    hasAllowedOrient = true;
            }
            if( !hasAllowedOrient )
                isPermAllowed = false;
        }
        if( isPermAllowed ) {
            for(let i = 0; i < 8; ++i) {
                let cno = ccp.getAt(i);
                for(let orient = 0; orient < 3; ++orient) {
                    let isAllowed = true;
                    for(let j = 0; j < 3; ++j) {
                        let owall = (orient+j) % 3;
                        if( fixedCornerColors[i][j] != -1 && fixedCornerColors[i][j] != cubeCornerColors[cno][owall] )
                            isAllowed = false;
                    }
                    if( isAllowed ) {
                        if( ccpres == null )
                            ccores.setAt(i, orient);
                        for(let j = 0; j < 3; ++j) {
                            let owall = (orient+j) % 3;
                            if( !allowedCornerColors[i][j].includes(cubeCornerColors[cno][owall]) ) {
                                //console.log(`add allowed color ${colorClassNames[cubeCornerColors[cno][owall]]} for ${cornerIds[i]} ${cornerSquareClasses[j]}`);
                                allowedCornerColors[i][j].push(cubeCornerColors[cno][owall]);
                            }
                        }
                    }
                }
            }
            if( ccpres == null )
                ccpres = ccp;
        }
    }
    if( ccref != undefined )
        ccref[0] = new cubecorners(ccpres, ccores);
    return allowedCornerColors;
}

function *edgePermOnFixedColors(occupEdges, perm, filledCount) {
    while( filledCount < 12 && fixedEdgeColors[filledCount][0] == -1 && fixedEdgeColors[filledCount][1] == -1 )
        ++filledCount;
    if( filledCount == 12 ) {
        yield { perm: perm, occupEdges: occupEdges };
    }else{
        for(let eno = 0; eno < 12; ++eno) {
            if( !occupEdges[eno] ) {
                let hasMatchingOrient = false;
                for(let orient = 0; orient < 2 && !hasMatchingOrient; ++orient) {
                    let isMatching = true;
                    for(let j = 0; j < 2; ++j) {
                        let owall = (orient+j) % 2;
                        if( fixedEdgeColors[filledCount][j] != -1 && fixedEdgeColors[filledCount][j] != cubeEdgeColors[eno][owall] )
                            isMatching = false;
                    }
                    if( isMatching )
                        hasMatchingOrient = true;
                }
                if( hasMatchingOrient ) {
                    occupEdges[eno] = true;
                    perm[filledCount] = eno;
                    yield *edgePermOnFixedColors(occupEdges, perm, filledCount+1);
                    occupEdges[eno] = false;
                }
            }
        }
    }
}

function getAllowedEdgeColors(ceref) {
    let allowedEdgeColors = [ [[], []], [[], []], [[], []], [[], []], [[], []], [[], []], [[], []], [[], []], [[], []], [[], []], [[], []], [[], []] ];
    let ceres = null, ce = new cubeedges();
    for(let po of edgePermOnFixedColors(
        [false, false, false, false, false, false, false, false, false, false, false, false],
        [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1], 0))
    {
        //console.log(`occupEdges: ${po.occupEdges[0]}, ${po.occupEdges[1]}, ${po.occupEdges[2]}, ${po.occupEdges[3]}, ${po.occupEdges[4]}, ${po.occupEdges[5]}, ${po.occupEdges[6]}, ${po.occupEdges[7]}, ${po.occupEdges[8]}, ${po.occupEdges[9]}, ${po.occupEdges[10]}, ${po.occupEdges[11]}`);
        //console.log(`perm: ${po.perm[0]}, ${po.perm[1]}, ${po.perm[2]}, ${po.perm[3]}, ${po.perm[4]}, ${po.perm[5]}, ${po.perm[6]}, ${po.perm[7]}, ${po.perm[8]}, ${po.perm[9]}, ${po.perm[10]}, ${po.perm[11]}`);
        for(let i = 0; i < 12; ++i) {
            let eno = po.perm[i];
            if( eno >= 0 ) {
                if( ceres == null )
                    ce.setPermAt(i, eno);
                for(let orient = 0; orient < 2; ++orient) {
                    let isAllowed = true;
                    for(let j = 0; j < 2; ++j) {
                        let owall = (orient+j) % 2;
                        if( fixedEdgeColors[i][j] != -1 && fixedEdgeColors[i][j] != cubeEdgeColors[eno][owall] )
                            isAllowed = false;
                    }
                    if( isAllowed ) {
                        if( ceres == null )
                            ce.setOrientAt(i, orient);
                        for(let j = 0; j < 2; ++j) {
                            let owall = (orient+j) % 2;
                            if( !allowedEdgeColors[i][j].includes(cubeEdgeColors[eno][owall]) ) {
                                //console.log(`add allowed edge color ${colorClassNames[cubeEdgeColors[eno][owall]]} for ${edgeIds[i]} ${edgeSquareClasses[j]}`);
                                allowedEdgeColors[i][j].push(cubeEdgeColors[eno][owall]);
                            }
                        }
                    }
                }
            }else{
                for(let eno = 0; eno < 12; ++eno) {
                    if( !po.occupEdges[eno] ) {
                        for(let orient = 0; orient < 2; ++orient) {
                            for(let j = 0; j < 2; ++j) {
                                let owall = (orient+j) % 2;
                                if( !allowedEdgeColors[i][j].includes(cubeEdgeColors[eno][owall]) ) {
                                    //console.log(`add allowed edge color ${colorClassNames[cubeEdgeColors[eno][owall]]} for ${edgeIds[i]} ${edgeSquareClasses[j]}`);
                                    allowedEdgeColors[i][j].push(cubeEdgeColors[eno][owall]);
                                }
                            }
                        }
                    }
                }
            }
        }
        if( ceres == null ) {
            ceres = ce;
        }
    }
    if( ceref != undefined )
        ceref[0] = ceres;
    return allowedEdgeColors;
}

function selectColors(ev) {
    const colorClassNames = [ 'cyellow', 'corange', 'cblue', 'cred', 'cgreen', 'cwhite' ];
    const cornerIds = ['c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7', 'c8'];
    const edgeIds = [ 'e1', 'e2', 'e3', 'e4', 'e5', 'e6', 'e7', 'e8', 'e9', 'ea', 'eb', 'ec' ];
    const cornerSquareClasses = [ 'csq1', 'csq2', 'csq3' ];
    const edgeSquareClasses = [ 'esq1', 'esq2' ];
    let thisElem = ev.target;
    let wallColor = -1;
    if( thisElem.parentElement.parentElement.classList.contains('corner') ) {
        let cornerNo = cornerIds.indexOf(thisElem.parentElement.parentElement.id);
        let squareNo = 0;
        while( !thisElem.parentElement.classList.contains(cornerSquareClasses[squareNo]) )
            ++squareNo;
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
        if( fixedEdgeColors[edgeNo][squareNo] == -1 ) {
            wallColor = 0;
            while( !thisElem.classList.contains(colorClassNames[wallColor]) )
                ++wallColor;
        }
        fixedEdgeColors[edgeNo][squareNo] = wallColor;
    }
    document.querySelectorAll('.csel').forEach( (el) => { el.classList.add('cnone'); } );
    let isFilled = true;
    let allowedCornerColors = getAllowedCornerColors();
    for(let cno = 0; cno < 8; ++cno) {
        for(let sqno = 0; sqno < 3; ++sqno) {
            for( let colorNo = 0; colorNo < allowedCornerColors[cno][sqno].length; ++colorNo ) {
                let cselElem = document.querySelector(`#${cornerIds[cno]} > .${cornerSquareClasses[sqno]} > .${colorClassNames[allowedCornerColors[cno][sqno][colorNo]]}`);
                cselElem.classList.remove('cnone');
            }
            if( allowedCornerColors[cno][sqno].length > 1 )
                isFilled = false;
        }
    }
    let allowedEdgeColors = getAllowedEdgeColors();
    for(let eno = 0; eno < 12; ++eno) {
        for(let sqno = 0; sqno < 2; ++sqno) {
            for( let colorNo = 0; colorNo < allowedEdgeColors[eno][sqno].length; ++colorNo ) {
                let selector = `#${edgeIds[eno]} > .${edgeSquareClasses[sqno]} > .${colorClassNames[allowedEdgeColors[eno][sqno][colorNo]]}`;
                let cselElem = document.querySelector(selector);
                cselElem.classList.remove('cnone');
            }
            if( allowedEdgeColors[eno][sqno].length > 1 )
                isFilled = false;
        }
    }
    applybtn.disabled = ! isFilled;
}

function areCubeColorsFilled() {
    let allowedCornerColors = getAllowedCornerColors();
    for(let cno = 0; cno < 8; ++cno) {
        for(let sqno = 0; sqno < 3; ++sqno) {
            if( allowedCornerColors[cno][sqno].length > 1 )
                return false;
        }
    }
    let allowedEdgeColors = getAllowedEdgeColors();
    for(let eno = 0; eno < 12; ++eno) {
        for(let sqno = 0; sqno < 2; ++sqno) {
            if( allowedEdgeColors[eno][sqno].length > 1 )
                return false;
        }
    }
    return true;
}

let cmanipulate = csolved;

function enterEditMode() {
    cmanipulate = cubeimgToCube();
    cubePrint(csolved);
    cubeimg.classList.add('editmode');
    document.querySelectorAll('.editmodeonly').forEach( (el) => { el.disabled = false; });
    document.querySelectorAll('.manipmodeonly').forEach( (el) => { el.disabled = true; });
    applybtn.disabled = ! areCubeColorsFilled();
}

function doReset() {
    if( cubeimg.classList.contains('editmode') ) {
        fixedCornerColors = [ [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1] ];
        fixedEdgeColors = [ [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1] ];
        document.querySelectorAll('.csel').forEach( (el) => { el.classList.remove('cnone'); } );
    }else{
        cubePrint(csolved);
    }
}

function enterManipulateMode() {
    cubeimg.classList.remove('editmode');
    cubePrint(cmanipulate);
    document.querySelectorAll('.editmodeonly').forEach( (el) => { el.disabled = true; });
    document.querySelectorAll('.manipmodeonly').forEach( (el) => { el.disabled = false; });
    applybtn.disabled = true;
}

function applyEdit() {
    let ccref = [], ceref = [];
    getAllowedCornerColors(ccref);
    getAllowedEdgeColors(ceref);
	cmanipulate = new cube(ccref[0], ceref[0]);
    manipmodebtn.checked = true;
    enterManipulateMode();
}

function searchSolution() {
    document.querySelectorAll('.log').forEach((e) => {e.textContent = ''});
    let c = cubeimgToCube();
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
        let c = cubeReadFromFile(val);
        cubePrint(c);
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
        let c = cubeimgToCube();
        let saveText = cubeToSaveText(c);
        await fout.write(saveText);
        await fout.close();
        localStorage.setItem('fileseq', +fileSeq+1);
    }, function() {});
}

function getCubeMxScrambled(s) {
    let rescubemx = cubeMxCopy(solvedcubemx);
    for(let i = 0; i+1 < s.length; i+=2) {
        let mappedVal = rotateMapFromExtFmt.get(s.substring(i, i+2));
        if( mappedVal != undefined )
            rescubemx = rotateWall(rescubemx, mappedVal)
        else
            dolog('err', `Unkown move: ${s.substring(i, i+2)}`);
    }
    return rescubemx;
}

function cubeMxFromStr(s) {
    let rescubemx;
    if( /[Yy]/.test(s) ) {
        let c = cubeReadFromFile(s);
        if( c )
            rescubemx = cubeToCubeMx(c);
    }else{
        rescubemx = getCubeMxScrambled(s);
    }
    return rescubemx;
}

let cubefill = '', cubelistval = '', cubelistitemno = -1;

function cubelistItemFind() {
    if( cubefill.length > 0 && cubeMxEqual(curcubemx, cubeMxFromStr(cubefill), false) ) {
        cubelistitemtext.textContent = cubefill;
        cubelistitemno = -1;
        cubelistiteminsert.disabled = false;
        return;
    }
    cubelistiteminsert.disabled = true;
    if( cubelistval.length > 0 ) {
        let itemsarr = cubelistval.split('\n');
        let curItem = 0;
        while( curItem < itemsarr.length ) {
            if( cubeMxEqual(curcubemx, cubeMxFromStr(itemsarr[curItem]), false) )
                break;
            ++curItem;
        }
        if( curItem < itemsarr.length ) {
            cubelistitemtext.textContent = curItem + ' ' + itemsarr[curItem];
            cubelistitemno = curItem;
        }else{
            cubelistitemtext.textContent = '';
            cubelistitemno = -1;
        }
    }
}

function cubelistItemFill(s) {
    let itemsarr = cubelistval.split('\n');
    if( ! itemsarr.includes(s) ) {
        cubefill = s;
    }
}

function cubelistLoad() {
    cubelist.showModal();
}

function cubelistSave() {
    cubelistval = cubelisttext.value.trim();
    localStorage.setItem('cubelist', cubelistval);
    cubelist.close();
    cubelistItemFind();
}

function cubelistCancel() {
    cubelisttext.value = cubelistval;
    cubelist.close();
}

function cubelistItemNext() {
    if( cubelistval.length > 0 ) {
        let itemsarr = cubelistval.split('\n');
        let nextItem = cubelistitemno+1;
        if( nextItem < itemsarr.length ) {
            cubelistitemtext.textContent = nextItem + ' ' + itemsarr[nextItem];
            curcubemx = cubeMxFromStr(itemsarr[nextItem]);
            updateStylesTransforms();
            cubelistitemno = nextItem;
        }
    }
}

function cubelistItemPrev() {
    if( cubelistval.length > 0 ) {
        let itemsarr = cubelistval.split('\n');
        let prevItem = cubelistitemno-1;
        if( prevItem >= 0 ) {
            cubelistitemtext.textContent = prevItem + ' ' + itemsarr[prevItem];
            curcubemx = cubeMxFromStr(itemsarr[prevItem]);
            updateStylesTransforms();
            cubelistitemno = prevItem;
        }
    }
}

function cubelistItemInsert() {
    if( cubefill ) {
        cubelistval = cubefill + '\n' + cubelistval;
        cubelisttext.value = cubelistval;
        cubefill = '';
        localStorage.setItem('cubelist', cubelistval);
        cubelistItemFind();
    }
}

onload = () => {
    if( window['showOpenFilePicker'] == undefined ) {
        loadsavebtns.style.display = 'none';
        document.querySelector('#cubefile').addEventListener('change', (ev) => {
            let file = ev.target.files[0];
            file.text().then((val) => {
                let c = cubeReadFromFile(val);
                cubePrint(c);
            });
        });
    }else{
        loadonlybtn.style.display = 'none';
        loadbtn.addEventListener('click', loadFromFile);
        savebtn.addEventListener('click', saveToFile);
    }
    cubelistloadbtn.addEventListener('click', cubelistLoad);
    cubelistsavebtn.addEventListener('click', cubelistSave);
    cubelistcancelbtn.addEventListener('click', cubelistCancel);
    cubelistitemprev.addEventListener('click', cubelistItemPrev);
    cubelistitemnext.addEventListener('click', cubelistItemNext);
    cubelistiteminsert.addEventListener('click', cubelistItemInsert);
    document.querySelector('#rxubutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = multiplyMatrixes(cubeimgmx, [[1, 0, 0, 0], [0, c, s, 0], [0, -s, c, 0], [0, 0, 0, 1]]);
        cubeimg.style.transform = `matrix3d(${cubeimgmx.join()})`;
    });
    document.querySelector('#rxdbutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = multiplyMatrixes(cubeimgmx, [[1, 0, 0, 0], [0, c, -s, 0], [0, s, c, 0], [0, 0, 0, 1]]);
        cubeimg.style.transform = `matrix3d(${cubeimgmx.join()})`;
    });
    document.querySelector('#rylbutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = multiplyMatrixes(cubeimgmx, [[c, 0, s, 0], [0, 1, 0, 0], [-s, 0, c, 0], [0, 0, 0, 1]]);
        cubeimg.style.transform = `matrix3d(${cubeimgmx.join()})`;
    });
    document.querySelector('#ryrbutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = multiplyMatrixes(cubeimgmx, [[c, 0, -s, 0], [0, 1, 0, 0], [s, 0, c, 0], [0, 0, 0, 1]]);
        cubeimg.style.transform = `matrix3d(${cubeimgmx.join()})`;
    });
    document.querySelector('#rzlbutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = multiplyMatrixes(cubeimgmx, [[c, -s, 0, 0], [s, c, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]);
        cubeimg.style.transform = `matrix3d(${cubeimgmx.join()})`;
    });
    document.querySelector('#rzrbutton').addEventListener('click', (ev) => {
        let angle = document.querySelector('#angle').value * Math.PI / 180;
        let c = Math.cos(angle);
        let s = Math.sin(angle);
        cubeimgmx = multiplyMatrixes(cubeimgmx, [[c, s, 0, 0], [-s, c, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]);
        cubeimg.style.transform = `matrix3d(${cubeimgmx.join()})`;
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
        cubePrint(c);
    }
    crestore = localStorage.getItem('cubelist');
    if( crestore ) {
        cubelistval = crestore;
        cubelisttext.value = cubelistval;
        cubelistItemFind();
    }
}
