#include "cubedefs.h"
#include "cserver.h"
#include "responder.h"
#include "progressbase.h"
#include "cuberead.h"
#include "threadpoolhelper.h"
#include "cubesearch.h"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

class SocketChunkedResponder : public Responder {
    const int m_fdReq;

    void handleMessage(MessageType, const char*);
public:
    explicit SocketChunkedResponder(int fdReq)
        : m_fdReq(fdReq)
    {
    }
};

void SocketChunkedResponder::handleMessage(MessageType mt, const char *msg) {
    const char *prefix = "";
    switch(mt) {
    case MT_UNQUALIFIED:
        break;
    case MT_PROGRESS:
        prefix = "progress: ";
        break;
    case MT_MOVECOUNT:
        prefix = "moves: ";
        break;
    case MT_SOLUTION:
        prefix = "solution:";
        break;
    }
    char respBuf[2048];
    unsigned msgLen = strlen(prefix) + strlen(msg) + 1;
    sprintf(respBuf, "%x\r\n%s%s\n\r\n", msgLen, prefix, msg);
    write(m_fdReq, respBuf, strlen(respBuf));
}

static void processCubeReq(int fdReq, const char *reqParam, unsigned depthMax)
{
    static std::mutex solverMutex;

    if( !strncmp(reqParam, "stop", 4) ) {
        ProgressBase::requestStop();
        char respHeader[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-length: 0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        write(fdReq, respHeader, strlen(respHeader));
        return;
    }
    if( solverMutex.try_lock() ) {
        ProgressBase::requestRestart();
        char respHeader[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-type: text/plain\r\n"
            "Transfer-encoding: chunked\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        write(fdReq, respHeader, strlen(respHeader));
        SocketChunkedResponder responder(fdReq);
        char mode = reqParam[0];
        if( reqParam[1] == '=' ) {
            cube c;
            if( cubeFromColorsOnSquares(responder, reqParam+2, c) ) {
                responder.message("thread count: %d", THREAD_COUNT);
                if( c == csolved )
                    responder.message("already solved");
                else
                    searchMoves(c, mode, depthMax, responder);
            }
        }else{
            responder.message("invalid parameter");
        }
        write(fdReq, "0\r\n\r\n", 5);
        solverMutex.unlock();
        printf("%d solver end\n", fdReq);
    }else{
        char respHeader[] =
                "HTTP/1.1 200 OK\r\n"
                "Content-type: text/plain\r\n"
                "Content-length: 26\r\n"
                "Connection: keep-alive\r\n"
                "\r\n"
                "setup: the solver is busy\n";
        write(fdReq, respHeader, strlen(respHeader));
        printf("%d solver busy\n", fdReq);
    }
}

static void getFile(int fdReq, const char *fnameBeg) {
    const char *fnameEnd = strchr(fnameBeg, ' ');
    if( fnameEnd != NULL ) {
        std::string fname;
        fname.assign(fnameBeg, fnameEnd - fnameBeg);
        if( fname.empty())
            fname = "cube.html";
        FILE *fp = fopen(fname.c_str(), "r");
        if( fp != NULL ) {
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
                "Connection: keep-alive\r\n"
                "\r\n", contentType, fsize);
            write(fdReq, respHeader, strlen(respHeader));
            rewind(fp);
            char fbuf[32768];
            long toWr = fsize;
            while( toWr > 0 ) {
                int toRd = (unsigned)toWr > sizeof(fbuf) ? sizeof(fbuf) : toWr;
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
            return;
        }
    }
    char respHeader[] =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-length: 0\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    write(fdReq, respHeader, strlen(respHeader));
}

static void processRequest(int fdReq, const char *reqHeader, unsigned depthMax) {
    if( strncasecmp(reqHeader, "get ", 4) ) {
        char respHeader[] =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-length: 0\r\n"
            "Allow: GET\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        write(fdReq, respHeader, strlen(respHeader));
        return;
    }
    if( reqHeader[4] == '/' ) {
        if( reqHeader[5] == '?' )
            processCubeReq(fdReq, reqHeader+6, depthMax);
        else
            getFile(fdReq, reqHeader+5);
    }else{
        char respHeader[] =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-length: 0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        write(fdReq, respHeader, strlen(respHeader));
    }
}

static void processConnection(int fdConn, unsigned depthMax) {
    char reqHeaderBuf[16385], *reqHeaderEnd;
    int rdTot = 0;
    while( true ) {
        int rd = read(fdConn, reqHeaderBuf + rdTot, sizeof(reqHeaderBuf) - rdTot - 1);
        if( rd < 0 ) {
            perror("read");
            break;
        }
        if( rd == 0 ) {
            printf("%d closed\n", fdConn);
            break;
        }
        //write(STDOUT_FILENO, reqHeaderBuf+rdTot, rd);
        rdTot += rd;
        reqHeaderBuf[rdTot] = '\0';
        if( (reqHeaderEnd = strstr(reqHeaderBuf, "\r\n\r\n")) != NULL ) {
            printf("%d %.*s\n", fdConn,
                    (unsigned)(strchr(reqHeaderBuf, '\n') - reqHeaderBuf), reqHeaderBuf);
            reqHeaderEnd[2] = '\0';
            processRequest(fdConn, reqHeaderBuf, depthMax);
            unsigned reqHeaderSize = reqHeaderEnd - reqHeaderBuf + 4;
            memmove(reqHeaderBuf, reqHeaderEnd+4, rdTot - reqHeaderSize);
            rdTot -= reqHeaderSize;
        }
        if( rdTot == sizeof(reqHeaderBuf)-1 ) {
            fprintf(stderr, "%d request header too long\n", fdConn);
            break;
        }
    }
    close(fdConn);
}

void runServer(unsigned depthMax)
{
    int listenfd, acceptfd, opton = 1;
    struct sockaddr_in addr;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket");
        return;
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opton, sizeof(opton));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    if( bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0 ) {
        perror("bind");
        return;
    }
    if( listen(listenfd, 1) < 0 ) {
        perror("listen");
        return;
    }
    while( (acceptfd = accept(listenfd, NULL, NULL)) >= 0 ) {
        //setsockopt(acceptfd, IPPROTO_TCP, TCP_NODELAY, &opton, sizeof(opton));
        std::thread t = std::thread(processConnection, acceptfd, depthMax);
        t.detach();
    }
    perror("accept");
    exit(1);
}

