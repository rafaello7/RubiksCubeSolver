package cubesrv
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

enum class rotate_dir {
    ORANGECW, ORANGE180, ORANGECCW, REDCW, RED180, REDCCW,
    YELLOWCW, YELLOW180, YELLOWCCW, WHITECW, WHITE180, WHITECCW,
    GREENCW, GREEN180, GREENCCW, BLUECW, BLUE180, BLUECCW,
    RCOUNT
}

enum class cubecolor {
    CYELLOW, CORANGE, CBLUE, CRED, CGREEN, CWHITE, CCOUNT
}

// The cube transformation (colors switch) through the cube rotation
enum class transform_dir {
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
}

class cubecorners_perm() : Comparable<cubecorners_perm> {
    var perm : Int = 0
    constructor(corner0perm : Int, corner1perm : Int,
            corner2perm : Int, corner3perm : Int,
            corner4perm : Int, corner5perm : Int,
            corner6perm : Int, corner7perm : Int) : this() {
        perm = corner0perm or (corner1perm shl 3) or (corner2perm shl 6) or
            (corner3perm shl 9) or (corner4perm shl 12) or (corner5perm shl 15) or
            (corner6perm shl 18) or (corner7perm shl 21)
    }

    fun setAt(idx : Int, p : Int) {
        perm = perm and ((7 shl 3*idx).inv())
        perm = perm or (p shl 3*idx)
    }

    fun getAt(idx : Int) : Int = (perm ushr 3*idx) and 7
    fun get() : Int = perm
    fun set(p : Int) { perm = p; }
    companion object {
        fun compose(ccp1 : cubecorners_perm, ccp2 : cubecorners_perm) : cubecorners_perm {
            val res = cubecorners_perm()
            for(i in 0..<8)
                res.perm = res.perm or (ccp1.getAt(ccp2.getAt(i)) shl 3*i)
            return res
        }
        fun compose3(ccp1 : cubecorners_perm, ccp2 : cubecorners_perm, ccp3 : cubecorners_perm)
            : cubecorners_perm
        {
            val res = cubecorners_perm()
            for(i in 0 ..< 8) {
                val corner3perm : Int = (ccp3.perm ushr 3 * i) and 0x7
                val corner2perm : Int = (ccp2.perm ushr 3 * corner3perm) and 0x7
                val corner1perm : Int = (ccp1.perm ushr 3 * corner2perm) and 0x7
                res.perm = res.perm or (corner1perm shl 3*i)
            }
            return res
        }

        fun fromPermIdx(idxp : Int) : cubecorners_perm {
            var idx = idxp
            var unused = 0x76543210
            val ccp = cubecorners_perm()

            for(cornerIdx in 8 downTo 1) {
                val p = idx % cornerIdx * 4
                ccp.setAt(8-cornerIdx, (unused ushr p) and 0xf)
                val m = -1 shl p
                unused = (unused and m.inv()) or ((unused ushr 4) and m)
                idx /= cornerIdx
            }
            return ccp
        }
    }

    fun symmetric() : cubecorners_perm {
        val permRes = perm xor 0x924924
        val ccpres = cubecorners_perm()
        ccpres.perm = ((permRes ushr 12) or (permRes shl 12)) and 0xffffff
        return ccpres
    }

    fun reverse() : cubecorners_perm {
        val res = cubecorners_perm()
        for(i in 0 ..< 8)
            res.perm = res.perm or (i shl 3*getAt(i))
        return res
    }

    fun transform(transformDir : Int) : cubecorners_perm {
        val cctr : cubecorners_perm = ctransformed[transformDir].ccp
        val ccrtr : cubecorners_perm = ctransformed[transformReverse(transformDir)].ccp
        return compose3(cctr, this, ccrtr)
    }

    override fun equals(other : Any?) : Boolean = perm == (other as cubecorners_perm).perm
    override fun hashCode(): Int = perm.hashCode()
    override fun compareTo(other : cubecorners_perm) : Int = perm.compareTo(other.perm)

    fun getPermIdx() : Int {
        var res = 0
        var indexes = 0
        for(i in 7 downTo 0) {
            val p = getAt(i) * 4
            res = (8-i) * res + ((indexes shr p) and 0xf)
            indexes += 0x11111111 shl p
        }
        return res
    }

    fun isPermParityOdd() : Boolean {
        var isSwapsOdd = false
        var permScanned = 0
        for(i in 0 ..< 8) {
            if( (permScanned and (1 shl i)) != 0 )
                continue
            permScanned = permScanned or (1 shl i)
            var p = i
            while( true ) {
                p = getAt(p)
                if( p == i )
                    break
                permScanned = permScanned or (1 shl p)
                isSwapsOdd = !isSwapsOdd
            }
        }
        return isSwapsOdd
    }
}

class cubecorner_orients() : Comparable<cubecorner_orients> {
    var orients : Int = 0
    constructor(corner0orient : Int, corner1orient : Int,
            corner2orient : Int, corner3orient : Int,
            corner4orient : Int, corner5orient : Int,
            corner6orient : Int, corner7orient : Int) : this()
    {
        orients = corner0orient or (corner1orient shl 2) or (corner2orient shl 4) or
            (corner3orient shl 6) or (corner4orient shl 8) or (corner5orient shl 10) or
            (corner6orient shl 12) or (corner7orient shl 14)
    }

    fun setAt(idx : Int, orient : Int) {
        orients = orients and (3 shl 2*idx).inv()
        orients = orients or (orient shl 2*idx)
    }

    fun getAt(idx : Int) : Int = (orients ushr 2*idx) and 3
    fun get() : Int = orients
    fun set(orts: Int) { orients = orts; }
    companion object {
        fun compose(
            cco1: cubecorner_orients,
            ccp2: cubecorners_perm, cco2: cubecorner_orients
        ) : cubecorner_orients
        {
            val res = cubecorner_orients()
            val MOD3 = arrayOf(0, 1, 2, 0, 1)
            for(i in 0 ..< 8) {
                val cc2Perm = (ccp2.get() ushr 3*i) and 7
                val cc2Orient = (cco2.orients ushr 2*i) and 3
                val cco1Orient = (cco1.orients ushr 2*cc2Perm) and 3
                val resOrient = MOD3[cco1Orient + cc2Orient]
                res.orients = res.orients or (resOrient shl 2 * i)
            }
            return res
        }

        fun compose3(
            cco1: cubecorner_orients,
            ccp2: cubecorners_perm, cco2: cubecorner_orients,
            ccp3: cubecorners_perm, cco3: cubecorner_orients
        ) : cubecorner_orients
        {
            val res = cubecorner_orients()
            val MOD3 = arrayOf(0, 1, 2, 0, 1, 2, 0)
            for(i in 0 ..< 8) {
                val ccp3Perm = (ccp3.get() ushr 3*i) and 7
                val cco3orient = (cco3.orients ushr 2*i) and 3
                val midperm = (ccp2.get() ushr 3*ccp3Perm) and 7
                val cco2orient = (cco2.orients ushr 2*ccp3Perm) and 3
                val cco1orient = (cco1.orients ushr 2*midperm) and 3
                val resorient = MOD3[cco1orient + cco2orient + cco3orient]
                res.orients = res.orients or (resorient shl 2 * i)
            }
            return res
        }

        fun fromOrientIdx(idxp : Int) : cubecorner_orients {
            var idx = idxp
            val res = cubecorner_orients()
            var sum = 0
            for(i in 0 ..< 7) {
                val value = idx % 3
                idx /= 3
                res.setAt(6-i, value)
                sum += value
            }
            res.setAt(7, (15-sum) % 3)
            return res
        }
    }

