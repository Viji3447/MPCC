#pragma once
/**
 * @file unique_fd.h
 * @brief Minimal RAII wrapper for POSIX file descriptors (close/shutdown safety).
 *
 * Keeps fd lifetime explicit and prevents leaks/double-close across early returns.
 */

#include <unistd.h>
#include <sys/socket.h>

class UniqueFd final {
public:
    UniqueFd() noexcept = default;
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    ~UniqueFd() { reset(); }

    UniqueFd(const UniqueFd&)            = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this == &other) return *this;
        reset();
        fd_ = other.fd_;
        other.fd_ = -1;
        return *this;
    }

    int  get() const noexcept { return fd_; }
    bool valid() const noexcept { return fd_ >= 0; }

    int release() noexcept {
        int tmp = fd_;
        fd_ = -1;
        return tmp;
    }

    void reset(int new_fd = -1) noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = new_fd;
    }

    void shutdown_rdwr() noexcept {
        if (fd_ >= 0) {
            ::shutdown(fd_, SHUT_RDWR);
        }
    }

private:
    int fd_{-1};
};

