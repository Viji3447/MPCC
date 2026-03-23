

/**
 * @file server.cpp
 * @brief TCP server: socket lifecycle, accept loop, graceful shutdown (MPCC_01, MPCC_05).
 *
 * Socket options set:
 *   SO_REUSEADDR  – avoids "address already in use" on restart (HLD §2.2.9)
 *   SO_RCVTIMEO   – 60 s receive timeout applied to each client socket (HLD §1.8)
 *                   NOTE: intentionally NOT set on the server (listening) socket
 *                   to avoid spurious EAGAIN from accept() during idle periods.
 */

#include "mpcc/server/server.h"
#include "mpcc/server/client_handler.h"
#include "mpcc/core/session.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
// Static member (SOLID Fix #7: Signal-safe access marked volatile)
// ─────────────────────────────────────────────────────────────────────────────
volatile Server* Server::instance_ = nullptr;

// ─────────────────────────────────────────────────────────────────────────────
Server::Server(int port,
               const std::string& log_path,
               LogLevel log_level,
               int backlog)
    : port_(port)
    , backlog_(backlog)
    , server_fd_()
    , running_(false)
    , logger_(log_path, log_level)
    , cipher_()
    , broadcast_mgr_(cipher_)
    , user_registry_("./data/registered_users.dat", &logger_)  // SOLID Fix #8: Pass logger
    , room_manager_(cipher_, broadcast_mgr_)  // SOLID Fix #2: Pass broadcast_mgr_
{
    instance_ = this;

    // Register SIGINT handler for graceful shutdown
    struct sigaction sa{};
    sa.sa_handler = Server::sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    // Ignore SIGPIPE so that a broken-pipe send() returns -1 instead of killing us
    ::signal(SIGPIPE, SIG_IGN);
}

Server::~Server()
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────
void Server::start()
{
    create_socket();
    bind_socket();
    listen_socket();

    logger_.info("MPCC Server listening on port " + std::to_string(port_));
    logger_.info("Press Ctrl-C to shut down.");

    running_.store(true, std::memory_order_release);
    accept_loop();
}

void Server::shutdown()
{
    logger_.info("Server::shutdown() – setting running=false");
    running_.store(false, std::memory_order_release);
    disconnect_all_clients();

    if (server_fd_.valid()) {
        server_fd_.shutdown_rdwr();
        server_fd_.reset();
    }

    // Join all worker threads
    thread_group_.join_all();

    logger_.info("Server shutdown complete.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private: Socket setup
// ─────────────────────────────────────────────────────────────────────────────
void Server::create_socket()
{
    server_fd_.reset(::socket(AF_INET, SOCK_STREAM, 0));
    if (!server_fd_.valid()) {
        logger_.fatal("socket() failed: " + std::string(std::strerror(errno)));
        throw std::runtime_error("socket() failed");
    }

    // SO_REUSEADDR — allow quick restart without "address already in use"
    int opt = 1;
    if (::setsockopt(server_fd_.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger_.warning("setsockopt(SO_REUSEADDR) failed: " +
                        std::string(std::strerror(errno)));
    }

    logger_.debug("Server socket created: fd=" + std::to_string(server_fd_.get()));
}

void Server::bind_socket()
{
    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(static_cast<uint16_t>(port_));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(server_fd_.get(),
               reinterpret_cast<struct sockaddr*>(&addr),
               sizeof(addr)) < 0) {
        logger_.fatal("bind() on port " + std::to_string(port_) +
                      " failed: " + std::strerror(errno));
        server_fd_.reset();
        throw std::runtime_error("bind() failed");
    }

    logger_.debug("Socket bound to port " + std::to_string(port_));
}

void Server::listen_socket()
{
    if (::listen(server_fd_.get(), backlog_) < 0) {
        logger_.fatal("listen() failed: " + std::string(std::strerror(errno)));
        server_fd_.reset();
        throw std::runtime_error("listen() failed");
    }
    logger_.debug("Socket in listen state, backlog=" + std::to_string(backlog_));
}

// ─────────────────────────────────────────────────────────────────────────────
// Private: Accept loop (HLD §4.3.1)
// ─────────────────────────────────────────────────────────────────────────────
void Server::accept_loop()
{
    while (running_.load(std::memory_order_acquire)) {
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = ::accept(server_fd_.get(),
                                 reinterpret_cast<struct sockaddr*>(&client_addr),
                                 &addr_len);

        if (client_fd < 0) {
            if (!running_.load(std::memory_order_acquire)) break;  // shutdown
            if (errno == EINTR) continue;
            logger_.warning("accept() error: " + std::string(std::strerror(errno)));
            continue;
        }

        std::string ip = ::inet_ntoa(client_addr.sin_addr);
        logger_.info("New connection accepted from " + ip +
                     "  (fd=" + std::to_string(client_fd) + ")");

        // Set TCP_NODELAY on client socket for low-latency chat
        int flag = 1;
        ::setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        // SO_RCVTIMEO — 60 s receive timeout on client socket (unblocks stuck recv)
        // Applied here (not on the server socket) to avoid spurious EAGAIN from accept()
        struct timeval tv{};
        tv.tv_sec  = 60;
        tv.tv_usec = 0;
        ::setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // Heap-allocate Session — owned (and deleted) by ClientHandler
        auto session = std::make_unique<Session>(client_fd, ip);

        auto handler = std::make_unique<ClientHandler>(
            session.release(), broadcast_mgr_, user_registry_, room_manager_, logger_, cipher_);

        // SOLID Fix #9: Exception-safe ThreadCtx allocation using unique_ptr
        auto ctx = std::make_unique<ClientHandler::ThreadCtx>(std::move(handler));

        // Spawn joinable worker thread
        PThreadAttr attr;
        if (!attr.valid()) {
            logger_.fatal("pthread_attr_init() failed.");
            // ctx automatically freed (unique_ptr destructor called)
            continue;
        }
        attr.set_joinable();

        pthread_t tid{};
        int rc = ::pthread_create(&tid, attr.get(),
                                  ClientHandler::thread_func, ctx.get());

        if (rc != 0) {
            logger_.fatal("pthread_create() failed: " + std::string(std::strerror(rc)));
            // ctx automatically freed on failure (unique_ptr destructor called)
            continue;
        }

        // Only release on success — thread now owns the ThreadCtx
        ctx.release();

        // Track the thread ID so shutdown() can join it
        thread_group_.add(tid);

        std::ostringstream tid_ss;
        tid_ss << tid;
        logger_.debug("ClientHandler thread spawned: tid=" + tid_ss.str());
    }

    logger_.info("accept_loop exited.");
}

void Server::disconnect_all_clients()
{
    logger_.info("Disconnecting all active client sessions...");

    // Ask room_manager to remove everyone and close sessions
    //room_manager_.leave_all_sessions();

    // Ask broadcast manager to close all client sockets
    broadcast_mgr_.shutdown_all();
}


// ─────────────────────────────────────────────────────────────────────────────
// Signal handler (SOLID Fix #7: Only perform async-signal-safe operations)
// ─────────────────────────────────────────────────────────────────────────────
void Server::sigint_handler(int /*sig*/)
{
    // ONLY set the atomic flag — this is the only async-signal-safe operation
    // The main thread will check running_ and handle actual cleanup
    if (instance_) {
        instance_->running_.store(false, std::memory_order_release);
    }
}

