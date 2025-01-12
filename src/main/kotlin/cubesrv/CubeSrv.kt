package cubesrv

private class DepthMaxByMem(val memMinMB : Int, val depthMax : Int, val useReverse : Boolean)

private val depthMaxByMem = arrayOf(
    DepthMaxByMem(memMinMB =        0, depthMax =  8, useReverse = true  ), // ~320MB
//  DepthMaxByMem(memMinMB =      768, depthMax =  8, useReverse = false ), slower than 8 rev; ~600MB
    DepthMaxByMem(memMinMB =     2700, depthMax =  9, useReverse = true  ), // ~2.4GB
    DepthMaxByMem(memMinMB =     5120, depthMax =  9, useReverse = false ), // ~4.8GB
    DepthMaxByMem(memMinMB = 1 shl 30, depthMax = 10, useReverse = true  ), // ~27GB
    DepthMaxByMem(memMinMB = 1 shl 30, depthMax = 10, useReverse = false ), // ~53GB
)

private fun depthMaxSelFn() : Int {
    val memSizeMB = Runtime.getRuntime().maxMemory() / 1048576
    var model = 0
    while( depthMaxByMem[model+1].memMinMB <= memSizeMB )
        ++model
    println("mem MB: $memSizeMB, model: $model")
    return model
}

private fun printUsage() {
    println("""
usage:
    cubesrv                                  start web server at port 8080
    cubesrv <maxdepth>                       ditto
    cubesrv <maxdepth> <cubecount>  <mode>   solve random cubes
    cubesrv <maxdepth> fname.txt    <mode>   solve cubes from file
    cubesrv <maxdepth> <cubestring> <mode>   solve the cube

parameters:
    <maxdepth>          1..10 or 1r..10r
    <cubecount>         a number
    <mode>              o - optimal, q - quick, m - quick multi,
                        O - optimal with preload

""")
}

fun main(args : Array<String>) {
    val memModel = depthMaxSelFn()
    var depthMax = depthMaxByMem[memModel].depthMax
    var useReverse = depthMaxByMem[memModel].useReverse

    if( args.size >= 1 ) {
        if( args[0] == "-h" || args[0] == "--help") {
            printUsage()
            return
        }
        depthMax = "\\d+".toRegex().find(args[0])!!.value.toInt()
        useReverse = args[0].contains("r")
        if( depthMax <= 0 || depthMax > 99 ) {
            println("DEPTH_MAX invalid: must be in range 1..99")
            return
        }
    }
    println("setup: depth $depthMax${if(useReverse) " rev" else ""}")
    if( args.size >= 3 ) {
        if(args[1][0].isDigit())
            cubeTester(args[1].toInt(), args[2], depthMax, useReverse)
        else
            solveCubes(args[1], args[2], depthMax, useReverse)
    }else
        runServer(depthMax, useReverse)
}
