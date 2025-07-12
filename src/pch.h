#ifndef COMMONHEADERS_H
#define COMMONHEADERS_H

#include "Types.h"               // Common data types and enums
#include "../util/Logger.h"
#include "../util/SafeQueue.h"

#include <atomic>              // For std::atomic (e.g., systemRunningFlag, systemBrokenFlag)
#include <chrono>              // For std::chrono (e.g., sleep_for, seconds)
#include <mutex>               // For std::mutex (for protecting shared data)
#include <condition_variable>  // For std::condition_variable (for thread synchronization)
#include <thread>

#endif //  COMMONHEADERS_H