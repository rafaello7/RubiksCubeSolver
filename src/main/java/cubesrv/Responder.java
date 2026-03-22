package cubesrv;

public abstract class Responder {
    public enum MessageType {
        MT_UNQUALIFIED,
        MT_PROGRESS,
        MT_MOVECOUNT,
        MT_SOLUTION
    }

    public abstract void handleMessage(MessageType mt, String msg);

    public void message(String msg) {
        handleMessage(MessageType.MT_UNQUALIFIED, msg);
    }

    public void progress(String msg) {
        handleMessage(MessageType.MT_PROGRESS, msg);
    }

    public void movecount(String msg) {
        handleMessage(MessageType.MT_MOVECOUNT, msg);
    }

    public void solution(String msg) {
        handleMessage(MessageType.MT_SOLUTION, msg);
    }
}

