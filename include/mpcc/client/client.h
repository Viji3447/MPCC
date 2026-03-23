#pragma once
/**
 * @file client.h
 * @brief MPCC client application: TCP connection, auth UI, send/receive loop.
 *
 * The client spawns one background pthread to receive incoming broadcast
 * messages so that the foreground thread can block on user input without
 * missing server messages.
 *
 * Thread model:
 *   Main thread    → user input loop (getline → send)
 *   Recv thread    → blocking recv() → print to stdout
 */

#include "mpcc/core/interfaces.h"
#include "mpcc/core/cipher.h"
#include "mpcc/util/logger.h"
#include "mpcc/util/thread_handler.h"
#include "mpcc/core/unique_fd.h"

#include <string>
#include <atomic>

// ─────────────────────────────────────────────────────────────────────────────
class Client final {
public:
    /**
     * @param host      Server IP address or hostname.
     * @param port      Server TCP port.
     * @param log_path  Path for client-side log file.
     * @param log_level Minimum log level.
     */
    Client(const std::string& host,
           int port,
           const std::string& log_path  = "./logs/mpcc_client.log",
           LogLevel log_level = LogLevel::INFO);
    ~Client();

    /// Connect → auth → chat loop (blocks until user types "exit")
    void run();

    // Non-copyable
    Client(const Client&)            = delete;
    Client& operator=(const Client&) = delete;

private:
    std::string host_;
    int         port_;
    UniqueFd    sock_fd_;
    Cipher      cipher_;
    Logger      logger_;
    std::atomic<bool> running_;

    JoinableThread recv_thread_;

    // ── Connection ────────────────────────────────────────────────────────────
    void connect_to_server();

    // ── Auth UI ───────────────────────────────────────────────────────────────
    void auth_loop();
    bool do_register();
    bool do_login();

    // ── Chat ──────────────────────────────────────────────────────────────────
    void send_loop();

    // ── Receive thread ────────────────────────────────────────────────────────
    static void* recv_thread_func(void* arg);
    void         recv_loop();

    // ── Socket helpers ────────────────────────────────────────────────────────
    bool send_encrypted(const std::string& plain);
    std::string recv_line();
};
