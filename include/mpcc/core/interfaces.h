#pragma once
/**
 * @file interfaces.h
 * @brief Pure abstract interfaces for MPCC subsystems (SOLID: Dependency Inversion Principle)
 *
 * All inter-module dependencies are expressed as references/pointers to these
 * interfaces, enabling unit-testing and future implementation swaps.
 */

#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// ILog — 4-level logging interface (MPCC_12)
// ─────────────────────────────────────────────────────────────────────────────
class ILog {
public:
    virtual ~ILog() = default;
    virtual void fatal(const std::string& msg)   = 0;
    virtual void warning(const std::string& msg) = 0;
    virtual void info(const std::string& msg)    = 0;
    virtual void debug(const std::string& msg)   = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IEncrypt — symmetric encrypt/decrypt interface (MPCC_10 / MPCC_11)
// ─────────────────────────────────────────────────────────────────────────────
class IEncrypt {
public:
    virtual ~IEncrypt() = default;
    virtual std::string encrypt(const std::string& data) const = 0;
    virtual std::string decrypt(const std::string& data) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IUserStore — user registration and authentication interface (MPCC_02, MPCC_03, MPCC_07)
// ─────────────────────────────────────────────────────────────────────────────
class IUserStore {
public:
    virtual ~IUserStore() = default;
    virtual bool register_user(const std::string& username,
                               const std::string& enc_password) = 0;
    virtual bool authenticate(const std::string& username,
                              const std::string& enc_password) = 0;
    virtual bool user_exists(const std::string& username) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// ISession — per-client session interface (MPCC_04, MPCC_08)
// ─────────────────────────────────────────────────────────────────────────────
class ISession {
public:
    virtual ~ISession() = default;
    virtual bool        send_msg(const std::string& msg) = 0;
    virtual void        close_session()                   = 0;
    virtual int         get_fd()       const = 0;
    virtual std::string get_username() const = 0;
    virtual std::string get_ip()       const = 0;
    virtual bool        is_active()    const = 0;
    virtual void        set_username(const std::string& username) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IBroadcast — broadcast messages to all connected sessions (MPCC_06)
// ─────────────────────────────────────────────────────────────────────────────
class IBroadcast {
public:
    virtual ~IBroadcast() = default;
    virtual void add_client(ISession* session)    = 0;
    virtual void remove_client(ISession* session) = 0;
    virtual void broadcast(const std::string& sender,
                           const std::string& message) = 0;

    // Optional: send a private message from one user to another.
    virtual void send_private(const std::string& from,
                              const std::string& to,
                              const std::string& message) = 0;

    // SOLID Fix #2: Room-scoped broadcast (sends to clients in given recipients list)
    virtual void broadcast_to_recipients(const std::string& sender,
                                         const std::string& message,
                                         const std::vector<ISession*>& recipients) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IRooms — room/channel membership and scoped broadcast
// ─────────────────────────────────────────────────────────────────────────────
class IRooms {
public:
    virtual ~IRooms() = default;

    virtual void join_room(const std::string& room, ISession* session)  = 0;
    virtual void leave_room(const std::string& room, ISession* session) = 0;
    virtual void leave_all(ISession* session)                            = 0;

    virtual void broadcast_to_room(const std::string& room,
                                   const std::string& sender,
                                   const std::string& message) = 0;
};
