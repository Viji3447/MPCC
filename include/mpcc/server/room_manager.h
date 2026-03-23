#pragma once
/**
 * @file room_manager.h
 * @brief Room/channel manager for scoped broadcasts.
 */

#include "mpcc/core/interfaces.h"
#include "mpcc/core/lock_guard.h"
#include "mpcc/core/cipher.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <pthread.h>

class RoomManager final : public IRooms {
public:
    // SOLID Fix #2: Accept IBroadcast dependency for delegated sending
    explicit RoomManager(const IEncrypt& cipher, IBroadcast& broadcast);
    ~RoomManager() override;

    void join_room(const std::string& room, ISession* session) override;
    void leave_room(const std::string& room, ISession* session) override;
    void leave_all(ISession* session) override;

    void broadcast_to_room(const std::string& room,
                           const std::string& sender,
                           const std::string& message) override;

    // Non-copyable
    RoomManager(const RoomManager&)            = delete;
    RoomManager& operator=(const RoomManager&) = delete;

private:
    using SessionSet = std::unordered_set<ISession*>;

    IBroadcast&                                 broadcast_;  // SOLID Fix #2: Delegate sending
    std::unordered_map<std::string, SessionSet> rooms_;
    pthread_mutex_t                             mutex_;
};

