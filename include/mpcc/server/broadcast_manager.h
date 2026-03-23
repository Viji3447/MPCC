
#pragma once
/**
 * @file broadcast_manager.h
 * @brief Thread-safe registry of active Sessions; delivers broadcast messages (MPCC_06).
 *
 * Design notes (SOLID):
 *  - Single Responsibility : manages the active-client list and broadcast delivery only.
 *  - Open/Closed           : IBroadcast interface allows alternative implementations.
 *  - Liskov Substitution   : BroadcastManager IS-A IBroadcast.
 *  - Interface Segregation : IBroadcast is narrow; callers depend only on what they use.
 *  - Dependency Inversion  : ClientHandler depends on IBroadcast, not the concrete class.
 *
 * Locking strategy (HLD §2.2.10):
 *   The mutex is held while iterating the client list.
 *   Session::send_msg() is called WITHOUT the mutex held — the lock is released
 *   before each send to avoid holding the lock during a potentially blocking syscall.
 *   A copy of the target list is made under the lock, then sends happen outside.
 */

#include "mpcc/core/interfaces.h"
#include "mpcc/core/lock_guard.h"
#include <vector>
#include <unordered_map>
#include <pthread.h>

// ─────────────────────────────────────────────────────────────────────────────
class BroadcastManager final : public IBroadcast {
public:
    explicit BroadcastManager(const IEncrypt& cipher);
    ~BroadcastManager() override;

    // IBroadcast interface
    void add_client(ISession* session)    override;
    void remove_client(ISession* session) override;

    /**
     * @brief Encrypt and send @p message to every active session except @p sender.
     * @param sender   Username of the originating client (excluded from delivery).
     * @param message  Plaintext chat message.
     */
    void broadcast(const std::string& sender, const std::string& message) override;

    void send_private(const std::string& from,
                      const std::string& to,
                      const std::string& message) override;

    // SOLID Fix #2: Send message to a specific set of recipients (used by RoomManager)
    void broadcast_to_recipients(const std::string& sender,
                                 const std::string& message,
                                 const std::vector<ISession*>& recipients) override;

    std::size_t client_count() const;
    void shutdown_all();

    // Non-copyable
    BroadcastManager(const BroadcastManager&)            = delete;
    BroadcastManager& operator=(const BroadcastManager&) = delete;

private:
    std::vector<ISession*>              clients_;
    std::unordered_map<std::string, ISession*> by_name_;
    const IEncrypt&                     cipher_;
    mutable pthread_mutex_t             mutex_;
};
