#pragma once
/**
 * @file session.h
 * @brief Encapsulates a single authenticated client connection (MPCC_04).
 *
 * Session is the only object that owns a client file descriptor.
 * All socket I/O on the client side goes through Session::send_msg()
 * and Session::close_session() so that cleanup is centralised.
 *
 * IP → username mapping is stored here to satisfy MPCC_09.
 */

#include "mpcc/core/interfaces.h"
#include "mpcc/core/unique_fd.h"
#include <string>
#include <atomic>
#include <pthread.h>

// ─────────────────────────────────────────────────────────────────────────────
class Session final : public ISession {
public:
    /**
     * @param client_fd  Connected socket file descriptor.
     * @param ip_addr    Dotted-decimal IPv4 address of the peer.
     */
    Session(int client_fd, const std::string& ip_addr);
    ~Session() override;

    // ISession interface
    bool        send_msg(const std::string& msg) override;
    void        close_session()                   override;
    int         get_fd()       const override;
    std::string get_username() const override;
    std::string get_ip()       const override;
    bool        is_active()    const override;
    void        set_username(const std::string& username) override;

    // Non-copyable
    Session(const Session&)            = delete;
    Session& operator=(const Session&) = delete;

private:
    UniqueFd         client_fd_;
    std::string      ip_addr_;

    // Thread-safe username (SOLID Fix #3: immutable after first set)
    mutable pthread_mutex_t username_mutex_ = PTHREAD_MUTEX_INITIALIZER;
    std::string      username_;
    bool             username_set_ = false;

    std::atomic<bool> active_;
};
