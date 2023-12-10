#ifndef THREADPOOLHELPER_H
#define THREADPOOLHELPER_H

#include <thread>

extern const unsigned THREAD_COUNT;

template<typename Fn, typename... Ts>
void runInThreadPool(Fn fn, Ts... args)
{
    std::thread threads[THREAD_COUNT-1];
    for(unsigned t = 1; t < THREAD_COUNT; ++t)
        threads[t-1] = std::thread(fn, t, args...);
    fn(0, args...);
    for(unsigned t = 1; t < THREAD_COUNT; ++t)
        threads[t-1].join();
}

#endif // THREADPOOLHELPER_H
