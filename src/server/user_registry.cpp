/**
 * @file user_registry.cpp
 * @brief Flat-file user store with atomic-swap writes (MPCC_02, MPCC_03, MPCC_07).
 *
 * File format:
 *   username:hex_encoded_xor_password\n
 *
 * Passwords are stored hex-encoded so that XOR bytes (which may include NUL
 * and non-printable chars) survive text-file round-trips safely.
 *
 * Atomic write pattern:
 *   Write full content to <filepath>.tmp, then rename() — POSIX guarantees
 *   rename() is atomic with respect to other processes seeing the file.
 */

#include "mpcc/server/user_registry.h"
#include "mpcc/core/lock_guard.h"
#include "mpcc/core/file_io.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <cerrno>
#include <cstring>
#include <iomanip>    // std::hex, std::setw

// ─────────────────────────────────────────────────────────────────────────────
// Helpers: hex encode / decode
// ─────────────────────────────────────────────────────────────────────────────
static std::string to_hex(const std::string& raw)
{
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    for (unsigned char c : raw) {
        oss << std::setw(2) << static_cast<unsigned>(c);
    }
    return oss.str();
}

static std::string from_hex(const std::string& hex)
{
    std::string out;
    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
        unsigned byte = 0;
        std::istringstream(hex.substr(i, 2)) >> std::hex >> byte;
        out.push_back(static_cast<char>(byte));
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Load all records from file (caller must hold mutex)
// ─────────────────────────────────────────────────────────────────────────────
static std::vector<std::pair<std::string,std::string>>
load_records(const std::string& filepath)
{
    std::vector<std::pair<std::string,std::string>> records;
    for (const std::string& line : FileIO::read_all_lines(filepath)) {
        if (line.empty()) continue;
        std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        records.emplace_back(line.substr(0, colon),
                             line.substr(colon + 1));
    }
    return records;
}

// ─────────────────────────────────────────────────────────────────────────────
// Persist all records (caller must hold mutex) — atomic write
// ─────────────────────────────────────────────────────────────────────────────
static bool save_records(const std::string& filepath,
                         const std::vector<std::pair<std::string,std::string>>& records)
{
    std::vector<std::string> lines;
    lines.reserve(records.size());
    for (const auto& r : records) {
        lines.push_back(r.first + ":" + r.second);
    }
    return FileIO::atomic_write_lines(filepath, lines);
}

// ─────────────────────────────────────────────────────────────────────────────
UserRegistry::UserRegistry(const std::string& filepath, ILog* logger)
    : filepath_(filepath)
    , logger_(logger)
    , mutex_(PTHREAD_MUTEX_INITIALIZER)
{
    FileIO::ensure_parent_dir(filepath_);
}

UserRegistry::~UserRegistry()
{
    pthread_mutex_destroy(&mutex_);
}

// ─────────────────────────────────────────────────────────────────────────────
// IUserStore interface
// ─────────────────────────────────────────────────────────────────────────────
bool UserRegistry::register_user(const std::string& username,
                                  const std::string& enc_password)
{
    LockGuard lock(mutex_);

    if (find_user_locked(username)) return false;   // already exists

    auto records = load_records(filepath_);
    records.emplace_back(username, to_hex(enc_password));
    bool success = save_records(filepath_, records);

    // SOLID Fix #8: Log file I/O failures
    if (!success && logger_) {
        logger_->warning("UserRegistry::register_user - Failed to save user " + username +
                        " to " + filepath_ + " (errno=" + std::to_string(errno) + ")");
    }

    return success;
}

bool UserRegistry::authenticate(const std::string& username,
                                 const std::string& enc_password)
{
    LockGuard lock(mutex_);

    std::string stored_hex;
    if (!find_user_locked(username, &stored_hex)) return false;

    // Decode the stored hex back to raw XOR bytes and compare
    std::string stored_raw = from_hex(stored_hex);
    if (stored_raw != enc_password) return false;

    // Check in-memory ban list
    return banned_.find(username) == banned_.end();
}

bool UserRegistry::user_exists(const std::string& username)
{
    LockGuard lock(mutex_);
    return find_user_locked(username);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers (caller holds mutex)
// ─────────────────────────────────────────────────────────────────────────────
bool UserRegistry::find_user_locked(const std::string& username,
                                     std::string* out_enc_pass) const
{
    auto records = load_records(filepath_);
    for (const auto& r : records) {
        if (r.first == username) {
            if (out_enc_pass) *out_enc_pass = r.second;
            return true;
        }
    }
    return false;
}
