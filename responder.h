#ifndef RESPONDER_H
#define RESPONDER_H

class Responder {
public:
    void message(const char *fmt, ...);
    void progress(const char *fmt, ...);
    void movecount(const char *fmt, ...);
    void solution(const char *fmt, ...);
protected:
    enum MessageType {
        MT_UNQUALIFIED,
        MT_PROGRESS,
        MT_MOVECOUNT,
        MT_SOLUTION
    };
    virtual void handleMessage(MessageType, const char*) = 0;
};

#endif // RESPONDER_H
