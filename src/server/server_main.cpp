/**
 * @file server_main.cpp
 * @brief MPCC server entry point — argument parsing and Server lifecycle.
 *
 * Usage:
 *   ./mpcc_server [--port <port>] [--loglevel <FATAL|WARNING|INFO|DEBUG>]
 *                 [--logfile <path>]
 *
 * Defaults:
 *   port      = 8080
 *   loglevel  = INFO
 *   logfile   = ./logs/mpcc_server.log
 */

#include "mpcc/server/server.h"
#include "mpcc/util/logger.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
static void print_usage(const char* prog)
{
    std::cerr << "Usage: " << prog
              << " [--port <port>]"
              << " [--loglevel <FATAL|WARNING|INFO|DEBUG>]"
              << " [--logfile <path>]\n";
}

static LogLevel parse_level(const std::string& s)
{
    if (s == "FATAL")   return LogLevel::FATAL;
    if (s == "WARNING") return LogLevel::WARNING;
    if (s == "INFO")    return LogLevel::INFO;
    if (s == "DEBUG")   return LogLevel::DEBUG;
    throw std::invalid_argument("Unknown log level: " + s);
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    int         port      = 8080;
    LogLevel    log_level = LogLevel::INFO;
    std::string log_file  = "./logs/mpcc_server.log";

    // Simple argument parser
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if ((arg == "--loglevel" || arg == "-l") && i + 1 < argc) {
            try {
                log_level = parse_level(argv[++i]);
            } catch (const std::invalid_argument& e) {
                std::cerr << "[ERROR] " << e.what() << "\n";
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else if ((arg == "--logfile" || arg == "-f") && i + 1 < argc) {
            log_file = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            std::cerr << "[ERROR] Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    try {
        Server server(port, log_file, log_level);
        server.start();
        server.shutdown();
    } catch (const std::exception& ex) {
        std::cerr << "[FATAL] Server failed to start: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
