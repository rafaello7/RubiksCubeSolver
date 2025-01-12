package cubesrv

import java.io.File
import kotlin.random.Random
import kotlin.time.TimeSource

private fun generateCube() : cube {
    val c = cube(
        ccp = cubecorners_perm.fromPermIdx(Random.nextInt(40320)),
        cco = cubecorner_orients.fromOrientIdx(Random.nextInt(2187)),
        ce = cubeedges.fromPermAndOrientIdx(Random.nextInt(479001600), Random.nextInt(2048)))
    if( c.ccp.isPermParityOdd() != c.ce.isPermParityOdd() ) {
        val p = c.ce.getPermAt(10)
        c.ce.setPermAt(10, c.ce.getPermAt(11))
        c.ce.setPermAt(11, p)
    }
    return c
}

class ConsoleResponder(verboseLevel : Int) : Responder() {
    val m_verboseLevel = verboseLevel + 1
    var m_solution : String? = null
    var m_movecount : String? = null
    val m_startTime = TimeSource.Monotonic.markNow()

    fun durationTimeMs() : Int {
        val dur = m_startTime.elapsedNow()
        return dur.inWholeMilliseconds.toInt()
    }

    fun durationTime() : String {
        var ms = durationTimeMs()
        val minutes = ms / 60000
        ms %= 60000
        val seconds = ms / 1000
        ms %= 1000
        var res = if( minutes != 0 )
            "$minutes:%02d".format(seconds)
        else
            "$seconds"
        res += ".%03d".format(ms)
        return res
    }

    override fun handleMessage(mt : MessageType, msg : String) {
        var pad : String = "                                                 "
        pad = pad.substring(minOf(msg.length, pad.length))
        when( mt ) {
            MessageType.MT_UNQUALIFIED -> {
                if( m_verboseLevel > 0 ) {
                    print("\r${durationTime()} $msg $pad")
                    if( msg.startsWith("finished at ") || m_verboseLevel >= 3 )
                        println()
                }
            }
            MessageType.MT_PROGRESS -> {
                if( m_verboseLevel > 1 ) {
                    print("\r${durationTime()} $msg $pad")
                }
            }
            MessageType.MT_MOVECOUNT -> {
                println("\r${durationTime()} moves: $msg $pad")
                m_movecount = msg
            }
            MessageType.MT_SOLUTION -> {
                println("\r${durationTime()} $msg $pad")
                m_solution = msg
            }
        }
    }

    fun getSolution() : String? = m_solution
    fun getMoveCount() : String?  = m_movecount
}

private fun solveCubes(cubes : List<cube>, mode : String, depthMax : Int, useReverse : Boolean) {
    val cubeSearcher = CubeSearcher(depthMax, useReverse)
    if( mode == "O" ) {
        val responder = ConsoleResponder(2)
        cubeSearcher.fillCubes(responder)
        println()
    }
    val moveCounters = mutableMapOf<String, Array<Int>>()
    for(i in 0 ..< cubes.size) {
        var c : cube = cubes[i]
        println("$i  ${c.toParamText()}")
        println()
        cubePrint(c)
        val responder = ConsoleResponder(2) //when(mode) {"q" -> 0 "m" -> 1 else -> 2})
        cubeSearcher.searchMoves(c, mode, responder)
        println()
        val solution : String = responder.getSolution() ?: ""
        for(s in solution.split(" ")) {
            if( !s.isEmpty() ) {
                val rd : Int = rotateNameToDir(s)
                if( rd == rotate_dir.RCOUNT.ordinal ) {
                    println("fatal: unrecognized move $s")
                    return
                }
                c = cube.compose(c, crotated[rd])
            }
        }
        if( c != csolved ) {
            println("fatal: bad solution!")
            return
        }
        val moveCount : String? = responder.getMoveCount()
        if( moveCount != null ) {
            val ent : Array<Int>? = moveCounters[moveCount]
            if( ent == null ) {
                moveCounters[moveCount] = arrayOf(1, responder.durationTimeMs())
            }else{
                ++ent[0]
                ent[1] += responder.durationTimeMs()
            }
        }
    }
    var durationTimeTot : Int = 0
    println("          moves  cubes  avg time (s)")
    println("---------------  -----  -------------")
    for(moveCount in moveCounters.keys.sortedDescending()) {
        val stats = moveCounters[moveCount] ?: arrayOf(0)
        println("%15s %6d  %.4f".format(moveCount, stats[0], stats[1]/1000.0/stats[0]))
        durationTimeTot += stats[1]
    }
    println("---------------  -----  -------------")
    println("total %7.0f s %6d  %.4f".format(durationTimeTot/1000.0, cubes.size,
            durationTimeTot/1000.0/cubes.size))
    println()
}

fun solveCubes(fnameOrCubeStr : String, mode : String, depthMax : Int, useReverse : Boolean) {
    val cubes = mutableListOf<cube>()
    val responder = ConsoleResponder(2)
    if( fnameOrCubeStr.endsWith(".txt") ) {
        File(fnameOrCubeStr).forEachLine() { line ->
            val c = arrayOf( cube() )
            if( cubeFromString(responder, line, c) )
                cubes.add(c[0])
        }
    }else{
        val c = arrayOf( cube() )
        if( !cubeFromString(responder, fnameOrCubeStr, c) )
            return
        cubes.add(c[0])
    }
    solveCubes(cubes, mode, depthMax, useReverse)
}

fun cubeTester(cubeCount : Int, mode : String, depthMax : Int, useReverse : Boolean) {
    val cubes = mutableListOf<cube>()
    for(i in 0 ..< cubeCount)
        cubes.add(generateCube())
    solveCubes(cubes, mode, depthMax, useReverse)
}
