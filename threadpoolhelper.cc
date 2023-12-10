#include "threadpoolhelper.h"
#include <algorithm>

const unsigned THREAD_COUNT = std::max(4u, std::thread::hardware_concurrency());
