package cubesrv

import com.sun.net.httpserver.HttpExchange
import com.sun.net.httpserver.HttpServer
import com.sun.net.httpserver.HttpHandler
import java.io.File
import java.io.FileInputStream
import java.io.InputStream
import java.io.IOException
import java.io.OutputStream
import java.net.InetSocketAddress
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.sync.Mutex

private class SocketChunkedResponder(val m_fdReq : OutputStream) : Responder() {
    private val respMutex = Mutex()
    override fun handleMessage(mt : MessageType, msg : String) {
        val prefix : String = when(mt) {
            MessageType.MT_UNQUALIFIED -> ""
            MessageType.MT_PROGRESS    -> "progress: "
            MessageType.MT_MOVECOUNT   -> "moves: "
            MessageType.MT_SOLUTION    -> "solution:"
        }
        val respBuf = "%s%s\n".format(prefix, msg)
        runBlocking {
            respMutex.lock()
            try {
                m_fdReq.write(respBuf.toByteArray())
                m_fdReq.flush()
            }finally{
                respMutex.unlock()
            }
        }
    }
}

private val solverMutex = Mutex()
private fun processCubeReq(exchange : HttpExchange, cubeSearcher : CubeSearcher) {
    val query = exchange.getRequestURI().getQuery()
    if( query.startsWith("stop") ) {
        runBlocking {
            ProgressBase.requestStop()
        }
        exchange.sendResponseHeaders(200, -1)
        return
    }
    if( solverMutex.tryLock() ) {
        runBlocking {
            ProgressBase.requestRestart()
        }
        val headers = exchange.getResponseHeaders()
        headers.add("Content-type", "text/plain")
        exchange.sendResponseHeaders(200, 0)
        val fdReq = exchange.getResponseBody()
        val responder = SocketChunkedResponder(fdReq)
        val mode  : String = query.substring(0, 1)
        if( query[1] == '=' ) {
            val c = arrayOf(cube())
            if( cubeFromColorsOnSquares(responder, query.substring(2), c) ) {
                responder.message("thread count: $THREAD_COUNT")
                if( c[0] == csolved )
                    responder.message("already solved")
                else
                    cubeSearcher.searchMoves(c[0], mode, responder)
            }
        }else{
            responder.message("invalid parameter")
        }
        fdReq.close()
        solverMutex.unlock()
        println("solver end")
    }else{
        val headers = exchange.getResponseHeaders()
        headers.add("Content-type", "text/plain")
        val resp = "setup: the solver is busy\n".toByteArray()
        exchange.sendResponseHeaders(200, resp.size.toLong())
        val fdReq = exchange.getResponseBody()
        fdReq.write(resp)
        fdReq.close()
        println("solver busy")
    }
}

private fun getFile(exchange : HttpExchange) {
    var fname = exchange.getRequestURI().getPath().substring(1)
    if( fname.isEmpty() )
        fname = "cube.html"
    Thread.currentThread().getContextClassLoader().getResourceAsStream(fname).use { fp ->
        if( fp != null ) {
            val dotPos = fname.lastIndexOf(".")
            val fnameExt = if( dotPos < 0 ) "txt" else fname.substring(dotPos+1)
            var contentType = "application/octet-stream"
            if( fnameExt.equals("html", ignoreCase = true) )
                contentType = "text/html; charset=utf-8"
            else if( fnameExt.equals("css", ignoreCase = true) )
                contentType = "text/css"
            else if( fnameExt.equals("js", ignoreCase = true) )
                contentType = "text/javascript"
            else if( fnameExt.equals("txt", ignoreCase = true) )
                contentType = "text/plain"
            val headers = exchange.getResponseHeaders()
            headers.add("Content-type", contentType)
            exchange.sendResponseHeaders(200, 0)
            val fdReq = exchange.getResponseBody()
            try{
                fp.copyTo(fdReq)
                fdReq.close()
            }catch(exc : IOException) {
                println("error: socket write")
                return
            }
        }else{
            exchange.sendResponseHeaders(404, -1)
        }
    }
}

private class ConnectionHandler(val cubeSearcher : CubeSearcher) : HttpHandler {
    override fun handle(exchange : HttpExchange) {
        try {
            val method = exchange.getRequestMethod()
            val uri = exchange.getRequestURI()
            println("$method $uri")
            if( !method.equals("get", ignoreCase = true) ) {
                exchange.sendResponseHeaders(405, -1)
                return
            }
            if( uri.getPath().equals("/") && uri.getQuery() != null )
                processCubeReq(exchange, cubeSearcher)
            else
                getFile(exchange)
        }catch(exc : Exception) {
            println("exception: $exc")
            exc.printStackTrace()
        }
    }
}

fun runServer(depthMax : Int, useReverse : Boolean) {
    val cubeSearcher = CubeSearcher(depthMax, useReverse)
    val server = HttpServer.create(InetSocketAddress(8080), 0);
    server.createContext("/", ConnectionHandler(cubeSearcher));
    server.start();
}
