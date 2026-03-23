/**
 * @file client_main.cpp
 * @brief MPCC client entry point — argument parsing and Client lifecycle.
 *
 * Usage:
 *   ./mpcc_client [--host <ip>] [--port <port>] [--loglevel <LEVEL>]
 *
 * Defaults:
 *   host      = 127.0.0.1
 *   port      = 8080
 *   loglevel  = INFO
 */

#include "mpcc/client/client.h"
#include "mpcc/util/logger.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
static void print_usage(const char* prog)
{
    std::cerr << "Usage: " << prog
              << " [--host <ip>]"
              << " [--port <port>]"
              << " [--loglevel <FATAL|WARNING|INFO|DEBUG>]\n";
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
    std::string host      = "127.0.0.1";
    int         port      = 8080;
    LogLevel    log_level = LogLevel::INFO;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if ((arg == "--host" || arg == "-H") && i + 1 < argc) {
            host = argv[++i];
        } else if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if ((arg == "--loglevel" || arg == "-l") && i + 1 < argc) {
            try {
                log_level = parse_level(argv[++i]);
            } catch (const std::invalid_argument& e) {
                std::cerr << "[ERROR] " << e.what() << "\n";
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
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
        Client client(host, port, "./logs/mpcc_client.log", log_level);
        client.run();
    } catch (const std::exception& ex) {
        std::cerr << "[FATAL] " << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
