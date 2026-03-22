package cubesrv;

public class ConsoleResponder extends Responder {
    private final int m_verboseLevel;
    String m_solution = null;
    String m_movecount = null;
    private final long m_startNano = System.nanoTime();

    public ConsoleResponder(int verboseLevel) {
        m_verboseLevel = verboseLevel + 1;
    }

    public int durationTimeMs() {
        return (int) ((System.nanoTime() - m_startNano) / 1_000_000L);
    }

    public String durationTime() {
        int ms = durationTimeMs();
        int minutes = ms / 60000;
        ms %= 60000;
        int seconds = ms / 1000;
        ms %= 1000;
        String res = (minutes != 0)
                ? minutes + ":" + String.format("%02d", seconds)
                : String.valueOf(seconds);
        res += String.format(".%03d", ms);
        return res;
    }

    @Override
    public void handleMessage(MessageType mt, String msg) {
        String pad = "                                                 ";
        pad = pad.substring(Math.min(msg.length(), pad.length()));
        switch (mt) {
            case MT_UNQUALIFIED -> {
                if (m_verboseLevel > 0) {
                    System.out.print("\r" + durationTime() + " " + msg + " " + pad);
                    if (msg.startsWith("finished at ") || m_verboseLevel >= 3)
                        System.out.println();
                }
            }
            case MT_PROGRESS -> {
                if (m_verboseLevel > 1)
                    System.out.print("\r" + durationTime() + " " + msg + " " + pad);
            }
            case MT_MOVECOUNT -> {
                System.out.println("\r" + durationTime() + " moves: " + msg + " " + pad);
                m_movecount = msg;
            }
            case MT_SOLUTION -> {
                System.out.println("\r" + durationTime() + " " + msg + " " + pad);
                m_solution = msg;
            }
        }
    }

    public String getSolution() {
        return m_solution;
    }

    public String getMoveCount() {
        return m_movecount;
    }
}
