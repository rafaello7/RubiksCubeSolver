let startTime, lastTime;

function dolog(section, text) {
    let el = document.querySelector(`#${section}`);
    if( section == 'depth' )
        document.querySelector('#progress').textContent = '';
    if( startTime && ['depth', 'progress', 'solutions'].includes(section) ) {
        let curTime = Date.now();
        if( curTime - lastTime >= 100 ) {
            let dig1 = curTime - lastTime < 10000 ? 1 : 0;
            let dig2 = curTime - startTime < 10000 ? 1 : 0;
            text = `${((curTime-lastTime)/1000).toFixed(dig1)}/${((curTime-startTime)/1000).toFixed(dig2)}s ${text}`;
        }
        if( section == 'depth' )
            lastTime = curTime;
    }
    if( ['progress'].includes(section) ) {
        el.textContent = text;
    }else{
        let d = document.createElement('div');
        d.textContent =  text;
        document.querySelector(`#${section}`).append(d);
        if( section == 'depth' || section == 'solutions' )
            d.scrollIntoView(false);
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
  [ CBLUE,   CORANGE, CYELLOW ], [ CBLUE,  CYELLOW, CRED    ],
  [ CBLUE,   CWHITE,  CORANGE ], [ CBLUE,  CRED,    CWHITE  ],
  [ CGREEN,  CYELLOW, CORANGE ], [ CGREEN, CRED,    CYELLOW ],
  [ CGREEN,  CORANGE, CWHITE  ], [ CGREEN, CWHITE,  CRED    ]
];

const cubeEdgeColors = [
  [ CBLUE,   CYELLOW ], [ CORANGE, CBLUE   ], [ CRED,    CBLUE   ],
  [ CBLUE,   CWHITE  ], [ CYELLOW, CORANGE ], [ CYELLOW, CRED    ],
  [ CWHITE,  CORANGE ], [ CWHITE,  CRED    ], [ CGREEN,  CYELLOW ],
  [ CORANGE, CGREEN  ], [ CRED,    CGREEN  ], [ CGREEN,  CWHITE  ]
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

    toString() {
        return `TransformMatrix(${this.#x[0]}, ${this.#x[1]}, ${this.#x[2]}, ${this.#y[0]}, ${this.#y[1]}, ${this.#y[2]}, ${this.#z[0]}, ${this.#z[1]}, ${this.#z[2]})`;
    }

    toMatrix3dParam() {
        return this.#x.join() + ',0,' + this.#y.join() + ',0,' +
            this.#z.join() + ',0,0,0,0,1';
    }
}

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

// Maps middle square rotation to a transformation matrix
let middlesmx = [
    [   // yellow - applying TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0)
        new TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0),
        new TransformMatrix(0, 0, 1, -1, 0, 0, 0, -1, 0),
        new TransformMatrix(-1, 0, 0, 0, 0, -1, 0, -1, 0),
        new TransformMatrix(0, 0, -1, 1, 0, 0, 0, -1, 0)
    ],[ // orange - applying TransformMatrix(1, 0, 0,  0, 0, -1,  0, 1, 0)
        new TransformMatrix(0, 0, 1,  0, 1, 0,  -1, 0, 0),
        new TransformMatrix(0, 1, 0,  0, 0, -1,  -1, 0, 0),
        new TransformMatrix(0, 0, -1,  0, -1, 0,  -1, 0, 0),
        new TransformMatrix(0, -1, 0,  0, 0, 1,  -1, 0, 0),
    ],[ // blue TransformMatrix(0, 1, 0,  -1, 0, 0,  0, 0, 1)
        new TransformMatrix(1, 0, 0, 0, 1, 0, 0, 0, 1),
        new TransformMatrix(0, 1, 0, -1, 0, 0, 0, 0, 1),
        new TransformMatrix(-1, 0, 0, 0, -1, 0, 0, 0, 1),
        new TransformMatrix(0, -1, 0, 1, 0, 0, 0, 0, 1)
    ],[ // red TransformMatrix(1, 0, 0,  0, 0, 1,  0, -1, 0)
        new TransformMatrix(0, 0, -1, 0, 1, 0, 1, 0, 0),
        new TransformMatrix(0, 1, 0, 0, 0, 1, 1, 0, 0),
        new TransformMatrix(0, 0, 1, 0, -1, 0, 1, 0, 0),
        new TransformMatrix(0, -1, 0, 0, 0, -1, 1, 0, 0)
    ],[ // green TransformMatrix(0, -1, 0,  1, 0, 0,  0, 0, 1)
        new TransformMatrix(1, 0, 0, 0, 1, 0, 0, 0, 1),
        new TransformMatrix(0, -1, 0, 1, 0, 0, 0, 0, 1),
        new TransformMatrix(-1, 0, 0, 0, -1, 0, 0, 0, 1),
        new TransformMatrix(0, 1, 0, -1, 0, 0, 0, 0, 1)
    ],[ // white TransformMatrix(0, 0, -1,  0, 1, 0,  1, 0, 0)
        new TransformMatrix(1, 0, 0, 0, 0, -1, 0, 1, 0),
        new TransformMatrix(0, 0, -1, -1, 0, 0, 0, 1, 0),
        new TransformMatrix(-1, 0, 0, 0, 0, 1, 0, 1, 0),
        new TransformMatrix(0, 0, 1, 1, 0, 0, 0, 1, 0)
    ]
];

