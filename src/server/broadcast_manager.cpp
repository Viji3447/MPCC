/**
 * @file broadcast_manager.cpp
 * @brief Thread-safe message broadcast to all connected sessions (MPCC_06).
 *
 * Locking strategy (HLD §2.2.10):
 *   The client list is copied under the mutex; sends happen outside the lock.
 *   This prevents a slow/blocked send() from stalling all other threads.
 */

#include "mpcc/server/broadcast_manager.h"
#include "mpcc/core/message.h"

#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
BroadcastManager::BroadcastManager(const IEncrypt& cipher)
    : clients_()
    , cipher_(cipher)
    , mutex_(PTHREAD_MUTEX_INITIALIZER)
{
    clients_.reserve(64);
}

BroadcastManager::~BroadcastManager()
{
    pthread_mutex_destroy(&mutex_);
}

// ─────────────────────────────────────────────────────────────────────────────
// IBroadcast interface
// ─────────────────────────────────────────────────────────────────────────────
void BroadcastManager::add_client(ISession* session)
{
    LockGuard lock(mutex_);
    clients_.push_back(session);
    if (session) {
        by_name_[session->get_username()] = session;
    }
}

void BroadcastManager::remove_client(ISession* session)
{
    LockGuard lock(mutex_);
    clients_.erase(
        std::remove(clients_.begin(), clients_.end(), session),
        clients_.end()
    );

    if (session) {
        auto it = by_name_.find(session->get_username());
        if (it != by_name_.end() && it->second == session) {
            by_name_.erase(it);
        }
    }
}

void BroadcastManager::broadcast(const std::string& sender,
                                  const std::string& message)
{
    // Serialise the broadcast message frame once (plaintext)
    std::string wire_plain = Message::make_bcast(sender, message).serialise();

    // Copy the active-client snapshot under the lock, then release
    std::vector<ISession*> snapshot;
    {
        LockGuard lock(mutex_);
        snapshot.reserve(clients_.size());
        for (ISession* s : clients_) {
            if (s->is_active() && s->get_username() != sender) {
                snapshot.push_back(s);
            }
        }
    }

    // Send without holding the mutex (avoids blocking all threads).
    // Each frame is XOR-encrypted before being written to the socket.
    for (ISession* s : snapshot) {
        std::string wire_enc = cipher_.encrypt(wire_plain);
        s->send_msg(wire_enc);
    }
}

void BroadcastManager::send_private(const std::string& from,
                                    const std::string& to,
                                    const std::string& message)
{
    ISession* target = nullptr;
    {
        LockGuard lock(mutex_);
        auto it = by_name_.find(to);
        if (it != by_name_.end() && it->second && it->second->is_active()) {
            target = it->second;
        }
    }
    if (!target) return;

    // Format: "[PM from from]: message"
    std::string pm_text   = "[PM from " + from + "]: " + message;
    std::string wire_plain = Message::make_bcast(from, pm_text).serialise();
    std::string wire_enc   = cipher_.encrypt(wire_plain);
    target->send_msg(wire_enc);
}

// ─────────────────────────────────────────────────────────────────────────────
// SOLID Fix #2: Broadcast to specific recipients (RoomManager delegation)
// ─────────────────────────────────────────────────────────────────────────────
void BroadcastManager::broadcast_to_recipients(const std::string& sender,
                                               const std::string& message,
                                               const std::vector<ISession*>& recipients)
{
    // Serialise the broadcast message frame once (plaintext)
    std::string wire_plain = Message::make_bcast(sender, message).serialise();
    std::string wire_enc   = cipher_.encrypt(wire_plain);

    // Send to each recipient (already filtered by caller)
    for (ISession* s : recipients) {
        if (!s || !s->is_active() || s->get_username() == sender) continue;
        s->send_msg(wire_enc);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
std::size_t BroadcastManager::client_count() const
{
    LockGuard lock(mutex_);
    return clients_.size();
}
void BroadcastManager::shutdown_all()
{
    pthread_mutex_lock(&mutex_);

    for (ISession* s : clients_) {
        if (s && s->is_active()) {
            s->close_session();  // force close the client socket
        }
    }

    clients_.clear();
    by_name_.clear();

    pthread_mutex_unlock(&mutex_);
}
