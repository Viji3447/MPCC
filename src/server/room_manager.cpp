/**
 * @file room_manager.cpp
 * @brief Room/channel manager implementation (SOLID Fix #2: Delegated sending).
 */

#include "mpcc/server/room_manager.h"
#include "mpcc/core/message.h"

RoomManager::RoomManager(const IEncrypt& cipher, IBroadcast& broadcast)
    : broadcast_(broadcast)
    , rooms_()
    , mutex_(PTHREAD_MUTEX_INITIALIZER)
{
    // cipher parameter no longer needed — broadcasting is delegated
    (void)cipher;  // Suppress unused parameter warning
}

RoomManager::~RoomManager()
{
    pthread_mutex_destroy(&mutex_);
}

void RoomManager::join_room(const std::string& room, ISession* session)
{
    if (!session) return;
    LockGuard lock(mutex_);
    rooms_[room].insert(session);
}

void RoomManager::leave_room(const std::string& room, ISession* session)
{
    if (!session) return;
    LockGuard lock(mutex_);
    auto it = rooms_.find(room);
    if (it == rooms_.end()) return;
    it->second.erase(session);
    if (it->second.empty()) {
        rooms_.erase(it);
    }
}

void RoomManager::leave_all(ISession* session)
{
    if (!session) return;
    LockGuard lock(mutex_);
    for (auto it = rooms_.begin(); it != rooms_.end(); ) {
        it->second.erase(session);
        if (it->second.empty()) {
            it = rooms_.erase(it);
        } else {
            ++it;
        }
    }
}

void RoomManager::broadcast_to_room(const std::string& room,
                                    const std::string& sender,
                                    const std::string& message)
{
    std::vector<ISession*> room_members;

    // SOLID Fix #2: Gather room members under lock
    {
        LockGuard lock(mutex_);
        auto it = rooms_.find(room);
        if (it == rooms_.end()) return;

        // Copy room members to vector
        room_members.reserve(it->second.size());
        for (ISession* s : it->second) {
            room_members.push_back(s);
        }
    }

    // Build room-tagged message
    std::string room_prefixed = "[" + room + "] " + message;

    // SOLID Fix #2: Delegate send to BroadcastManager (eliminates duplication)
    broadcast_.broadcast_to_recipients(sender, room_prefixed, room_members);
}

