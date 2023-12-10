#ifndef MEASURETIME_H
#define MEASURETIME_H

#include <iostream>
#include <chrono>
#include <atomic>

template<unsigned l>
class MeasureTimeHelper {
    static std::atomic_uint m_callCnt;
    static std::atomic_ulong m_durTot;
    std::chrono::time_point<std::chrono::system_clock> m_pre;
    const char *m_fnname;
public:
    MeasureTimeHelper(const char *fnname)
        : m_pre(std::chrono::system_clock::now()), m_fnname(fnname)
    {
        ++m_callCnt;
    }

    ~MeasureTimeHelper() {
        std::chrono::time_point<std::chrono::system_clock> post(std::chrono::system_clock::now());
        auto durns = post - m_pre;
        m_durTot += durns.count();
        std::chrono::milliseconds dur = std::chrono::duration_cast<std::chrono::milliseconds>(durns);
        std::cout << m_fnname << " " << m_callCnt << " " << dur.count() << " ms, total " << m_durTot/1000000 << " ms" << std::endl;
    }
};

template<unsigned l>
std::atomic_uint MeasureTimeHelper<l>::m_callCnt;
template<unsigned l>
std::atomic_ulong MeasureTimeHelper<l>::m_durTot;

#define MeasureTime MeasureTimeHelper<__LINE__> measureTime(__func__)

#endif // MEASURETIME_H
