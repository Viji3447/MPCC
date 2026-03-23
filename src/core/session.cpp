/**
 * @file session.cpp
 * @brief Encapsulates a single client connection (MPCC_04, MPCC_09).
 *
 * SOLID Fix #3: Username is now thread-safe and immutable after first set.
 * Multiple calls to set_username() after the first one are silently ignored.
 */

#include "mpcc/core/session.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
Session::Session(int client_fd, const std::string& ip_addr)
    : client_fd_(client_fd)
    , ip_addr_(ip_addr)
    , username_mutex_(PTHREAD_MUTEX_INITIALIZER)
    , username_("(unauthenticated)")
    , username_set_(false)
    , active_(true)
{}

Session::~Session()
{
    close_session();
    pthread_mutex_destroy(&username_mutex_);
}

// ─────────────────────────────────────────────────────────────────────────────
// ISession interface
// ─────────────────────────────────────────────────────────────────────────────
bool Session::send_msg(const std::string& msg)
{
    if (!active_.load(std::memory_order_acquire)) {
        return false;
    }

    std::size_t total    = 0;
    std::size_t to_send  = msg.size();
    const char* buf      = msg.data();

    while (total < to_send) {
        ssize_t n = ::send(client_fd_.get(), buf + total, to_send - total, MSG_NOSIGNAL);
        if (n <= 0) {
            active_.store(false, std::memory_order_release);
            return false;
        }
        total += static_cast<std::size_t>(n);
    }
    return true;
}

void Session::close_session()
{
    bool expected = true;
    if (active_.compare_exchange_strong(expected, false,
                                        std::memory_order_acq_rel)) {
        client_fd_.shutdown_rdwr();
        client_fd_.reset();
    }
}

int Session::get_fd() const
{
    return client_fd_.get();
}

std::string Session::get_username() const
{
    // Thread-safe read
    pthread_mutex_lock(&username_mutex_);
    std::string result = username_;
    pthread_mutex_unlock(&username_mutex_);
    return result;
}

std::string Session::get_ip() const
{
    return ip_addr_;
}

bool Session::is_active() const
{
    return active_.load(std::memory_order_acquire);
}

// ─────────────────────────────────────────────────────────────────────────────
// Mutator: set_username - SOLID Fix #3: Thread-safe, immutable after first set
// ─────────────────────────────────────────────────────────────────────────────
void Session::set_username(const std::string& username)
{
    pthread_mutex_lock(&username_mutex_);

    // Only set once; subsequent calls are silently ignored
    if (!username_set_) {
        username_ = username;
        username_set_ = true;
    }

    pthread_mutex_unlock(&username_mutex_);
}

