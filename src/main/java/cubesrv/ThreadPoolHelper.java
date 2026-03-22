package cubesrv;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

public class ThreadPoolHelper {
    public static final int THREAD_COUNT = Runtime.getRuntime().availableProcessors();

    @FunctionalInterface
    public interface IntTask {
        void run(int threadNo) throws Exception;
    }

    public static void runInThreadPool(IntTask fn) {
        ExecutorService pool = Executors.newFixedThreadPool(THREAD_COUNT);
        List<Future<?>> futures = new ArrayList<>();
        for (int i = 0; i < THREAD_COUNT; i++) {
            final int t = i;
            futures.add(pool.submit(() -> { fn.run(t); return null; }));
        }
        for (Future<?> f : futures) {
            try {
                f.get();
            } catch (ExecutionException e) {
                throw new RuntimeException(e.getCause());
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                throw new RuntimeException(e);
            }
        }
        pool.shutdown();
    }
}

