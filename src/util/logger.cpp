/**
 * @file logger.cpp
 * @brief Thread-safe 4-level logger implementation (MPCC_12).
 */

#include "mpcc/util/logger.h"
#include "mpcc/core/lock_guard.h"
#include "mpcc/core/file_io.h"

#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <pthread.h>
#include <cerrno>
#include <cstring>

// ─────────────────────────────────────────────────────────────────────────────
Logger::Logger(const std::string& log_file_path, LogLevel min_level)
    : min_level_(min_level)
    , mutex_(PTHREAD_MUTEX_INITIALIZER)
{
    FileIO::ensure_parent_dir(log_file_path);

    log_file_.open(log_file_path, std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "[FATAL] Logger: cannot open log file: "
                  << log_file_path << " – " << std::strerror(errno) << "\n";
    }
}

Logger::~Logger()
{
    if (log_file_.is_open()) {
        log_file_.flush();
        log_file_.close();
    }
    pthread_mutex_destroy(&mutex_);
}

// ─────────────────────────────────────────────────────────────────────────────
// ILog interface
// ─────────────────────────────────────────────────────────────────────────────
void Logger::fatal(const std::string& msg)   { log(LogLevel::FATAL,   msg); }
void Logger::warning(const std::string& msg) { log(LogLevel::WARNING, msg); }
void Logger::info(const std::string& msg)    { log(LogLevel::INFO,    msg); }
void Logger::debug(const std::string& msg)   { log(LogLevel::DEBUG,   msg); }

// ─────────────────────────────────────────────────────────────────────────────
void Logger::log(LogLevel level, const std::string& msg)
{
    // Filter below minimum level
    if (static_cast<int>(level) > static_cast<int>(min_level_)) {
        return;
    }

    std::string timestamp = current_timestamp();
    pthread_t   tid       = pthread_self();

    std::ostringstream line;
    line << "[" << level_to_str(level) << "]"
         << "[" << timestamp << "]"
         << "[" << tid       << "] "
         << msg << "\n";

    std::string entry = line.str();

    LockGuard lock(mutex_);

    // Write to file
    if (log_file_.is_open()) {
        log_file_ << entry;
        log_file_.flush();
    }

    // Mirror FATAL and WARNING to stderr, rest to stdout
    if (level == LogLevel::FATAL || level == LogLevel::WARNING) {
        std::cerr << entry;
    } else {
        std::cout << entry;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────
const char* Logger::level_to_str(LogLevel level) noexcept
{
    switch (level) {
        case LogLevel::FATAL:   return "FATAL  ";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::INFO:    return "INFO   ";
        case LogLevel::DEBUG:   return "DEBUG  ";
        default:                return "UNKNOWN";
    }
}

std::string Logger::current_timestamp()
{
    using namespace std::chrono;
    auto now      = system_clock::now();
    auto now_time = system_clock::to_time_t(now);

    struct tm buf{};
    ::localtime_r(&now_time, &buf);   // thread-safe reentrant variant

    char ts[32];
    ::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &buf);
    return std::string(ts);
}
