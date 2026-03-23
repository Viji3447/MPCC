#pragma once
/**
 * @file server.h
 * @brief TCP server: socket lifecycle, accept loop, thread management (MPCC_01, MPCC_05).
 *
 * Server is the application root object.  It owns (via composition):
 *   - The listening file descriptor
 *   - BroadcastManager
 *   - UserRegistry
 *   - Logger
 *   - The list of active ClientHandler thread IDs (for join on shutdown)
 *
 * Shutdown sequence:
 *   1. SIGINT handler sets running_ = false
 *   2. accept() returns EINTR; loop exits
 *   3. shutdown() closes server_fd_ (unblocks any lingering accept())
 *   4. For each active thread: send shutdown signal via session close,
 *      then pthread_join()
 */

#include "mpcc/core/interfaces.h"
#include "mpcc/server/broadcast_manager.h"
#include "mpcc/server/user_registry.h"
#include "mpcc/server/room_manager.h"
#include "mpcc/util/logger.h"
#include "mpcc/core/cipher.h"
#include "mpcc/core/lock_guard.h"
#include "mpcc/util/thread_handler.h"
#include "mpcc/core/unique_fd.h"

#include <atomic>
#include <pthread.h>

// ─────────────────────────────────────────────────────────────────────────────
class Server final {
public:
    /**
     * @param port      TCP port to listen on (default 8080).
     * @param log_path  Path for the log file.
     * @param log_level Minimum log level to emit.
     * @param backlog   listen() backlog (default 10).
     */
    explicit Server(int port       = 8080,
                    const std::string& log_path  = "./logs/mpcc_server.log",
                    LogLevel log_level = LogLevel::DEBUG,
                    int backlog    = 10);
    ~Server();

    /// Initialise socket, bind, listen, then run accept loop until stopped.
    void start();

    /// Signal the accept loop to exit and clean up all resources.
    void shutdown();

    // Non-copyable
    Server(const Server&)            = delete;
    Server& operator=(const Server&) = delete;

private:
    int              port_;
    int              backlog_;
    UniqueFd         server_fd_;
    std::atomic<bool> running_;

    Logger           logger_;
    Cipher           cipher_;
    BroadcastManager broadcast_mgr_;
    UserRegistry     user_registry_;
    RoomManager      room_manager_;

    // Tracks pthread IDs so we can join them on shutdown
    ThreadGroup       thread_group_;

    // ── Setup helpers ─────────────────────────────────────────────────────────
    void create_socket();
    void bind_socket();
    void listen_socket();

    // ── Accept loop ───────────────────────────────────────────────────────────
    void accept_loop();

    // ── Signal handling ───────────────────────────────────────────────────────
    static void sigint_handler(int sig);
    static volatile Server* instance_;  ///< Pointer used by signal handler (volatile for signal-safe access)
};
