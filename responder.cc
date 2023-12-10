#include "responder.h"
#include <cstdio>
#include <stdarg.h>

void Responder::message(const char *fmt, ...) {
    char msg[2000];
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);
    handleMessage(MT_UNQUALIFIED, msg);
}

void Responder::progress(const char *fmt, ...) {
    char msg[2000];
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);
    handleMessage(MT_PROGRESS, msg);
}

void Responder::movecount(const char *fmt, ...) {
    char msg[2000];
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);
    handleMessage(MT_MOVECOUNT, msg);
}

void Responder::solution(const char *fmt, ...) {
    char msg[2000];
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);
    handleMessage(MT_SOLUTION, msg);
}

