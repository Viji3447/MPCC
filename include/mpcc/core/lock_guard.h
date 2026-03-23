#pragma once
/**
 * @file lock_guard.h
 * @brief RAII wrapper for pthread_mutex_t (SOLID: Single Responsibility)
 *
 * Ensures pthread_mutex_unlock() is called on every exit path (normal,
 * exception, early return) without requiring explicit unlock calls.
 *
 * Usage:
 *   pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
 *   {
 *       LockGuard lock(mtx);   // locks on construction
 *       // ... critical section ...
 *   }                          // unlocks on destruction
 */

#include <pthread.h>

class LockGuard final {
public:
    explicit LockGuard(pthread_mutex_t& mutex) noexcept
        : mutex_(mutex)
    {
        pthread_mutex_lock(&mutex_);
    }

    ~LockGuard() noexcept
    {
        pthread_mutex_unlock(&mutex_);
    }

    // Non-copyable, non-movable
    LockGuard(const LockGuard&)            = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    pthread_mutex_t& mutex_;
};