class cubecorners {
    #cperms;
    #corients;
	constructor(corner0perm = 0, corner0orient = 0, corner1perm = 0, corner1orient = 0,
			corner2perm = 0, corner2orient = 0, corner3perm = 0, corner3orient = 0,
			corner4perm = 0, corner4orient = 0, corner5perm = 0, corner5orient = 0,
			corner6perm = 0, corner6orient = 0, corner7perm = 0, corner7orient = 0)
    {
        this.#cperms = corner0perm | corner1perm << 3 | corner2perm << 6 |
            corner3perm << 9 | corner4perm << 12 | corner5perm << 15 |
            corner6perm << 18 | corner7perm << 21;
        this.#corients = corner0orient | corner1orient << 2 | corner2orient << 4 |
            corner3orient << 6 | corner4orient << 8 | corner5orient << 10 |
            corner6orient << 12 | corner7orient << 14;
    }

	getPermAt(idx) { return this.#cperms >>> 3*idx & 7; }
	getOrientAt(idx) { return this.#corients >> 2*idx & 3; }
	setPermAt(idx, perm) {
        this.#cperms &= ~(7 << 3*idx);
        this.#cperms |= perm << 3*idx;
	}
	setOrientAt(idx, orient) {
		this.#corients &= ~(3 << 2*idx);
		this.#corients |= orient << 2*idx;
	}

    static compose(cc1, cc2) {
        let res = new cubecorners();
        for(let i = 0; i < 8; ++i) {
            let corner2perm = cc2.#cperms >> 3 * i & 7;
            let corner2orient = cc2.#corients >> 2 * i & 3;
            let corner1perm = cc1.#cperms >> 3 * corner2perm & 7;
            let corner1orient = cc1.#corients >> 2 * corner2perm & 3;
            res.#cperms |= corner1perm << 3*i;
            res.#corients |= (corner1orient+corner2orient)%3 << 2*i;
        }
        return res;
    }

	getPerms() { return this.#cperms; }
	getOrients() { return this.#corients; }
	equals(cc) {
		return this.#cperms == cc.#cperms && this.#corients == cc.#corients;
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

	edgeN(idx) { return Number(this.#edges >> BigInt(5 * idx) & 0xfn); }
	edgeR(idx) { return Number(this.#edges >> BigInt(5*idx+4) & 1n); }
	equals(ce) { return this.#edges == ce.#edges; }
    get() { return this.#edges; }
};

class cubemiddles {
    #quarterturns;

    constructor(mid0 = 0, mid1 = 0, mid2 = 0, mid3 = 0, mid4 = 0, mid5 = 0) {
        this.#quarterturns = mid0 | mid1 << 2 | mid2 << 4 |
            mid3 << 6 | mid4 << 8 | mid5 << 10;
    }

    getAt(idx) { return this.#quarterturns >> 2 * idx & 3; }
    setAt(idx, qturn) {
		this.#quarterturns &= ~(3 << 2*idx);
		this.#quarterturns |= qturn << 2*idx;
    }

    static compose(cm1, cm2) {
        let res = new cubemiddles();
        for(let i = 0; i < 6; ++i) {
            let cm1mid = cm1.#quarterturns >> 2 * i & 3;
            let cm2mid = cm2.#quarterturns >> 2 * i & 3;
            res.#quarterturns |= ((cm1mid + cm2mid) % 4) << 2*i;
        }
        return res;
    }
}

class cube {
	/*cubecorners*/ cc;
	/*cubeedges*/ ce;
    /*cubemiddles*/ cm;

    constructor(cc = new cubecorners(), ce = new cubeedges(),
        cm = new cubemiddles())
    {
        this.cc = cc;
        this.ce = ce;
        this.cm = cm;
    }

    static compose(c1, c2) {
        return new cube(
            cubecorners.compose(c1.cc, c2.cc),
            cubeedges.compose(c1.ce, c2.ce),
            cubemiddles.compose(c1.cm, c2.cm)
        );
    }

    equals(c) {
        return this.cc.equals(c.cc) && this.ce.equals(c.ce);
    }

    static fromScrambleStr(s) {
        const rotateMapFromExtFmt = new Map([
            [ 'L1', BLUECW   ],
            [ 'L2', BLUE180  ],
            [ 'L3', BLUECCW  ],
            [ 'R1', GREENCW  ],
            [ 'R2', GREEN180 ],
            [ 'R3', GREENCCW ],
            [ 'U1', YELLOWCW ],
            [ 'U2', YELLOW180],
            [ 'U3', YELLOWCCW],
            [ 'D1', WHITECW  ],
            [ 'D2', WHITE180 ],
            [ 'D3', WHITECCW ],
            [ 'B1', ORANGECW ],
            [ 'B2', ORANGE180],
            [ 'B3', ORANGECCW],
            [ 'F1', REDCW    ],
            [ 'F2', RED180   ],
            [ 'F3', REDCCW   ],
        ]);

        let rescube = csolved;
        let i = 0;
        while(i+1 < s.length) {
            if( /\S/.test(s[i]) ) {
                let mappedVal = rotateMapFromExtFmt.get(s.substring(i, i+2));
                if( mappedVal != undefined )
                    rescube = cube.compose(rescube, crotated[mappedVal])
                else
                    dolog('err', `Unkown move: ${s.substring(i, i+2)}`);
                i+=2;
            }else
                ++i;
        }
        return rescube;
    }

    static fromScrambleStrReversed(s) {
        const rotateMapFromExtFmt = new Map([
            [ 'L3', BLUECW   ],
            [ 'L2', BLUE180  ],
            [ 'L1', BLUECCW  ],
            [ 'R3', GREENCW  ],
            [ 'R2', GREEN180 ],
            [ 'R1', GREENCCW ],
            [ 'U3', YELLOWCW ],
            [ 'U2', YELLOW180],
            [ 'U1', YELLOWCCW],
            [ 'D3', WHITECW  ],
            [ 'D2', WHITE180 ],
            [ 'D1', WHITECCW ],
            [ 'B3', ORANGECW ],
            [ 'B2', ORANGE180],
            [ 'B1', ORANGECCW],
            [ 'F3', REDCW    ],
            [ 'F2', RED180   ],
            [ 'F1', REDCCW   ],
        ]);

        let rescube = csolved;
        let i = s.length - 1;
        while(i > 0) {
            if( /\S/.test(s[i]) ) {
                let mappedVal = rotateMapFromExtFmt.get(s.substring(i-1, i+1));
                if( mappedVal != undefined )
                    rescube = cube.compose(rescube, crotated[mappedVal])
                else
                    dolog('err', `Unkown move: ${s.substring(i-1, i+1)}`);
                i-=2;
            }else
                --i;
        }
        return rescube;
    }

    applyToCubeImg() {
        let corners = [ c1, c2, c3, c4, c5, c6, c7, c8 ];
        let edges = [ e1, e2, e3, e4, e5, e6, e7, e8, e9, ea, eb, ec ];
        let wcolors = [ wyellow, worange, wblue, wred, wgreen, wwhite ];
        for(let i = 0; i < 8; ++i) {
            let cno = this.cc.getPermAt(i);
            let squaremx = cornersmx[i][this.cc.getOrientAt(i)];
            corners[cno].style.transform = `matrix3d(${squaremx.toMatrix3dParam()})`;
        }

        for(let i = 0; i < 12; ++i) {
            let eno = this.ce.edgeN(i);
            let squaremx = edgesmx[i][this.ce.edgeR(i)];
            edges[eno].style.transform = `matrix3d(${squaremx.toMatrix3dParam()})`;
        }
        for(let i = 0; i < 6; ++i) {
            let squaremx = middlesmx[i][this.cm.getAt(i)];
            wcolors[i].style.transform = `matrix3d(${squaremx.toMatrix3dParam()})`;
        }
    }

};

const csolved = new cube(
    new cubecorners(
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
	),
    new cubemiddles(
        0,
        0,
        0,
        0,
        0,
        0
    )
);

const crotated = [
	new cube( // ORANGECW
		new cubecorners(
			4, 1,
			1, 0,
			0, 2,
			3, 0,
			6, 2,
			5, 0,
			2, 1,
			7, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            1,
            0,
            0,
            0,
            0
        )
    ),
    new cube( // ORANGE180
		new cubecorners(
			6, 0,
			1, 0,
			4, 0,
			3, 0,
			2, 0,
			5, 0,
			0, 0,
			7, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            2,
            0,
            0,
            0,
            0
		)
    ),
    new cube(// ORANGECCW
		new cubecorners(
			2, 1,
			1, 0,
			6, 2,
			3, 0,
			0, 2,
			5, 0,
			4, 1,
			7, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            3,
            0,
            0,
            0,
            0
		)
    ),
    new cube(// REDCW
		new cubecorners(
			0, 0,
			3, 2,
			2, 0,
			7, 1,
			4, 0,
			1, 1,
			6, 0,
			5, 2
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            0,
            0,
            1,
            0,
            0
		)
    ),
    new cube(// RED180
		new cubecorners(
			0, 0,
			7, 0,
			2, 0,
			5, 0,
			4, 0,
			3, 0,
			6, 0,
			1, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            0,
            0,
            2,
            0,
            0
		)
    ),
    new cube(// REDCCW
		new cubecorners(
			0, 0,
			5, 2,
			2, 0,
			1, 1,
			4, 0,
			7, 1,
			6, 0,
			3, 2
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            0,
            0,
            3,
            0,
            0
		)
    ),
    new cube(// YELLOWCW
		new cubecorners(
			1, 2,
			5, 1,
			2, 0,
			3, 0,
			0, 1,
			4, 2,
			6, 0,
			7, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            1,
            0,
            0,
            0,
            0,
            0
		)
    ),
    new cube(// YELLOW180
		new cubecorners(
			5, 0,
			4, 0,
			2, 0,
			3, 0,
			1, 0,
			0, 0,
			6, 0,
			7, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            2,
            0,
            0,
            0,
            0,
            0
		)
    ),
    new cube(// YELLOWCCW
		new cubecorners(
			4, 2,
			0, 1,
			2, 0,
			3, 0,
			5, 1,
			1, 2,
			6, 0,
			7, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            3,
            0,
            0,
            0,
            0,
            0
		)
    ),
    new cube(// WHITECW
		new cubecorners(
			0, 0,
			1, 0,
			6, 1,
			2, 2,
			4, 0,
			5, 0,
			7, 2,
			3, 1
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            0,
            0,
            0,
            0,
            1
		)
    ),
    new cube(// WHITE180
		new cubecorners(
			0, 0,
			1, 0,
			7, 0,
			6, 0,
			4, 0,
			5, 0,
			3, 0,
			2, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            0,
            0,
            0,
            0,
            2
		)
    ),
    new cube(// WHITECCW
		new cubecorners(
			0, 0,
			1, 0,
			3, 1,
			7, 2,
			4, 0,
			5, 0,
			2, 2,
			6, 1
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            0,
            0,
            0,
            0,
            3
		)
    ),
    new cube(// GREENCW
		new cubecorners(
			0, 0,
			1, 0,
			2, 0,
			3, 0,
			5, 0,
			7, 0,
			4, 0,
			6, 0
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
			10, 1,
			 8, 1,
			11, 1,
			 9, 1
		),
        new cubemiddles(
            0,
            0,
            0,
            0,
            1,
            0
		)
    ),
    new cube(// GREEN180
		new cubecorners(
			0, 0,
			1, 0,
			2, 0,
			3, 0,
			7, 0,
			6, 0,
			5, 0,
			4, 0
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
			11, 0,
			10, 0,
			 9, 0,
			 8, 0
		),
        new cubemiddles(
            0,
            0,
            0,
            0,
            2,
            0
		)
    ),
    new cube(// GREENCCW
		new cubecorners(
			0, 0,
			1, 0,
			2, 0,
			3, 0,
			6, 0,
			4, 0,
			7, 0,
			5, 0
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
			 9, 1,
			11, 1,
			 8, 1,
			10, 1
		),
        new cubemiddles(
            0,
            0,
            0,
            0,
            3,
            0
		)
    ),
    new cube(// BLUECW
		new cubecorners(
			2, 0,
			0, 0,
			3, 0,
			1, 0,
			4, 0,
			5, 0,
			6, 0,
			7, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            0,
            1,
            0,
            0,
            0
		)
    ),
    new cube(// BLUE180
		new cubecorners(
			3, 0,
			2, 0,
			1, 0,
			0, 0,
			4, 0,
			5, 0,
			6, 0,
			7, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            0,
            2,
            0,
            0,
            0
		)
    ),
    new cube(// BLUECCW
		new cubecorners(
			1, 0,
			3, 0,
			0, 0,
			2, 0,
			4, 0,
			5, 0,
			6, 0,
			7, 0
		),
		new cubeedges(
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
		),
        new cubemiddles(
            0,
            0,
            3,
            0,
            0,
            0
		)
	)
];

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

/*
struct elemLoc {
	int wall;
    int row;
	int col;
};
*/

function mapColorsOnSqaresFromExt(ext) {
    // external format:
    //     U                Yellow
    //   L F R B   =   Blue   Red   Green Orange
    //     D                 White
    // sequence:
    //    yellow   green    red      white    blue     orange
    //  UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB
    let squaremap = [
         2,  5,  8,  1,  4,  7,  0,  3,  6, // upper -> yellow
        45, 46, 47, 48, 49, 50, 51, 52, 53, // back -> orange
        36, 37, 38, 39, 40, 41, 42, 43, 44, // left -> blue
        18, 19, 20, 21, 22, 23, 24, 25, 26, // front -> red
         9, 10, 11, 12, 13, 14, 15, 16, 17, // right -> green
        33, 30, 27, 34, 31, 28, 35, 32, 29  // down -> white
    ];
    let colormap = new Map([
        ['U', 'Y'], ['R', 'G'], ['F', 'R'],
        ['D', 'W'], ['L', 'B'], ['B', 'O']
    ]);
    let res = '';
    for(let i = 0; i < 54; ++i)
        res += colormap.get(ext[squaremap[i]]);
    return res;
}

function cubeFromColorsOnSquares(text)
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

function cubeFromString(s) {
    document.querySelectorAll('.log').forEach((e) => {e.textContent = ''});
    let rescube = null;
    if( /^[YOBRGW]{4}Y[YOBRGW]{8}O[YOBRGW]{8}B[YOBRGW]{8}R[YOBRGW]{8}G[YOBRGW]{8}W[YOBRGW]{4}$/.test(s) ) {
        rescube = cubeFromColorsOnSquares(s);
    }else if( /^[URFDLB]{4}U[URFDLB]{8}R[URFDLB]{8}F[URFDLB]{8}D[URFDLB]{8}L[URFDLB]{8}B[URFDLB]{4}$/.test(s) ) {
        let fromExt = mapColorsOnSqaresFromExt(s);
        rescube = cubeFromColorsOnSquares(fromExt);
    }else if( /^([FBLRUD][123]|\s)*$/.test(s) ) {
        rescube = cube.fromScrambleStr(s);
    }else{
        dolog('err', 'format not recognized');
    }
    return rescube;
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

async function searchMoves(c, qparammode) {
    try {
        startTime = lastTime = Date.now();
        moves = [];
        moveidx = moveidxgoal = -1;
        document.querySelectorAll(`#movebuttons > button`).forEach( (btn) => { btn.classList.remove('currentmv'); });
        movebuttonini.classList.add('currentmv');
        document.querySelectorAll('.movebtn').forEach(function(el) { el.textContent = ''; });

        let qparamcube=cubeToParamText(c);
        let a = await fetch('/?' + qparammode + '=' + qparamcube);
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
                }else if( line.startsWith("moves: ") ) {
                    dolog('movecount', `${line}\n`);
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
    stopbtn.disabled = true;
    startTime = undefined;
}

let cubeimgmx = new TransformMatrix(1, 0, 0,  0, 1, 0,  0, 0, 1);

let curcube = csolved;
let fixedCornerColors = cubeCornerColors.map((it) => it.slice()); // copy 2D
let fixedEdgeColors = cubeEdgeColors.map((it) => it.slice());

const STYLEAPPLY_IMMEDIATE = 1;
const STYLEAPPLY_DEFER = 2;

function rotateCurWall(rotateDir, styleApply) {
    function rotateCurWallInt(rotateDir, styleApply) {
        curcube = cube.compose(curcube, crotated[rotateDir]);
        if( styleApply === STYLEAPPLY_DEFER ) {
            let c = curcube;
            setTimeout(() => c.applyToCubeImg(), 50);
        }else
            curcube.applyToCubeImg();
    }
    function rotateCurWallInt180(rotateDirCW) {
        if( styleApply === undefined ) {
            // minimize split rotations of pieces
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
    let isPermSwapsOdd = false;
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
        // constrain to solvable
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
            // possible further selection can leave only odd/even permutation
            // so, yield odd/even permutations separately
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
        // constrain to solvable
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
            // possible further selection can leave only odd/even permutation
            // so, yield odd/even permutations separately
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

const colorClassNames = [ 'cyellow', 'corange', 'cblue', 'cred', 'cgreen', 'cwhite' ];
const cornerIds = ['c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7', 'c8'];
const edgeIds = [ 'e1', 'e2', 'e3', 'e4', 'e5', 'e6', 'e7', 'e8', 'e9', 'ea', 'eb', 'ec' ];
const cornerSquareClasses = [ 'csq1', 'csq2', 'csq3' ];
const edgeSquareClasses = [ 'esq1', 'esq2' ];

function applyFixedColorsToImg()
{
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
    solvebtn.disabled = ! isFilled;
    cubelistaddcurrent.disabled = ! isFilled;
}

function selectColors(ev) {
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
    applyFixedColorsToImg();
}

function getDisplayedCube()
{
    let allowedCornerColors = getAllowedCornerColors();
    let allowedEdgeColors = getAllowedEdgeColors();
    let permParity = allowedEdgeColors.permParity & allowedCornerColors.permParity;
    let cc = allowedCornerColors.sampleCubecorners[permParity];
    let ce = allowedEdgeColors.sampleCubeedges[permParity];
	return cube.compose(new cube(cc, ce), curcube);
}

function displayCube(c) {
    fixedCornerColors = [[], [], [], [], [], [], [], []];
    for(let cno = 0; cno < 8; ++cno)
        for(let cwall = 0; cwall < 3; ++cwall)
            fixedCornerColors[curcube.cc.getPermAt(cno)][(curcube.cc.getOrientAt(cno)+cwall)%3] =
                cubeCornerColors[c.cc.getPermAt(cno)][(c.cc.getOrientAt(cno)+cwall)%3];
    fixedEdgeColors = [[], [], [], [], [], [], [], [], [], [], [], []];
    for(let eno = 0; eno < 12; ++eno)
        for(let ewall = 0; ewall < 2; ++ewall)
            fixedEdgeColors[curcube.ce.edgeN(eno)][(curcube.ce.edgeR(eno)+ewall)%2] =
                cubeEdgeColors[c.ce.edgeN(eno)][(c.ce.edgeR(eno)+ewall)%2];
    applyFixedColorsToImg();
}

function doNew() {
    let isEmpty = true;
    for(let cno = 0; cno < 8 && isEmpty; ++cno)
        for(let cwall = 0; cwall < 3 && isEmpty; ++cwall)
            isEmpty = fixedCornerColors[cno][cwall] == -1;
    for(let eno = 0; eno < 8 && isEmpty; ++eno)
        for(let ewall = 0; ewall < 3 && isEmpty; ++ewall)
            isEmpty = fixedCornerColors[eno][ewall] == -1;
    if( isEmpty ) {
        displayCube(csolved);
    }else{
        fixedCornerColors = [
            [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1],
            [-1, -1, -1], [-1, -1, -1], [-1, -1, -1], [-1, -1, -1]
        ];
        fixedEdgeColors = [
            [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1],
            [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1]
        ];
        document.querySelectorAll('.csel').forEach( (el) => { el.classList.remove('cnone'); } );
        applyFixedColorsToImg();
    }
}

function searchSolution() {
    document.querySelectorAll('.log').forEach((e) => {e.textContent = ''});
	let c = getDisplayedCube();
    let qparammode = solvequick.checked ? 'q' : solveqmulti.checked ? 'm' : 'o';
    localStorage.setItem('cube', `${c.cc.getPerms()} ${c.cc.getOrients()} ${c.ce.get()}`);
    localStorage.setItem('solvemode', qparammode);
    if( c.equals(csolved) )
        dolog('err', "already solved\n");
    else{
        document.querySelectorAll('.manipmodeonly,.changingcube').forEach((e) => {e.disabled = true;});
        stopbtn.disabled = false;
        searchMoves(c, qparammode);
    }
}

function searchStop() {
    fetch('/?stop');
}

function cubelistloadFromText(txt, isFromLocalStorage) {
    cubelistselect.innerHTML = '';
    for(let line of txt.split('\n')) {
        let cubeStr = line.trim();
        if( cubeStr ) {
            let opt = document.createElement('option');
            opt.value = cubeStr;
            opt.textContent = cubeStr;
            cubelistselect.append(opt);
        }
    }
    if( !isFromLocalStorage ) {
        cubelistitemno.value = '';
        localStorage.setItem('cubelist', txt);
        localStorage.setItem('cubelistitemno', '');
    }
}

function cubelistToText() {
    let txt = '';
    for(let opt of cubelistselect.options)
        txt += opt.value + '\n';
    return txt;
}

function cubelistAddCurrent() {
	let c = getDisplayedCube();
    let qparamcube = cubeToParamText(c);
    let opt = document.createElement('option');
    opt.value = qparamcube;
    opt.textContent = qparamcube;
    cubelistselect.insertAdjacentElement('afterbegin', opt);
    cubelistselect.selectedIndex = 0;
    cubelistitemno.value = '1';
    let cubelistText = cubelistToText();
    localStorage.setItem('cubelist', cubelistText);
}

function cubelistRemove() {
    let curSel = cubelistselect.selectedIndex;
    let removeOpts = Array.from(cubelistselect.selectedOptions);
    for(opt of removeOpts)
        opt.remove();
    if( curSel >= 0 && curSel < cubelistselect.length )
        cubelistselect.selectedIndex = curSel;
    cubelistChange();
    let cubelistText = cubelistToText();
    localStorage.setItem('cubelist', cubelistText);
}

function cubelistLoad() {
    cubelistloaddialogtext.value = cubelistToText();
    cubelistloaddialog.showModal();
}

function cubelistLoadDialogOk() {
    cubelistloaddialog.close();
    cubelistloadFromText(cubelistloaddialogtext.value);
}

function cubelistLoadDialogCancel() {
    cubelistloaddialog.close();
}

function cubelistLoadFromFile() {
    showOpenFilePicker({
        types: [ {
            description: 'saved cube list',
            accept: { 'text/plain': [ '.txt' ] }
        } ]
    }).then(async function ( [fileHandle] ) {
        let file = await fileHandle.getFile();
        let val = await file.text();
        cubelistloadFromText(val);
    }, function() {});
}

function cubelistSaveToFile() {
    showSaveFilePicker({
        suggestedName: 'cubelist.txt',
        types: [ {
            description: 'saved cube list',
            accept: { 'text/plain': [ '.txt' ] }
        } ]
    }).then(async function(fileHandle) {
        const fout = await fileHandle.createWritable();
        await fout.write(cubelistToText());
        await fout.close();
    }, function() {});
}

function cubelistChange() {
    if( cubelistselect.value ) {
        let c = cubeFromString(cubelistselect.value);
        if( c )
            displayCube(c);
        cubelistitemno.value = cubelistselect.selectedIndex+1;
    }else
        cubelistitemno.value = '';
    localStorage.setItem('cubelistitemno', cubelistitemno.value);
}

function cubeLoadFromInput(ev) {
    if( ev.type == 'keydown' && ev.key != 'Enter' )
        return;
    if( cubeinput.value ) {
        let c = cubeFromString(cubeinput.value);
        if( c )
            displayCube(c);
    }
}

onload = () => {
    if( window['showOpenFilePicker'] == undefined ) {
        cubelistloadfromfile.style.display = 'none';
        cubelistsavetofile.style.display = 'none';
    }
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
    for(let i = 0; i < 30; ++i)
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
    btnnew.addEventListener('click', doNew);
    solvebtn.addEventListener('click', searchSolution);
    stopbtn.addEventListener('click', searchStop);
    cubelistaddcurrent.addEventListener('click', cubelistAddCurrent);
    cubelistremove.addEventListener('click', cubelistRemove);
    cubelistload.addEventListener('click', cubelistLoad);
    cubelistloaddialogok.addEventListener('click', cubelistLoadDialogOk);
    cubelistloaddialogcancel.addEventListener('click', cubelistLoadDialogCancel);
    cubelistloadfromfile.addEventListener('click', cubelistLoadFromFile);
    cubelistsavetofile.addEventListener('click', cubelistSaveToFile);
    cubelistselect.addEventListener('change', cubelistChange);
    cubeloadbtn.addEventListener('click', cubeLoadFromInput);
    cubeinput.addEventListener('keydown', cubeLoadFromInput);
    let crestore = localStorage.getItem('cube');
    if( crestore ) {
        let cr = crestore.split(' ');
        let c = new cube(new cubecorners(+cr[0], +cr[1]), new cubeedges(BigInt(cr[2])));
        displayCube(c);
    }
    switch( localStorage.getItem('solvemode') ) {
        case 'q':
            solvequick.checked = true;
            break;
        case 'm':
            solveqmulti.checked = true;
            break;
        default:
            solveoptimal.checked = true;
            break;
    }
    crestore = localStorage.getItem('cubelist');
    if( crestore ) {
        cubelistloadFromText(crestore, true);
        let itemNo = localStorage.getItem('cubelistitemno');
        if( itemNo > 0 && itemNo <= cubelistselect.length ) {
            cubelistselect.selectedIndex = itemNo-1;
            cubelistitemno.value = itemNo;
        }
    }
}
