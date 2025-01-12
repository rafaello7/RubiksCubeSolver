package cubesrv

abstract class Responder {
    enum class MessageType {
        MT_UNQUALIFIED,
        MT_PROGRESS,
        MT_MOVECOUNT,
        MT_SOLUTION
    }
    abstract fun handleMessage(mt : MessageType, msg : String)

    fun message(msg : String) : Unit {
        handleMessage(MessageType.MT_UNQUALIFIED, msg);
    }

    fun progress(msg : String) : Unit {
        handleMessage(MessageType.MT_PROGRESS, msg);
    }

    fun movecount(msg : String) : Unit {
        handleMessage(MessageType.MT_MOVECOUNT, msg);
    }

    fun solution(msg : String) : Unit {
        handleMessage(MessageType.MT_SOLUTION, msg);
    }
}