    fun symmetric() : cubecorner_orients {
        val ccores = cubecorner_orients()
        // set orient 2 -> 1, 1 -> 2, 0 unchanged
        val orie = ((orients and 0xaaaa) ushr 1) or ((orients and 0x5555) shl 1)
        ccores.orients = ((orie ushr 8) or (orie shl 8)) and 0xffff
        return ccores
    }

    fun reverse(ccp : cubecorners_perm) : cubecorner_orients {
        val revOrients = ((orients and 0xaaaa) ushr 1) or ((orients and 0x5555) shl 1)
        val res = cubecorner_orients()
        for(i in 0 ..< 8)
            res.orients = res.orients or (((revOrients ushr 2*i) and 3) shl 2 * ccp.getAt(i))
        return res
    }

    fun transform(ccp : cubecorners_perm, transformDir : Int) : cubecorner_orients {
        val ctrans = ctransformed[transformDir]
        val ctransRev = ctransformed[transformReverse(transformDir)]
        return compose3(ctrans.cco, ccp, this, ctransRev.ccp, ctransRev.cco)
    }

    override fun equals(other : Any?) = orients == (other as cubecorner_orients).orients
    override fun hashCode() = orients.hashCode()
    override fun compareTo(other : cubecorner_orients) = orients.compareTo(other.orients)

    fun getOrientIdx() : Int {
        var res = 0
        for(i in 0 ..< 7)
            res = res * 3 + getAt(i)
        return res
    }

    fun isBGspace() : Boolean = orients == 0

    fun isYWspace(ccp : cubecorners_perm) : Boolean {
        val orients0356 = arrayOf(0, 1, 1, 0, 1, 0, 0, 1)
        val orients1247 = arrayOf(2, 0, 0, 2, 0, 2, 2, 0)
        for(i in 0 ..< 8) {
            when( ccp.getAt(i) ) {
                in arrayOf(0, 3, 5, 6) ->
                    if( getAt(i) != orients0356[i] )
                        return false
                in arrayOf(1, 2, 4, 7) ->
                    if( getAt(i) != orients1247[i] )
                        return false
            }
        }
        return true
    }

    fun isORspace(ccp : cubecorners_perm) : Boolean {
        val orients0356 = arrayOf(0, 2, 2, 0, 2, 0, 0, 2)
        val orients1247 = arrayOf(1, 0, 0, 1, 0, 1, 1, 0)
        for(i in 0 ..< 8) {
            when ( ccp.getAt(i) ) {
                in arrayOf(0, 3, 5, 6) ->
                    if( getAt(i) != orients0356[i] )
                        return false
                in arrayOf(1, 2, 4, 7) ->
                    if( getAt(i) != orients1247[i] )
                        return false
            }
        }
        return true
    }

    fun representativeBG(ccp : cubecorners_perm) : cubecorner_orients {
        val orepr = cubecorner_orients()
        for(i in 0 ..< 8)
            orepr.orients = orepr.orients or (((orients ushr 2*i) and 3) shl 2*ccp.getAt(i))
        return orepr
    }

    fun representativeYW(ccp : cubecorners_perm) : cubecorner_orients {
        val oadd = arrayOf(2, 1, 1, 2, 1, 2, 2, 1)
        val orepr = cubecorner_orients()
        val ccpRev = ccp.reverse()
        for(i in 0 ..< 8) {
            val toAdd = if(oadd[i] == oadd[ccpRev.getAt(i)]) 0 else oadd[i]
            orepr.setAt(i, (getAt(ccpRev.getAt(i))+toAdd) % 3)
        }
        return orepr
    }

    fun representativeOR(ccp : cubecorners_perm) : cubecorner_orients {
        val oadd = arrayOf(1, 2, 2, 1, 2, 1, 1, 2)
        val orepr = cubecorner_orients()
        val ccpRev = ccp.reverse()
        for(i in 0 ..< 8) {
            val toAdd = if(oadd[i] == oadd[ccpRev.getAt(i)]) 0 else oadd[i]
            orepr.setAt(i, (getAt(ccpRev.getAt(i))+toAdd) % 3)
        }
        return orepr
    }
}

class cubeedges(private var edges : Long = 0) : Comparable<cubeedges> {
    constructor(edge0perm : Int, edge1perm : Int, edge2perm : Int,
        edge3perm : Int, edge4perm : Int, edge5perm : Int,
        edge6perm : Int, edge7perm : Int, edge8perm : Int,
        edge9perm : Int, edge10perm : Int, edge11perm : Int,
        edge0orient : Int, edge1orient : Int, edge2orient : Int,
        edge3orient : Int, edge4orient : Int, edge5orient : Int,
        edge6orient : Int, edge7orient : Int, edge8orient : Int,
        edge9orient : Int, edge10orient : Int, edge11orient : Int)
        : this()
    {
        edges = edge0perm.toLong() or      (edge0orient.toLong() shl 4) or
            (edge1perm.toLong() shl 5)  or (edge1orient.toLong() shl 9) or
            (edge2perm.toLong() shl 10) or (edge2orient.toLong() shl 14) or
            (edge3perm.toLong() shl 15) or (edge3orient.toLong() shl 19) or
            (edge4perm.toLong() shl 20) or (edge4orient.toLong() shl 24) or
            (edge5perm.toLong() shl 25) or (edge5orient.toLong() shl 29) or
            (edge6perm.toLong() shl 30) or (edge6orient.toLong() shl 34) or
            (edge7perm.toLong() shl 35) or (edge7orient.toLong() shl 39) or
            (edge8perm.toLong() shl 40) or (edge8orient.toLong() shl 44) or
            (edge9perm.toLong() shl 45) or (edge9orient.toLong() shl 49) or
            (edge10perm.toLong() shl 50) or (edge10orient.toLong() shl 54) or
            (edge11perm.toLong() shl 55) or (edge11orient.toLong() shl 59)
    }


    fun setAt(idx : Int, perm : Int, orient : Int) {
        edges = edges and (0x1FL shl 5*idx).inv()
        edges = edges or (((orient shl 4) or perm).toLong() shl 5*idx)
    }

    fun setPermAt(idx : Int, perm : Int) : Unit {
        edges = edges and (0xFL shl 5*idx).inv()
        edges = edges or (perm.toLong() shl 5*idx)
    }

    fun setOrientAt(idx : Int, orient : Int) {
        edges = edges and (0x1L shl 5*idx+4).inv()
        edges = edges or (orient.toLong() shl 5*idx+4)
    }

