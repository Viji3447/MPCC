#pragma once
/**
 * @file logger.h
 * @brief Thread-safe 4-level logger (MPCC_12)
 *
 * Log levels (numeric, ascending verbosity):
 *   FATAL=0  WARNING=1  INFO=2  DEBUG=3
 *
 * Output is written to both console (stderr) and a log file.
 * All writes are serialized by an internal pthread_mutex_t.
 */

#include "mpcc/core/interfaces.h"
#include <string>
#include <fstream>
#include <pthread.h>

// ─────────────────────────────────────────────────────────────────────────────
enum class LogLevel : int {
    FATAL   = 0,
    WARNING = 1,
    INFO    = 2,
    DEBUG   = 3
};

// ─────────────────────────────────────────────────────────────────────────────
class Logger final : public ILog {
public:
    /**
     * @brief Initialise the logger.
     * @param log_file_path  Path to the log file (append mode).
     * @param min_level      Minimum severity to emit (default DEBUG = all).
     */
    explicit Logger(const std::string& log_file_path = "./logs/mpcc_server.log",
                    LogLevel min_level = LogLevel::DEBUG);
    ~Logger() override;

    // ILog interface
    void fatal(const std::string& msg)   override;
    void warning(const std::string& msg) override;
    void info(const std::string& msg)    override;
    void debug(const std::string& msg)   override;

    // Core log method used by all level helpers
    void log(LogLevel level, const std::string& msg);

    // Non-copyable / non-movable (resource owner)
    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

private:
    LogLevel        min_level_;
    std::ofstream   log_file_;
    pthread_mutex_t mutex_;

    static const char* level_to_str(LogLevel level) noexcept;
    static std::string current_timestamp();
};
