#pragma once
/**
 * @file user_registry.h
 * @brief Flat-file user store with mutex-serialised I/O (MPCC_02, MPCC_03, MPCC_07).
 *
 * File format (registered_users.dat):
 *   username:hex_encoded_xor_password\n
 *   alice:5A3F2B1C4D8E9A0B\n
 *   bob:7D2A0F4E3C1B6A9F\n
 *
 * Safety:
 *   All read and write operations are protected by pthread_mutex_t to prevent
 *   concurrent file corruption from multiple ClientHandler threads.
 *
 *   Writes use an atomic swap pattern:
 *     1. Write full new content to <filepath>.tmp
 *     2. rename(<filepath>.tmp, <filepath>)   ← atomic on POSIX
 */

#include "mpcc/core/interfaces.h"
#include <string>
#include <pthread.h>
#include <unordered_set>

// ─────────────────────────────────────────────────────────────────────────────
class UserRegistry final : public IUserStore {
public:
    /**
     * @param filepath  Path to registered_users.dat (created on first write).
     * @param logger    Optional logger for error reporting (SOLID Fix #8).
     */
    explicit UserRegistry(const std::string& filepath = "./data/registered_users.dat",
                         ILog* logger = nullptr);
    ~UserRegistry() override;

    // IUserStore interface
    bool register_user(const std::string& username,
                       const std::string& enc_password) override;
    bool authenticate (const std::string& username,
                       const std::string& enc_password) override;
    bool user_exists  (const std::string& username)     override;

    // Non-copyable
    UserRegistry(const UserRegistry&)            = delete;
    UserRegistry& operator=(const UserRegistry&) = delete;

private:
    std::string     filepath_;
    ILog*           logger_;  // SOLID Fix #8: Optional logger for failures
    pthread_mutex_t mutex_;

    // In-memory ban list; protected by mutex_
    std::unordered_set<std::string> banned_;

    // Helpers — must be called with mutex already held
    bool find_user_locked(const std::string& username,
                          std::string* out_enc_pass = nullptr) const;
};
