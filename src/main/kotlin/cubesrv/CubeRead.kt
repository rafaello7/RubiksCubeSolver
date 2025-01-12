package cubesrv

private fun isCubeSolvable(responder : Responder, c: cube) : Boolean
{
    var isSwapsOdd : Boolean = false
    var permScanned : Int = 0
    for(i in 0 ..< 8) {
        if( (permScanned and (1 shl i)) != 0 )
            continue
        permScanned = permScanned or (1 shl i)
        var p : Int = i
        while( true ) {
            p = c.ccp.getAt(p)
            if( p == i )
                break
            if( (permScanned and (1 shl p)) != 0 ) {
                responder.message("corner perm $p is twice")
                return false
            }
            permScanned = permScanned or (1 shl p)
            isSwapsOdd = !isSwapsOdd
        }
    }
    permScanned = 0
    for(i in 0 ..< 12) {
        if( (permScanned and (1 shl i)) != 0 )
            continue
        permScanned = permScanned or (1 shl i)
        var p : Int = i
        while( true ) {
            p = c.ce.getPermAt(p)
            if( p == i )
                break
            if( (permScanned and (1 shl p)) != 0 ) {
                responder.message("edge perm $p is twice")
                return false
            }
            permScanned = permScanned or (1 shl p)
            isSwapsOdd = !isSwapsOdd
        }
    }
	if( isSwapsOdd ) {
		responder.message("cube unsolvable due to permutation parity")
		return false
	}
	var sumOrient : Int = 0
	for(i in 0 ..< 8)
		sumOrient += c.cco.getAt(i)
	if( (sumOrient % 3) != 0 ) {
		responder.message("cube unsolvable due to corner orientations")
		return false
	}
	sumOrient = 0
	for(i in 0 ..< 12)
		sumOrient += c.ce.getOrientAt(i)
	if( (sumOrient % 2) != 0 ) {
		responder.message("cube unsolvable due to edge orientations")
		return false
	}
    return true
}

private class elemLoc(val wall : Int, val row : Int, val col : Int)

fun cubeFromColorsOnSquares(responder : Responder, squareColors : String,
        c : Array<cube>) : Boolean
{
    val R120 = arrayOf(1, 2, 0)
    val R240 = arrayOf(2, 0, 1)
	val walls = Array<Array<Array<cubecolor>>>(6) { Array<Array<cubecolor>>(3) {
        Array<cubecolor>(3){ cubecolor.CCOUNT }}}
	val colorLetters = "YOBRGW";
	val cornerLocMap = arrayOf(
		arrayOf( elemLoc(2, 0, 0), elemLoc(1, 0, 2), elemLoc(0, 2, 0) ),
		arrayOf( elemLoc(2, 0, 2), elemLoc(0, 2, 2), elemLoc(3, 0, 0) ),
		arrayOf( elemLoc(2, 2, 0), elemLoc(5, 0, 0), elemLoc(1, 2, 2) ),
		arrayOf( elemLoc(2, 2, 2), elemLoc(3, 2, 0), elemLoc(5, 0, 2) ),
		arrayOf( elemLoc(4, 0, 2), elemLoc(0, 0, 0), elemLoc(1, 0, 0) ),
		arrayOf( elemLoc(4, 0, 0), elemLoc(3, 0, 2), elemLoc(0, 0, 2) ),
		arrayOf( elemLoc(4, 2, 2), elemLoc(1, 2, 0), elemLoc(5, 2, 0) ),
		arrayOf( elemLoc(4, 2, 0), elemLoc(5, 2, 2), elemLoc(3, 2, 2) )
	)
	val edgeLocMap = arrayOf(
		arrayOf( elemLoc(2, 0, 1), elemLoc(0, 2, 1) ),
		arrayOf( elemLoc(1, 1, 2), elemLoc(2, 1, 0) ),
		arrayOf( elemLoc(3, 1, 0), elemLoc(2, 1, 2) ),
		arrayOf( elemLoc(2, 2, 1), elemLoc(5, 0, 1) ),
		arrayOf( elemLoc(0, 1, 0), elemLoc(1, 0, 1) ),
		arrayOf( elemLoc(0, 1, 2), elemLoc(3, 0, 1) ),
		arrayOf( elemLoc(5, 1, 0), elemLoc(1, 2, 1) ),
		arrayOf( elemLoc(5, 1, 2), elemLoc(3, 2, 1) ),
		arrayOf( elemLoc(4, 0, 1), elemLoc(0, 0, 1) ),
		arrayOf( elemLoc(1, 1, 0), elemLoc(4, 1, 2) ),
		arrayOf( elemLoc(3, 1, 2), elemLoc(4, 1, 0) ),
		arrayOf( elemLoc(4, 2, 1), elemLoc(5, 2, 1) )
	)
    for(cno in 0..53) {
        val idx = colorLetters.indexOf(squareColors[cno], ignoreCase = true)
        if( idx < 0 ) {
            responder.message("bad letter at column $cno")
            return false
        }
        walls[cno / 9][cno % 9 / 3][cno % 3] = cubecolor.entries[idx]
    }
	for(i in 0..5) {
		if( walls[i][1][1] != cubecolor.entries[i] ) {
			responder.message(
                "bad orientation: wall=$i exp=${colorLetters[i]} is=${colorLetters[walls[i][1][1].ordinal]}")
			return false
		}
	}
	for(i in 0..7) {
		var match : Boolean = false
		for(n in 0..7) {
			val elemColors : Array<cubecolor> = cubeCornerColors[n]
			match = true
			for(r in 0..2) {
				val el : elemLoc = cornerLocMap[i][r]
				if( walls[el.wall][el.row][el.col] != elemColors[r] ) {
					match = false
                    break
                }
			}
			if( match ) {
				c[0].ccp.setAt(i, n)
				c[0].cco.setAt(i, 0)
				break
			}
			match = true
			for(r in 0..2) {
				val el : elemLoc = cornerLocMap[i][r]
				if( walls[el.wall][el.row][el.col] != elemColors[R120[r]] ) {
					match = false
                    break
                }
			}
			if( match ) {
				c[0].ccp.setAt(i, n)
				c[0].cco.setAt(i, 1)
				break
			}
			match = true
			for(r in 0..2) {
				val el : elemLoc = cornerLocMap[i][r]
				if( walls[el.wall][el.row][el.col] != elemColors[R240[r]] ) {
					match = false
                    break
                }
			}
			if( match ) {
				c[0].ccp.setAt(i, n)
				c[0].cco.setAt(i, 2)
				break
			}
		}
		if( ! match ) {
			responder.message("corner $i not found")
			return false
		}
		for(j in 0..< i) {
			if( c[0].ccp.getAt(i) == c[0].ccp.getAt(j) ) {
				responder.message("corner ${c[0].ccp.getAt(i)} is twice: at $j and $i")
				return false
			}
		}
	}
	for(i in 0..11) {
		var match : Boolean = false
		for(n in 0..11) {
			val elemColors : Array<cubecolor> = cubeEdgeColors[n]
			match = true
			for(r in 0..1) {
				val el : elemLoc = edgeLocMap[i][r]
				if( walls[el.wall][el.row][el.col] != elemColors[r] ) {
                    match = false
                    break
                }
			}
			if( match ) {
				c[0].ce.setAt(i, n, 0)
				break
			}
			match = true
			for(r in 0..1) {
				val el : elemLoc = edgeLocMap[i][r]
				if( walls[el.wall][el.row][el.col] != elemColors[1-r] ) {
					match = false
                    break
                }
			}
			if( match ) {
				c[0].ce.setAt(i, n, 1)
				break
			}
		}
		if( ! match ) {
			responder.message("edge $i not found")
			return false
		}
		for(j in 0 ..< i) {
			if( c[0].ce.getPermAt(i) == c[0].ce.getPermAt(j) ) {
				responder.message("edge ${c[0].ce.getPermAt(i)} is twice: at $j and $i")
				return false
			}
		}
	}
    if( ! isCubeSolvable(responder, c[0]) )
        return false
    return true
}

