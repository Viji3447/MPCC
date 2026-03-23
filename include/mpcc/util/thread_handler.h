#pragma once
/**
 * @file thread_handler.h
 * @brief Small helpers to standardize pthread lifecycle (create/join/group).
 *
 * Goal: avoid duplicating pthread_attr_init/destroy, join bookkeeping, and
 * "join-all on shutdown" patterns across classes.
 */
 
 #include "mpcc/core/lock_guard.h"
 
 #include <pthread.h>
 #include <vector>
 #include <utility>
 
 // ─────────────────────────────────────────────────────────────────────────────
 // RAII wrapper for pthread_attr_t
 // ─────────────────────────────────────────────────────────────────────────────
 class PThreadAttr final {
 public:
     PThreadAttr()
         : ok_(::pthread_attr_init(&attr_) == 0)
     {}
 
     ~PThreadAttr() noexcept
     {
         if (ok_) ::pthread_attr_destroy(&attr_);
     }
 
     PThreadAttr(const PThreadAttr&)            = delete;
     PThreadAttr& operator=(const PThreadAttr&) = delete;
 
     bool valid() const noexcept { return ok_; }
     pthread_attr_t* get() noexcept { return ok_ ? &attr_ : nullptr; }
 
     void set_joinable() noexcept
     {
         if (ok_) ::pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_JOINABLE);
     }
 
 private:
     pthread_attr_t attr_{};
     bool ok_;
 };
 
 // ─────────────────────────────────────────────────────────────────────────────
 // One joinable pthread with safe join/reset semantics
 // ─────────────────────────────────────────────────────────────────────────────
 class JoinableThread final {
 public:
     JoinableThread() = default;
     ~JoinableThread() { join(); }
 
     JoinableThread(const JoinableThread&)            = delete;
     JoinableThread& operator=(const JoinableThread&) = delete;
 
     JoinableThread(JoinableThread&& other) noexcept
         : tid_(other.tid_)
         , started_(other.started_)
     {
         other.tid_ = {};
         other.started_ = false;
     }
 
     JoinableThread& operator=(JoinableThread&& other) noexcept
     {
         if (this == &other) return *this;
         join();
         tid_ = other.tid_;
         started_ = other.started_;
         other.tid_ = {};
         other.started_ = false;
         return *this;
     }
 
     int start(void* (*fn)(void*), void* arg) noexcept
     {
         if (started_) return 0; // already started; treat as success
 
         PThreadAttr attr;
         if (!attr.valid()) return 1;
         attr.set_joinable();
 
         int rc = ::pthread_create(&tid_, attr.get(), fn, arg);
         if (rc == 0) started_ = true;
         return rc;
     }
 
     void join() noexcept
     {
         if (!started_) return;
         ::pthread_join(tid_, nullptr);
         tid_ = {};
         started_ = false;
     }
 
     pthread_t native_handle() const noexcept { return tid_; }
     bool started() const noexcept { return started_; }
 
 private:
     pthread_t tid_{};
     bool started_{false};
 };
 
 // ─────────────────────────────────────────────────────────────────────────────
 // Thread group: track pthread_t and join them later (e.g., on shutdown).
 // ─────────────────────────────────────────────────────────────────────────────
 class ThreadGroup final {
 public:
     ThreadGroup()
         : mutex_(PTHREAD_MUTEX_INITIALIZER)
     {
         tids_.reserve(32);
     }
 
     ~ThreadGroup()
     {
         // Do not implicitly join in destructor; caller should control shutdown order.
         ::pthread_mutex_destroy(&mutex_);
     }
 
     ThreadGroup(const ThreadGroup&)            = delete;
     ThreadGroup& operator=(const ThreadGroup&) = delete;
 
     void add(pthread_t tid)
     {
         LockGuard lock(mutex_);
         tids_.push_back(tid);
     }
 
     std::vector<pthread_t> snapshot() const
     {
         LockGuard lock(mutex_);
         return tids_;
     }
 
     void join_all()
     {
         std::vector<pthread_t> copy;
         {
             LockGuard lock(mutex_);
             copy.swap(tids_);
         }
 
         for (pthread_t tid : copy) {
             ::pthread_join(tid, nullptr);
         }
     }
 
 private:
     mutable pthread_mutex_t mutex_;
     std::vector<pthread_t> tids_;
 };