    companion object {
        fun compose(ce1 : cubeedges, ce2 : cubeedges) : cubeedges {
            val res = cubeedges()
            for(i in 0 ..< 12) {
                val edge2perm = ((ce2.edges ushr 5 * i) and 0xf).toInt()
                val edge1item = (ce1.edges ushr 5 * edge2perm) and 0x1f
                res.edges = res.edges or (edge1item shl 5*i)
            }
            val edge2orients = ce2.edges and 0x842108421084210L
            res.edges = res.edges xor edge2orients
            return res
        }

        fun compose3(ce1 : cubeedges, ce2 : cubeedges, ce3 : cubeedges) : cubeedges {
            val res = cubeedges()
            //res = compose(compose(ce1, ce2), ce3);
            for(i in 0 ..< 12) {
                val edge3perm = ((ce3.edges ushr 5 * i) and 0xf).toInt()
                val edge2item = ce2.edges ushr 5 * edge3perm
                val edge2perm = (edge2item and 0xf).toInt()
                val edge2orient = edge2item and 0x10
                val edge1item = (ce1.edges ushr 5 * edge2perm) and 0x1f
                val edgemitem = edge1item xor edge2orient
                res.edges = res.edges or (edgemitem shl 5*i)
            }
            val edge3orients = ce3.edges and 0x842108421084210L
            res.edges = res.edges xor edge3orients
            return res
        }

        // the middle cubeedges should be reversed before compose
        fun compose3revmid(ce1 : cubeedges, ce2 : cubeedges, ce3 : cubeedges) : cubeedges {
            val res = cubeedges()
            val ce2rperm = Array<Int>(12) { 0 }
            val ce2rorient = Array<Long>(12) { 0 }
            for(i in 0 ..< 12) {
                val edge2item = ce2.edges ushr 5*i
                val edge2perm = (edge2item and 0xf).toInt()
                ce2rperm[edge2perm] = i
                ce2rorient[edge2perm] = edge2item and 0x10L
            }
            for(i in 0 ..< 12) {
                val edge3perm = ((ce3.edges ushr 5 * i) and 0xf).toInt()
                val edge1item = (ce1.edges ushr 5 * ce2rperm[edge3perm]) and 0x1f
                val edgemitem = edge1item xor ce2rorient[edge3perm]
                res.edges = res.edges or (edgemitem shl 5*i)
            }
            val edge3orients = ce3.edges and 0x842108421084210L
            res.edges = res.edges xor edge3orients
            return res
        }

        fun isCeReprSolvable(ce : cubeedges) : Boolean
        {
            var isSwapsOdd = false
            var permScanned = 0
            for(i in 0 ..< 12) {
                if( (permScanned and (1 shl i)) != 0 )
                    continue
                permScanned = permScanned or (1 shl i)
                var p = i
                while( true ) {
                    p = ce.getPermAt(p)
                    if( p == i )
                        break
                    if( (permScanned and (1 shl p)) != 0 ) {
                        println("edge perm $p is twice")
                        return false
                    }
                    permScanned = permScanned or (1 shl p)
                    isSwapsOdd = !isSwapsOdd
                }
            }
            if( isSwapsOdd ) {
                println("cubeedges unsolvable due to permutation parity")
                return false
            }
            var sumOrient = 0
            for(i in 0 ..< 12)
                sumOrient += ce.getOrientAt(i)
            if( sumOrient % 2 != 0 ) {
                println("cubeedges unsolvable due to edge orientations")
                return false
            }
            return true
        }

        fun fromPermAndOrientIdx(permIdxp : Int, orientIdxp : Int) : cubeedges {
            var permIdx = permIdxp
            var orientIdx = orientIdxp
            var unused = 0xba9876543210L
            val ce = cubeedges()
            var orient11 = 0

            for(edgeIdx in 12 downTo 1) {
                val p = permIdx % edgeIdx * 4
                ce.setPermAt(12-edgeIdx, (unused ushr p).toInt() and 0xf)
                ce.setOrientAt(12-edgeIdx, orientIdx and 1)
                var m : Long = -1L shl p
                unused = (unused and m.inv()) or ((unused ushr 4) and m)
                permIdx /= edgeIdx
                orient11 = orient11 xor orientIdx
                orientIdx /= 2
            }
            ce.setOrientAt(11, orient11 and 1)
            return ce
        }
    }

    fun symmetric() : cubeedges {
        val ceres = cubeedges()
        // map perm: 0 1 2 3 4 5 6 7 8 9 10 11 -> 8 9 10 11 4 5 6 7 0 1 2 3
        val edg = edges xor ((edges.inv() and 0x210842108421084L) shl 1)
        ceres.edges = (edg ushr 40) or (edg and 0xfffff00000L) or ((edg and 0xfffff) shl 40)
        return ceres
    }

    fun reverse() : cubeedges {
        val res = cubeedges()
        for(i in 0 ..< 12) {
            val edgeItem = edges ushr 5*i
            res.edges = res.edges or (((edgeItem and 0x10L) or i.toLong()) shl 5*(edgeItem.toInt() and 0xf))
        }
        return res
    }

    fun transform(idx : Int) : cubeedges {
        val cetrans = ctransformed[idx].ce
        val res = cubeedges()
        for(i in 0 ..< 12) {
            val ceItem : Int = (this.edges ushr 5 * i).toInt()
            val cePerm : Int = ceItem and 0xf
            val ceOrient : Int = ceItem and 0x10
            val ctransPerm : Int = (cetrans.edges ushr 5 * cePerm).toInt() and 0xf
            val ctransPerm2 : Int = (cetrans.edges ushr 5 * i).toInt() and 0xf
            res.edges = res.edges or ((ctransPerm or ceOrient).toLong() shl 5*ctransPerm2)
        }
        return res
    }

    fun getPermAt(idx: Int) : Int = (edges ushr 5 * idx).toInt() and 0xf
    fun getOrientAt(idx : Int) : Int = (edges ushr (5*idx+4)).toInt() and 1

    override fun equals(other : Any?) = edges == (other as cubeedges).edges
    override fun hashCode() = edges.hashCode()
    override fun compareTo(other : cubeedges) = edges.compareTo(other.edges)

    fun getPermIdx() : Int {
        var res = 0
        var indexes : Long = 0L
        val e = edges shl 2
        var shift = 60
        for(i in 1 .. 12) {
            shift -= 5
            val p = (e ushr shift).toInt() and 0x3c
            res = i * res + ((indexes ushr p).toInt() and 0xf)
            indexes += 0x111111111111L shl p
        }
        return res
    }

    fun getOrientIdx() : Int {
        var orients = (edges and 0x42108421084210L) ushr 4
        // orients == k0000j0000i0000h0000g0000f0000e0000d0000c0000b0000a
        orients = orients or (orients ushr 4)
        // orients == k000kj000ji000ih000hg000gf000fe000ed000dc000cb000ba
        orients = orients or (orients ushr 8)
        // orients == k000kj00kji0kjih0jihg0ihgf0hgfe0gfed0fedc0edcb0dcba
        orients = orients and 0x70000f0000fL
        // orients ==         kji0000000000000000hgfe0000000000000000dcba
        val res = orients or (orients ushr 16) or (orients ushr 32)
        // res == kjihgfedcba
        return res.toInt() and 0xffff
    }

    fun isPermParityOdd() : Boolean {
        var isSwapsOdd = false
        var permScanned = 0
        for(i in 0 ..< 12) {
            if( (permScanned and (1 shl i)) != 0 )
                continue
            permScanned = permScanned or (1 shl i)
            var p = i
            while( true ) {
                p = getPermAt(p)
                if( p == i )
                    break
                permScanned = permScanned or (1 shl p)
                isSwapsOdd = !isSwapsOdd
            }
        }
        return isSwapsOdd
    }

    fun get() = edges
    fun set(e : Long) { edges = e; }
    fun isNil() : Boolean = edges == 0L

    fun isBGspace() : Boolean {
        val orients03811 = arrayOf(0, 1, 1, 0, 2, 2, 2, 2, 0, 1, 1, 0)
        val orients12910 = arrayOf(1, 0, 0, 1, 2, 2, 2, 2, 1, 0, 0, 1)
        for(i in 0 ..< 12) {
            if( i < 4 || i >= 8 ) {
                when( getPermAt(i) ) {
                    in arrayOf(0, 3, 8, 11) ->
                        if( getOrientAt(i) != orients03811[i] )
                            return false
                    in arrayOf(1, 2, 9, 10) -> {
                        if( getOrientAt(i) != orients12910[i] )
                            return false
                    }
                    else -> return false
                }
            }else{
                if( getOrientAt(i) != 0 )
                    return false
            }
        }
        return true
    }