private fun cubeFromScrambleStr(responder : Responder, scrambleStr : String, c: Array<cube>) : Boolean {
    val rotateMapFromExtFmt = arrayOf(
        "B1", "B2", "B3", "F1", "F2", "F3", "U1", "U2", "U3",
        "D1", "D2", "D3", "R1", "R2", "R3", "L1", "L2", "L3"
    )
    c[0] = csolved
    var i : Int = 0
    while(i+1 < scrambleStr.length) {
        val scramSub = scrambleStr.substring(i);
        if( scramSub[0].isLetterOrDigit() ) {
            var rd : Int = 0
            while( rd < rotate_dir.RCOUNT.ordinal && !scramSub.startsWith(rotateMapFromExtFmt[rd]))
                ++rd
            if( rd == rotate_dir.RCOUNT.ordinal ) {
                responder.message("Unkown move: ${scramSub.substring(0, 2)}")
                return false
            }
            c[0] = cube.compose(c[0], crotated[rd])
            i += 2
        }else
            ++i
    }
    return true
}

private fun mapColorsOnSqaresFromExt(ext : String) : String {
    // external format:
    //     U                Yellow
    //   L F R B   =   Blue   Red   Green Orange
    //     D                 White
    // sequence:
    //    yellow   green    red      white    blue     orange
    //  UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB
    val squaremap = arrayOf(
         2,  5,  8,  1,  4,  7,  0,  3,  6, // upper -> yellow
        45, 46, 47, 48, 49, 50, 51, 52, 53, // back -> orange
        36, 37, 38, 39, 40, 41, 42, 43, 44, // left -> blue
        18, 19, 20, 21, 22, 23, 24, 25, 26, // front -> red
         9, 10, 11, 12, 13, 14, 15, 16, 17, // right -> green
        33, 30, 27, 34, 31, 28, 35, 32, 29  // down -> white
    )
    val colormap = mapOf(
        'U' to 'Y', 'R' to 'G', 'F' to 'R',
        'D' to 'W', 'L' to 'B', 'B' to 'O'
    )
    var res : String = ""
    for(i in 0..53) {
        res += colormap[ext[squaremap[i]]]
    }
    return res
}

fun cubeFromString(responder : Responder, cubeStr : String, c : Array<cube>) : Boolean {
    var res : Boolean = false
    if( cubeStr matches Regex("[YOBRGW]{54}") )
        res = cubeFromColorsOnSquares(responder, cubeStr, c)
    else if( cubeStr matches Regex("[URFDLB]{54}") ) {
        val fromExt : String = mapColorsOnSqaresFromExt(cubeStr)
        res = cubeFromColorsOnSquares(responder, fromExt, c)
    }else if( cubeStr matches Regex("[URFDLB123 \t\r\n]+") )
        res = cubeFromScrambleStr(responder, cubeStr, c)
    else
        responder.message("cube string format not recognized: $cubeStr")
    return res
}
