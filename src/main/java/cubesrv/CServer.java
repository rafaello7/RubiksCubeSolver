package cubesrv;

import static cubesrv.CubeDefs.*;
import static cubesrv.CubeRead.*;
import static cubesrv.CubeSearch.*;
import static cubesrv.ThreadPoolHelper.*;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.util.concurrent.locks.ReentrantLock;

public class CServer {

    private static class SocketChunkedResponder extends Responder {
        private final OutputStream m_fdReq;
        private final ReentrantLock respLock = new ReentrantLock();

        SocketChunkedResponder(OutputStream fdReq) { m_fdReq = fdReq; }

        @Override
        public void handleMessage(MessageType mt, String msg) {
            String prefix = switch (mt) {
                case MT_UNQUALIFIED -> "";
                case MT_PROGRESS    -> "progress: ";
                case MT_MOVECOUNT   -> "moves: ";
                case MT_SOLUTION    -> "solution:";
            };
            String respBuf = prefix + msg + "\n";
            respLock.lock();
            try {
                m_fdReq.write(respBuf.getBytes());
                m_fdReq.flush();
            } catch (IOException e) {
                System.out.println("error: socket write in responder");
            } finally {
                respLock.unlock();
            }
        }
    }

    private static final ReentrantLock solverLock = new ReentrantLock();

    private static void processCubeReq(HttpExchange exchange, CubeSearcher cubeSearcher) throws IOException {
        String query = exchange.getRequestURI().getQuery();
        if (query.startsWith("stop")) {
            ProgressBase.requestStop();
            exchange.sendResponseHeaders(200, -1);
            return;
        }
        if (solverLock.tryLock()) {
            try {
                ProgressBase.requestRestart();
                exchange.getResponseHeaders().add("Content-type", "text/plain");
                exchange.sendResponseHeaders(200, 0);
                try (OutputStream fdReq = exchange.getResponseBody()) {
                    SocketChunkedResponder responder = new SocketChunkedResponder(fdReq);
                    String mode = query.substring(0, 1);
                    if (query.length() > 1 && query.charAt(1) == '=') {
                        cube[] c = {new cube()};
                        if (cubeFromColorsOnSquares(responder, query.substring(2), c)) {
                            responder.message("thread count: " + THREAD_COUNT);
                            if (c[0].equals(csolved))
                                responder.message("already solved");
                            else
                                cubeSearcher.searchMoves(c[0], mode, responder);
                        }
                    } else {
                        responder.message("invalid parameter");
                    }
                }
            } finally {
                solverLock.unlock();
            }
            System.out.println("solver end");
        } else {
            exchange.getResponseHeaders().add("Content-type", "text/plain");
            byte[] resp = "setup: the solver is busy\n".getBytes();
            exchange.sendResponseHeaders(200, resp.length);
            try (OutputStream fdReq = exchange.getResponseBody()) {
                fdReq.write(resp);
            }
            System.out.println("solver busy");
        }
    }

    private static void getFile(HttpExchange exchange) throws IOException {
        String fname = exchange.getRequestURI().getPath().substring(1);
        if (fname.isEmpty()) fname = "cube.html";
        InputStream fp = Thread.currentThread().getContextClassLoader().getResourceAsStream(fname);
        if (fp != null) {
            try {
                int dotPos = fname.lastIndexOf('.');
                String fnameExt = (dotPos < 0) ? "txt" : fname.substring(dotPos+1);
                String contentType = "application/octet-stream";
                if (fnameExt.equalsIgnoreCase("html")) contentType = "text/html; charset=utf-8";
                else if (fnameExt.equalsIgnoreCase("css")) contentType = "text/css";
                else if (fnameExt.equalsIgnoreCase("js"))  contentType = "text/javascript";
                else if (fnameExt.equalsIgnoreCase("txt")) contentType = "text/plain";
                exchange.getResponseHeaders().add("Content-type", contentType);
                exchange.sendResponseHeaders(200, 0);
                try (OutputStream fdReq = exchange.getResponseBody()) {
                    fp.transferTo(fdReq);
                }
            } catch (IOException exc) {
                System.out.println("error: socket write");
            } finally {
                fp.close();
            }
        } else {
            exchange.sendResponseHeaders(404, -1);
        }
    }

    private static class ConnectionHandler implements HttpHandler {
        private final CubeSearcher cubeSearcher;
        ConnectionHandler(CubeSearcher cs) { cubeSearcher = cs; }

        @Override
        public void handle(HttpExchange exchange) {
            try {
                String method = exchange.getRequestMethod();
                var uri = exchange.getRequestURI();
                System.out.println(method + " " + uri);
                if (!method.equalsIgnoreCase("get")) {
                    exchange.sendResponseHeaders(405, -1);
                    return;
                }
                if (uri.getPath().equals("/") && uri.getQuery() != null)
                    processCubeReq(exchange, cubeSearcher);
                else
                    getFile(exchange);
            } catch (Exception exc) {
                System.out.println("exception: " + exc);
                exc.printStackTrace();
            }
        }
    }

    public static void runServer(int depthMax, boolean useReverse) throws IOException {
        CubeSearcher cubeSearcher = new CubeSearcher(depthMax, useReverse);
        HttpServer server = HttpServer.create(new InetSocketAddress(8080), 0);
        server.createContext("/", new ConnectionHandler(cubeSearcher));
        server.start();
    }
}