    fun isYWspace() : Boolean {
        val orients03811 = arrayOf(0, 2, 2, 0, 1, 1, 1, 1, 0, 2, 2, 0)
        val orients4567  = arrayOf(1, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 1)
        for(i in 0 ..< 12) {
            if( orients03811[i] == 2 ) {
                if( getOrientAt(i) != 0 )
                    return false
            }else{
                when( getPermAt(i) ) {
                    in arrayOf(0, 3, 8, 11) ->
                        if( getOrientAt(i) != orients03811[i] )
                            return false
                    in 4 .. 7 -> {
                        if( getOrientAt(i) != orients4567[i] )
                            return false
                    }
                    else -> return false
                }
            }
        }
        return true
    }

    fun isORspace() : Boolean {
        val orients12910 = arrayOf(2, 0, 0, 2, 1, 1, 1, 1, 2, 0, 0, 2)
        val orients4567  = arrayOf(2, 1, 1, 2, 0, 0, 0, 0, 2, 1, 1, 2)
        for(i in 0 ..< 12) {
            if( orients12910[i] == 2 ) {
                if( getOrientAt(i) != 0 )
                    return false;
            }else{
                when( getPermAt(i) ) {
                    in arrayOf(1, 2, 9, 10) ->
                        if( getOrientAt(i) != orients12910[i] )
                            return false
                    in 4 .. 7 -> {
                        if( getOrientAt(i) != orients4567[i] )
                            return false
                    }
                    else -> return false
                }
            }
        }
        return true
    }

    fun representativeBG() : cubeedges {
        val cerepr = cubeedges()
        val orientsIn = 0x6060L

        val ceRev = reverse()
        var destIn = 0
        var destOut = 4
        var permOutSum = 0
        for(i in 0 ..< 12) {
            val ipos = ceRev.getPermAt(i)
            var item = (edges ushr 5*ipos) and 0x1f
            if( ipos < 4 || ipos >= 8 ) {
                item = item xor (((orientsIn ushr ipos) xor (orientsIn ushr destIn)) and 0x10)
                cerepr.edges = cerepr.edges or (item shl 5*destIn)
                ++destIn
                destIn += destIn and 0x4 // if destIn == 4 then destIn := 8
            }else{
                permOutSum += i
                if( destOut == 7 && (permOutSum and 1) != 0 ) {
                    // fix permutation parity
                    val item6 = (cerepr.edges ushr 6*5) and 0x1f
                    cerepr.edges = cerepr.edges and (0x1fL shl 30).inv()
                    cerepr.edges = cerepr.edges or (item6 shl 7*5)
                    --destOut
                }
                cerepr.edges = cerepr.edges or (item shl 5*destOut)
                ++destOut
            }
        }
        return cerepr
    }

    fun representativeYW() : cubeedges {
        val cerepr = cubeedges()
        val orientsIn = arrayOf(1, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 1)

        val ceRev = reverse()
        var destIn = 0
        var destOut = 0
        for(i in 0 ..< 12) {
            val ipos = ceRev.getPermAt(i)
            if( orientsIn[ipos] != 2 ) {
                while( orientsIn[destIn] == 2 )
                    ++destIn
                cerepr.setPermAt(destIn, i)
                cerepr.setOrientAt(destIn,
                    if( orientsIn[ipos] == orientsIn[destIn] ) getOrientAt(ipos) else 1-getOrientAt(ipos))
                ++destIn
            }else{
                while( orientsIn[destOut] != 2 )
                    ++destOut
                cerepr.setPermAt(destOut, i)
                cerepr.setOrientAt(destOut, getOrientAt(ipos))
                ++destOut
            }
        }
        // make the cube solvable
        var isSwapsOdd = false
        var permScanned = 0
        for(i in 0 ..< 12) {
            if( (permScanned and (1 shl i)) != 0 )
                continue
            permScanned = permScanned or (1 shl i)
            var p = i
            while( true ) {
                p = cerepr.getPermAt(p)
                if( p == i )
                    break
                permScanned = permScanned or (1 shl p)
                isSwapsOdd = !isSwapsOdd
            }
        }
        if( isSwapsOdd ) {
            val p = cerepr.getPermAt(10)
            val o = cerepr.getOrientAt(10)
            cerepr.setPermAt(10, cerepr.getPermAt(9))
            cerepr.setOrientAt(10, cerepr.getOrientAt(9))
            cerepr.setPermAt(9, p)
            cerepr.setOrientAt(9, o)
        }
        if( !isCeReprSolvable(this) ) {
            println("fatal: YW cube repesentative is unsolvable")
            kotlin.system.exitProcess(1)
        }
        return cerepr
    }

    fun representativeOR() : cubeedges {
        val cerepr = cubeedges()
        val orientsIn = arrayOf(2, 0, 0, 2, 1, 1, 1, 1, 2, 0, 0, 2)

        val ceRev = reverse()
        var destIn = 0
        var destOut = 0
        for(i in 0 ..< 12) {
            val ipos = ceRev.getPermAt(i)
            if( orientsIn[ipos] != 2 ) {
                while( orientsIn[destIn] == 2 )
                    ++destIn
                cerepr.setPermAt(destIn, i)
                cerepr.setOrientAt(destIn, if(orientsIn[ipos] == orientsIn[destIn]) 
                    getOrientAt(ipos) else 1-getOrientAt(ipos))
                ++destIn
            }else{
                while( orientsIn[destOut] != 2 )
                    ++destOut
                cerepr.setPermAt(destOut, i)
                cerepr.setOrientAt(destOut, getOrientAt(ipos))
                ++destOut
            }
        }
        // make the cube solvable
        var isSwapsOdd = false
        var permScanned = 0
        for(i in 0 ..< 12) {
            if( (permScanned and (1 shl i)) != 0 )
                continue
            permScanned = permScanned or (1 shl i)
            var p = i
            while( true ) {
                p = cerepr.getPermAt(p)
                if( p == i )
                    break
                permScanned = permScanned or (1 shl p)
                isSwapsOdd = !isSwapsOdd
            }
        }
        if( isSwapsOdd ) {
            val p = cerepr.getPermAt(8)
            val o = cerepr.getOrientAt(8)
            cerepr.setPermAt(8, cerepr.getPermAt(11))
            cerepr.setOrientAt(8, cerepr.getOrientAt(11))
            cerepr.setPermAt(11, p)
            cerepr.setOrientAt(11, o)
        }
        if( !isCeReprSolvable(this) ) {
            println("fatal: OR cube repesentative is unsolvable")
            kotlin.system.exitProcess(1)
        }
        return cerepr
    }
}

val R120 = arrayOf(1, 2, 0)
val R240 = arrayOf(2, 0, 1)

val cubeCornerColors : Array<Array<cubecolor>> = arrayOf(
  arrayOf(cubecolor.CBLUE,   cubecolor.CORANGE, cubecolor.CYELLOW),
  arrayOf(cubecolor.CBLUE,  cubecolor.CYELLOW, cubecolor.CRED),
  arrayOf(cubecolor.CBLUE,   cubecolor.CWHITE,  cubecolor.CORANGE),
  arrayOf(cubecolor.CBLUE,  cubecolor.CRED,    cubecolor.CWHITE),
  arrayOf(cubecolor.CGREEN,  cubecolor.CYELLOW, cubecolor.CORANGE),
  arrayOf(cubecolor.CGREEN, cubecolor.CRED,    cubecolor.CYELLOW),
  arrayOf(cubecolor.CGREEN,  cubecolor.CORANGE, cubecolor.CWHITE),
  arrayOf(cubecolor.CGREEN, cubecolor.CWHITE,  cubecolor.CRED)
)

val cubeEdgeColors : Array<Array<cubecolor>> = arrayOf(
  arrayOf(cubecolor.CBLUE,   cubecolor.CYELLOW),
  arrayOf(cubecolor.CORANGE, cubecolor.CBLUE  ),
  arrayOf(cubecolor.CRED,    cubecolor.CBLUE),
  arrayOf(cubecolor.CBLUE,   cubecolor.CWHITE ),
  arrayOf(cubecolor.CYELLOW, cubecolor.CORANGE),
  arrayOf(cubecolor.CYELLOW, cubecolor.CRED),
  arrayOf(cubecolor.CWHITE,  cubecolor.CORANGE),
  arrayOf(cubecolor.CWHITE,  cubecolor.CRED),
  arrayOf(cubecolor.CGREEN,  cubecolor.CYELLOW),
  arrayOf(cubecolor.CORANGE, cubecolor.CGREEN ),
  arrayOf(cubecolor.CRED,    cubecolor.CGREEN),
  arrayOf(cubecolor.CGREEN,  cubecolor.CWHITE )
)

class cube(
    var ccp : cubecorners_perm = cubecorners_perm(),
    var cco : cubecorner_orients = cubecorner_orients(),
    var ce : cubeedges = cubeedges()) : Comparable<cube>
{

    companion object {
        fun compose(c1 : cube, c2 : cube) : cube {
            return cube(cubecorners_perm.compose(c1.ccp, c2.ccp),
                        cubecorner_orients.compose(c1.cco, c2.ccp, c2.cco),
                        cubeedges.compose(c1.ce, c2.ce))
        }
    }

    fun symmetric() : cube {
        return cube(ccp.symmetric(), cco.symmetric(), ce.symmetric())
    }

    fun reverse() : cube {
        return cube(ccp.reverse(), cco.reverse(ccp), ce.reverse())
    }

    fun transform(transformDir : Int) : cube {
        val ctrans = ctransformed[transformDir];
        val ccp1 = cubecorners_perm.compose(ctrans.ccp, this.ccp)
        val cco1 = cubecorner_orients.compose(ctrans.cco, this.ccp, this.cco)
        val ce1 = cubeedges.compose(ctrans.ce, this.ce)

        val ctransRev = ctransformed[transformReverse(transformDir)]
        val ccp2 = cubecorners_perm.compose(ccp1, ctransRev.ccp)
        val cco2 = cubecorner_orients.compose(cco1, ctransRev.ccp, ctransRev.cco)
        val ce2 = cubeedges.compose(ce1, ctransRev.ce)
        return cube(ccp2, cco2, ce2)
    }

    override fun equals(other : Any?) : Boolean {
        val c = other as cube
        return ccp == c.ccp && cco == c.cco && ce == c.ce
    }

    override fun compareTo(other : cube) : Int {
        return if( ccp.equals(other.ccp) ) {
            if( cco.equals(other.cco) ) {
                ce.compareTo(other.ce)
            }else
                cco.compareTo(other.cco)
        }else
            ccp.compareTo(other.ccp)
    }

    fun isBGspace() : Boolean = cco.isBGspace() && ce.isBGspace()
    fun isYWspace() : Boolean = cco.isYWspace(ccp) && ce.isYWspace()
    fun isORspace() : Boolean = cco.isORspace(ccp) && ce.isORspace()

    fun representativeBG() : cube {
        val ccoRepr = cco.representativeBG(ccp)
        val ceRepr = ce.representativeBG()
        return cube(csolved.ccp, ccoRepr, ceRepr)
    }

    fun representativeYW() : cube {
        val ccoRepr = cco.representativeYW(ccp)
        val ceRepr = ce.representativeYW()
        return cube(csolved.ccp, ccoRepr, ceRepr)
    }

    fun representativeOR() : cube {
        val ccoRepr = cco.representativeOR(ccp)
        val ceRepr = ce.representativeOR()
        return cube(csolved.ccp, ccoRepr, ceRepr)
    }

    fun toParamText() : String {
        val colorChars = arrayOf("Y", "O", "B", "R", "G", "W")
        var res : String = ""

        res += colorChars[cubeCornerColors[ccp.getAt(4)][R120[cco.getAt(4)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(8)][1-ce.getOrientAt(8)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(5)][R240[cco.getAt(5)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(4)][ce.getOrientAt(4)].ordinal]
        res += 'Y'
        res += colorChars[cubeEdgeColors[ce.getPermAt(5)][ce.getOrientAt(5)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(0)][R240[cco.getAt(0)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(0)][1-ce.getOrientAt(0)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(1)][R120[cco.getAt(1)]].ordinal]

        res += colorChars[cubeCornerColors[ccp.getAt(4)][R240[cco.getAt(4)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(4)][1-ce.getOrientAt(4)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(0)][R120[cco.getAt(0)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(9)][ce.getOrientAt(9)].ordinal]
        res += 'O'
        res += colorChars[cubeEdgeColors[ce.getPermAt(1)][ce.getOrientAt(1)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(6)][R120[cco.getAt(6)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(6)][1-ce.getOrientAt(6)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(2)][R240[cco.getAt(2)]].ordinal]

        res += colorChars[cubeCornerColors[ccp.getAt(0)][cco.getAt(0)].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(0)][ce.getOrientAt(0)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(1)][cco.getAt(1)].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(1)][1-ce.getOrientAt(1)].ordinal]
        res += 'B'
        res += colorChars[cubeEdgeColors[ce.getPermAt(2)][1-ce.getOrientAt(2)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(2)][cco.getAt(2)].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(3)][ce.getOrientAt(3)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(3)][cco.getAt(3)].ordinal]

        res += colorChars[cubeCornerColors[ccp.getAt(1)][R240[cco.getAt(1)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(5)][1-ce.getOrientAt(5)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(5)][R120[cco.getAt(5)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(2)][ce.getOrientAt(2)].ordinal]
        res += 'R'
        res += colorChars[cubeEdgeColors[ce.getPermAt(10)][ce.getOrientAt(10)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(3)][R120[cco.getAt(3)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(7)][1-ce.getOrientAt(7)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(7)][R240[cco.getAt(7)]].ordinal]

        res += colorChars[cubeCornerColors[ccp.getAt(5)][cco.getAt(5)].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(8)][ce.getOrientAt(8)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(4)][cco.getAt(4)].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(10)][1-ce.getOrientAt(10)].ordinal]
        res += 'G'
        res += colorChars[cubeEdgeColors[ce.getPermAt(9)][1-ce.getOrientAt(9)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(7)][cco.getAt(7)].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(11)][ce.getOrientAt(11)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(6)][cco.getAt(6)].ordinal]

        res += colorChars[cubeCornerColors[ccp.getAt(2)][R120[cco.getAt(2)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(3)][1-ce.getOrientAt(3)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(3)][R240[cco.getAt(3)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(6)][ce.getOrientAt(6)].ordinal]
        res += 'W'
        res += colorChars[cubeEdgeColors[ce.getPermAt(7)][ce.getOrientAt(7)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(6)][R240[cco.getAt(6)]].ordinal]
        res += colorChars[cubeEdgeColors[ce.getPermAt(11)][1-ce.getOrientAt(11)].ordinal]
        res += colorChars[cubeCornerColors[ccp.getAt(7)][R120[cco.getAt(7)]].ordinal]
        return res
    }
}

fun rotateDirName(rd : Int) : String {
    return when( kotlin.enums.enumEntries<rotate_dir>()[rd] ) {
        rotate_dir.ORANGECW -> "orange-cw"
        rotate_dir.ORANGE180 -> "orange-180"
        rotate_dir.ORANGECCW -> "orange-ccw"
        rotate_dir.REDCW  -> "red-cw"
        rotate_dir.RED180 -> "red-180"
        rotate_dir.REDCCW -> "red-ccw"
        rotate_dir.YELLOWCW -> "yellow-cw"
        rotate_dir.YELLOW180 -> "yellow-180"
        rotate_dir.YELLOWCCW -> "yellow-ccw"
        rotate_dir.WHITECW -> "white-cw"
        rotate_dir.WHITE180 -> "white-180"
        rotate_dir.WHITECCW -> "white-ccw"
        rotate_dir.GREENCW -> "green-cw"
        rotate_dir.GREEN180 -> "green-180"
        rotate_dir.GREENCCW -> "green-ccw"
        rotate_dir.BLUECW -> "blue-cw"
        rotate_dir.BLUE180 -> "blue-180"
        rotate_dir.BLUECCW -> "blue-ccw"
        else -> "$rd"
    }
}

fun rotateNameToDir(rotateName : String) : Int {
    for(rd in 0 ..< rotate_dir.RCOUNT.ordinal) {
        val dirName = rotateDirName(rd)
        if( rotateName.startsWith(dirName) )
            return rd
    }
    return rotate_dir.RCOUNT.ordinal
}

fun rotateDirReverse(rd : Int) : Int {
    return when( kotlin.enums.enumEntries<rotate_dir>()[rd] ) {
        rotate_dir.ORANGECW -> rotate_dir.ORANGECCW.ordinal
        rotate_dir.ORANGE180 -> rotate_dir.ORANGE180.ordinal
        rotate_dir.ORANGECCW -> rotate_dir.ORANGECW.ordinal
        rotate_dir.REDCW -> rotate_dir.REDCCW.ordinal
        rotate_dir.RED180 -> rotate_dir.RED180.ordinal
        rotate_dir.REDCCW -> rotate_dir.REDCW.ordinal
        rotate_dir.YELLOWCW -> rotate_dir.YELLOWCCW.ordinal
        rotate_dir.YELLOW180 -> rotate_dir.YELLOW180.ordinal
        rotate_dir.YELLOWCCW -> rotate_dir.YELLOWCW.ordinal
        rotate_dir.WHITECW -> rotate_dir.WHITECCW.ordinal
        rotate_dir.WHITE180 -> rotate_dir.WHITE180.ordinal
        rotate_dir.WHITECCW -> rotate_dir.WHITECW.ordinal
        rotate_dir.GREENCW -> rotate_dir.GREENCCW.ordinal
        rotate_dir.GREEN180 -> rotate_dir.GREEN180.ordinal
        rotate_dir.GREENCCW -> rotate_dir.GREENCW.ordinal
        rotate_dir.BLUECW -> rotate_dir.BLUECCW.ordinal
        rotate_dir.BLUE180 -> rotate_dir.BLUE180.ordinal
        rotate_dir.BLUECCW -> rotate_dir.BLUECW.ordinal
        else -> rotate_dir.RCOUNT.ordinal
    }
}

fun transformReverse(idx : Int) : Int {
    return when( idx ) {
        transform_dir.TD_C0_7_CW.ordinal -> transform_dir.TD_C0_7_CCW.ordinal
        transform_dir.TD_C0_7_CCW.ordinal ->  transform_dir.TD_C0_7_CW.ordinal
        transform_dir.TD_C1_6_CW.ordinal ->  transform_dir.TD_C1_6_CCW.ordinal
        transform_dir.TD_C1_6_CCW.ordinal ->  transform_dir.TD_C1_6_CW.ordinal
        transform_dir.TD_C2_5_CW.ordinal ->  transform_dir.TD_C2_5_CCW.ordinal
        transform_dir.TD_C2_5_CCW.ordinal ->  transform_dir.TD_C2_5_CW.ordinal
        transform_dir.TD_C3_4_CW.ordinal ->  transform_dir.TD_C3_4_CCW.ordinal
        transform_dir.TD_C3_4_CCW.ordinal ->  transform_dir.TD_C3_4_CW.ordinal
        transform_dir.TD_BG_CW.ordinal ->  transform_dir.TD_BG_CCW.ordinal
        transform_dir.TD_BG_CCW.ordinal -> transform_dir.TD_BG_CW.ordinal
        transform_dir.TD_YW_CW.ordinal -> transform_dir.TD_YW_CCW.ordinal
        transform_dir.TD_YW_CCW.ordinal -> transform_dir.TD_YW_CW.ordinal
        transform_dir.TD_OR_CW.ordinal -> transform_dir.TD_OR_CCW.ordinal
        transform_dir.TD_OR_CCW.ordinal -> transform_dir.TD_OR_CW.ordinal
        else -> idx
    }
}

fun transformName(td : Int) : String {
    return when( kotlin.enums.enumEntries<transform_dir>()[td] ) {
        transform_dir.TD_0 -> "0"
        transform_dir.TD_C0_7_CW -> "c0-7.cw"
        transform_dir.TD_C0_7_CCW -> "c0-7.ccw"
        transform_dir.TD_C1_6_CW -> "c1-6.cw"
        transform_dir.TD_C1_6_CCW -> "c1-6.ccw"
        transform_dir.TD_C2_5_CW -> "c2-5.cw"
        transform_dir.TD_C2_5_CCW -> "c2-5.ccw"
        transform_dir.TD_C3_4_CW -> "c3-4.cw"
        transform_dir.TD_C3_4_CCW -> "c3-4.ccw"
        transform_dir.TD_BG_CW -> "bg.cw"
        transform_dir.TD_BG_180 -> "bg.180"
        transform_dir.TD_BG_CCW -> "bg.ccw"
        transform_dir.TD_YW_CW -> "yw.cw"
        transform_dir.TD_YW_180 -> "yw.180"
        transform_dir.TD_YW_CCW -> "yw.ccw"
        transform_dir.TD_OR_CW -> "or.cw"
        transform_dir.TD_OR_180 -> "or.180"
        transform_dir.TD_OR_CCW -> "or.ccw"
        transform_dir.TD_E0_11 -> "e0-11"
        transform_dir.TD_E1_10 -> "e1-10"
        transform_dir.TD_E2_9 -> "e2-9"
        transform_dir.TD_E3_8 -> "e3-8"
        transform_dir.TD_E4_7 -> "e4-7"
        transform_dir.TD_E5_6 -> "e5-6"
        else -> "unknown"
    }
}

class cubecorners(corner0perm : Int, corner0orient : Int,
            corner1perm : Int, corner1orient : Int,
            corner2perm : Int, corner2orient : Int,
            corner3perm : Int, corner3orient : Int,
            corner4perm : Int, corner4orient : Int,
            corner5perm : Int, corner5orient : Int,
            corner6perm : Int, corner6orient : Int,
            corner7perm : Int, corner7orient : Int)
{
    var perm = cubecorners_perm(corner0perm, corner1perm, corner2perm, corner3perm,
                    corner4perm, corner5perm, corner6perm, corner7perm)
    var orients = cubecorner_orients(corner0orient, corner1orient, corner2orient,
                    corner3orient, corner4orient, corner5orient, corner6orient, corner7orient)
}

val csolved : cube = cube(
        cubecorners_perm(0, 1, 2, 3, 4, 5, 6, 7),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
)

val crotated : Array<cube> = arrayOf(
    cube(   // ORANGECW
        cubecorners_perm(4, 1, 0, 3, 6, 5, 2, 7),
        cubecorner_orients(1, 0, 2, 0, 2, 0, 1, 0),
        cubeedges(0, 4, 2, 3, 9, 5, 1, 7, 8, 6, 10, 11,
                  0, 1, 0, 0, 1, 0, 1, 0, 0, 1,  0,  0)
    ), cube( // ORANGE180
        cubecorners_perm(6, 1, 4, 3, 2, 5, 0, 7),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(0, 9, 2, 3, 6, 5, 4, 7, 8, 1, 10, 11,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
    ), cube( // ORANGECCW
        cubecorners_perm(2, 1, 6, 3, 0, 5, 4, 7),
        cubecorner_orients(1, 0, 2, 0, 2, 0, 1, 0),
        cubeedges(0, 6, 2, 3, 1, 5, 9, 7, 8, 4, 10, 11,
                  0, 1, 0, 0, 1, 0, 1, 0, 0, 1,  0,  0)
    ), cube( // REDCW
        cubecorners_perm(0, 3, 2, 7, 4, 1, 6, 5),
        cubecorner_orients(0, 2, 0, 1, 0, 1, 0, 2),
        cubeedges(0, 1, 7, 3, 4, 2, 6, 10, 8, 9, 5, 11,
                  0, 0, 1, 0, 0, 1, 0,  1, 0, 0, 1,  0)
    ), cube( // RED180
        cubecorners_perm(0, 7, 2, 5, 4, 3, 6, 1),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(0, 1, 10, 3, 4, 7, 6, 5, 8, 9, 2, 11,
                  0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  0)
    ), cube( // REDCCW
        cubecorners_perm(0, 5, 2, 1, 4, 7, 6, 3),
        cubecorner_orients(0, 2, 0, 1, 0, 1, 0, 2),
        cubeedges(0, 1, 5, 3, 4, 10, 6, 2, 8, 9, 7, 11,
                  0, 0, 1, 0, 0,  1, 0, 1, 0, 0, 1,  0)
    ), cube( // YELLOWCW
        cubecorners_perm(1, 5, 2, 3, 0, 4, 6, 7),
        cubecorner_orients(2, 1, 0, 0, 1, 2, 0, 0),
        cubeedges(5, 1, 2, 3, 0, 8, 6, 7, 4, 9, 10, 11,
                  1, 0, 0, 0, 1, 1, 0, 0, 1, 0,  0,  0)
    ), cube( // YELLOW180
        cubecorners_perm(5, 4, 2, 3, 1, 0, 6, 7),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(8, 1, 2, 3, 5, 4, 6, 7, 0, 9, 10, 11,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
    ), cube( // YELLOWCCW
        cubecorners_perm(4, 0, 2, 3, 5, 1, 6, 7),
        cubecorner_orients(2, 1, 0, 0, 1, 2, 0, 0),
        cubeedges(4, 1, 2, 3, 8, 0, 6, 7, 5, 9, 10, 11,
                  1, 0, 0, 0, 1, 1, 0, 0, 1, 0,  0,  0)
    ), cube( // WHITECW
        cubecorners_perm(0, 1, 6, 2, 4, 5, 7, 3),
        cubecorner_orients(0, 0, 1, 2, 0, 0, 2, 1),
        cubeedges(0, 1, 2, 6, 4, 5, 11, 3, 8, 9, 10, 7,
                  0, 0, 0, 1, 0, 0,  1, 1, 0, 0,  0, 1)
    ), cube( // WHITE180
        cubecorners_perm(0, 1, 7, 6, 4, 5, 3, 2),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(0, 1, 2, 11, 4, 5, 7, 6, 8, 9, 10, 3,
                  0, 0, 0,  0, 0, 0, 0, 0, 0, 0,  0, 0)
    ), cube( // WHITECCW
        cubecorners_perm(0, 1, 3, 7, 4, 5, 2, 6),
        cubecorner_orients(0, 0, 1, 2, 0, 0, 2, 1),
        cubeedges(0, 1, 2, 7, 4, 5, 3, 11, 8, 9, 10, 6,
                  0, 0, 0, 1, 0, 0, 1,  1, 0, 0,  0, 1)
    ), cube( // GREENCW
        cubecorners_perm(0, 1, 2, 3, 5, 7, 4, 6),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 10, 8, 11, 9,
                  0, 0, 0, 0, 0, 0, 0, 0,  1, 1,  1, 1)
    ), cube( // GREEN180
        cubecorners_perm(0, 1, 2, 3, 7, 6, 5, 4),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 11, 10, 9, 8,
                  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0)
    ), cube( // GREENCCW
        cubecorners_perm(0, 1, 2, 3, 6, 4, 7, 5),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 8, 10,
                  0, 0, 0, 0, 0, 0, 0, 0, 1,  1, 1,  1)
    ), cube( // BLUECW
        cubecorners_perm(2, 0, 3, 1, 4, 5, 6, 7),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(1, 3, 0, 2, 4, 5, 6, 7, 8, 9, 10, 11,
                  1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  0, 0)
    ), cube( // BLUE180
        cubecorners_perm(3, 2, 1, 0, 4, 5, 6, 7),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(3, 2, 1, 0, 4, 5, 6, 7, 8, 9, 10, 11,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
    ), cube( // BLUECCW
        cubecorners_perm(1, 3, 0, 2, 4, 5, 6, 7),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(2, 0, 3, 1, 4, 5, 6, 7, 8, 9, 10, 11,
                  1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  0,  0)
    )
)

val ctransformed : Array<cube> = arrayOf(
    csolved,
    cube(   // TD_C0_7_CW
        cubecorners_perm(0, 2, 4, 6, 1, 3, 5, 7),
        cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
        cubeedges(1, 4, 6, 9, 0, 3, 8, 11, 2, 5, 7, 10,
                  0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0,  0)
    ), cube( // TD_C0_7_CCW
        cubecorners_perm(0, 4, 1, 5, 2, 6, 3, 7),
        cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
        cubeedges(4, 0, 8, 5, 1, 9, 2, 10, 6, 3, 11, 7,
                  0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0)
    ), cube( // TD_C1_6_CW
        cubecorners_perm(5, 1, 4, 0, 7, 3, 6, 2),
        cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
        cubeedges(5, 8, 0, 4, 10, 2, 9, 1, 7, 11, 3, 6,
                  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0)
    ), cube( // TD_C1_6_CCW
        cubecorners_perm(3, 1, 7, 5, 2, 0, 6, 4),
        cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
        cubeedges(2, 7, 5, 10, 3, 0, 11, 8, 1, 6, 4, 9,
                  0, 0, 0,  0, 0, 0,  0, 0, 0, 0, 0, 0)
    ), cube( // TD_C2_5_CW
        cubecorners_perm(3, 7, 2, 6, 1, 5, 0, 4),
        cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
        cubeedges(7, 3, 11, 6, 2, 10, 1, 9, 5, 0, 8, 4,
                  0, 0,  0, 0, 0,  0, 0, 0, 0, 0, 0, 0)
    ), cube( // TD_C2_5_CCW
        cubecorners_perm(6, 4, 2, 0, 7, 5, 3, 1),
        cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
        cubeedges(9, 6, 4, 1, 11, 8, 3, 0, 10, 7, 5, 2,
                  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0)
    ), cube( // TD_C3_4_CW
        cubecorners_perm(5, 7, 1, 3, 4, 6, 0, 2),
        cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
        cubeedges(10, 5, 7, 2, 8, 11, 0, 3, 9, 4, 6, 1,
                   0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0)
    ), cube( // TD_C3_4_CCW
        cubecorners_perm(6, 2, 7, 3, 4, 0, 5, 1),
        cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
        cubeedges(6, 11, 3, 7, 9, 1, 10, 2, 4, 8, 0, 5,
                  0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0)
    ), cube( // TD_BG_CW
        cubecorners_perm(1, 3, 0, 2, 5, 7, 4, 6),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(2, 0, 3, 1, 5, 7, 4, 6, 10, 8, 11, 9,
                  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1)
    ), cube( // TD_BG_180
        cubecorners_perm(3, 2, 1, 0, 7, 6, 5, 4),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8,
                  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0)
    ), cube( // TD_BG_CCW
        cubecorners_perm(2, 0, 3, 1, 6, 4, 7, 5),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(1, 3, 0, 2, 6, 4, 7, 5, 9, 11, 8, 10,
                  1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1,  1)
    ), cube( // TD_YW_CW
        cubecorners_perm(4, 0, 6, 2, 5, 1, 7, 3),
        cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
        cubeedges(4, 9, 1, 6, 8, 0, 11, 3, 5, 10, 2, 7,
                  1, 1, 1, 1, 1, 1,  1, 1, 1,  1, 1, 1)
    ), cube( // TD_YW_180
        cubecorners_perm(5, 4, 7, 6, 1, 0, 3, 2),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(8, 10, 9, 11, 5, 4, 7, 6, 0, 2, 1, 3,
                  0,  0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0)
    ), cube( // TD_YW_CCW
        cubecorners_perm(1, 5, 3, 7, 0, 4, 2, 6),
        cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
        cubeedges(5, 2, 10, 7, 0, 8, 3, 11, 4, 1, 9, 6,
                  1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1, 1)
    ), cube( // TD_OR_CW
        cubecorners_perm(4, 5, 0, 1, 6, 7, 2, 3),
        cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
        cubeedges(8, 4, 5, 0, 9, 10, 1, 2, 11, 6, 7, 3,
                  1, 1, 1, 1, 1,  1, 1, 1,  1, 1, 1, 1)
    ), cube( // TD_OR_180
        cubecorners_perm(6, 7, 4, 5, 2, 3, 0, 1),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(11, 9, 10, 8, 6, 7, 4, 5, 3, 1, 2, 0,
                   0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    ), cube( // TD_OR_CCW
        cubecorners_perm(2, 3, 6, 7, 0, 1, 4, 5),
        cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
        cubeedges(3, 6, 7, 11, 1, 2, 9, 10, 0, 4, 5, 8,
                  1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1)
    ), cube( // TD_E0_11
        cubecorners_perm(1, 0, 5, 4, 3, 2, 7, 6),
        cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
        cubeedges(0, 5, 4, 8, 2, 1, 10, 9, 3, 7, 6, 11,
                  1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1,  1)
    ), cube( // TD_E1_10
        cubecorners_perm(2, 6, 0, 4, 3, 7, 1, 5),
        cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
        cubeedges(6, 1, 9, 4, 3, 11, 0, 8, 7, 2, 10, 5,
                  1, 1, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1)
    ), cube( // TD_E2_9
        cubecorners_perm(7, 3, 5, 1, 6, 2, 4, 0),
        cubecorner_orients(2, 1, 1, 2, 1, 2, 2, 1),
        cubeedges(7, 10, 2, 5, 11, 3, 8, 0, 6, 9, 1, 4,
                  1,  1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1)
    ), cube( // TD_E3_8
        cubecorners_perm(7, 6, 3, 2, 5, 4, 1, 0),
        cubecorner_orients(1, 2, 2, 1, 2, 1, 1, 2),
        cubeedges(11, 7, 6, 3, 10, 9, 2, 1, 8, 5, 4, 0,
                   1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1)
    ), cube( // TD_E4_7
        cubecorners_perm(4, 6, 5, 7, 0, 2, 1, 3),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(9, 8, 11, 10, 4, 6, 5, 7, 1, 0, 3, 2,
                  1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
    ), cube( // TD_E5_6
        cubecorners_perm(7, 5, 6, 4, 3, 1, 2, 0),
        cubecorner_orients(0, 0, 0, 0, 0, 0, 0, 0),
        cubeedges(10, 11, 8, 9, 7, 5, 6, 4, 2, 3, 0, 1,
                   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
    )
)

fun cubePrint(c : cube) {
    val colorPrint : Array<String> = arrayOf(
        "\u001B[48;2;230;230;0m  \u001B[m",     // CYELLOW
        "\u001B[48;2;230;148;0m  \u001B[m",     // CORANGE
        "\u001B[48;2;0;0;230m  \u001B[m",       // CBLUE
        "\u001B[48;2;230;0;0m  \u001B[m",       // CRED
        "\u001B[48;2;0;230;0m  \u001B[m",       // CGREEN
        "\u001B[48;2;230;230;230m  \u001B[m",   // CWHITE
    )

    println()
    println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(4)][R120[c.cco.getAt(4)]].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(8)][1-c.ce.getOrientAt(8)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(5)][R240[c.cco.getAt(5)]].ordinal])
    println("        " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(4)][c.ce.getOrientAt(4)].ordinal] +
            colorPrint[cubecolor.CYELLOW.ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(5)][c.ce.getOrientAt(5)].ordinal])
    println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(0)][R240[c.cco.getAt(0)]].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(0)][1-c.ce.getOrientAt(0)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(1)][R120[c.cco.getAt(1)]].ordinal])
    println()
    println(" " +
            colorPrint[cubeCornerColors[c.ccp.getAt(4)][R240[c.cco.getAt(4)]].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(4)][1-c.ce.getOrientAt(4)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(0)][R120[c.cco.getAt(0)]].ordinal] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(0)][c.cco.getAt(0)].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(0)][c.ce.getOrientAt(0)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(1)][c.cco.getAt(1)].ordinal] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(1)][R240[c.cco.getAt(1)]].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(5)][1-c.ce.getOrientAt(5)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(5)][R120[c.cco.getAt(5)]].ordinal] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(5)][c.cco.getAt(5)].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(8)][c.ce.getOrientAt(8)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(4)][c.cco.getAt(4)].ordinal])
    println(" " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(9)][c.ce.getOrientAt(9)].ordinal] +
            colorPrint[cubecolor.CORANGE.ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(1)][c.ce.getOrientAt(1)].ordinal] + " " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(1)][1-c.ce.getOrientAt(1)].ordinal] +
            colorPrint[cubecolor.CBLUE.ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(2)][1-c.ce.getOrientAt(2)].ordinal] + " " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(2)][c.ce.getOrientAt(2)].ordinal] +
            colorPrint[cubecolor.CRED.ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(10)][c.ce.getOrientAt(10)].ordinal] + " " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(10)][1-c.ce.getOrientAt(10)].ordinal] +
            colorPrint[cubecolor.CGREEN.ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(9)][1-c.ce.getOrientAt(9)].ordinal])
    println(" " +
            colorPrint[cubeCornerColors[c.ccp.getAt(6)][R120[c.cco.getAt(6)]].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(6)][1-c.ce.getOrientAt(6)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(2)][R240[c.cco.getAt(2)]].ordinal] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(2)][c.cco.getAt(2)].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(3)][c.ce.getOrientAt(3)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(3)][c.cco.getAt(3)].ordinal] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(3)][R120[c.cco.getAt(3)]].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(7)][1-c.ce.getOrientAt(7)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(7)][R240[c.cco.getAt(7)]].ordinal] + " " +
            colorPrint[cubeCornerColors[c.ccp.getAt(7)][c.cco.getAt(7)].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(11)][c.ce.getOrientAt(11)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(6)][c.cco.getAt(6)].ordinal])
    println()
    println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(2)][R120[c.cco.getAt(2)]].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(3)][1-c.ce.getOrientAt(3)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(3)][R240[c.cco.getAt(3)]].ordinal])
    println("        " +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(6)][c.ce.getOrientAt(6)].ordinal] +
            colorPrint[cubecolor.CWHITE.ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(7)][c.ce.getOrientAt(7)].ordinal])
    println("        " +
            colorPrint[cubeCornerColors[c.ccp.getAt(6)][R240[c.cco.getAt(6)]].ordinal] +
            colorPrint[cubeEdgeColors[c.ce.getPermAt(11)][1-c.ce.getOrientAt(11)].ordinal] +
            colorPrint[cubeCornerColors[c.ccp.getAt(7)][R120[c.cco.getAt(7)]].ordinal])
    println()
}
