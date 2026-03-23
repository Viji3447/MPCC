#include "doctest.h"

#include "mpcc/server/room_manager.h"
#include "mpcc/core/cipher.h"

namespace {
    struct FakeSession : ISession {
        std::string name;
        std::string last_msg;
        bool active{true};

        explicit FakeSession(std::string n) : name(std::move(n)) {}

        bool send_msg(const std::string& msg) override {
            last_msg = msg;
            return true;
        }
        void close_session() override { active = false; }
        int  get_fd() const override { return -1; }
        std::string get_username() const override { return name; }
        std::string get_ip() const override { return "0.0.0.0"; }
        bool is_active() const override { return active; }
        void set_username(const std::string& username) override {
            // SOLID Fix #1: Implement in FakeSession
            // Allow setting for test flexibility
            name = username;
        }
    };

    // SOLID Fix #2: FakeBroadcast for testing RoomManager delegation
    struct FakeBroadcast : IBroadcast {
        void add_client(ISession* /*session*/) override {}
        void remove_client(ISession* /*session*/) override {}
        void broadcast(const std::string& /*sender*/, const std::string& /*message*/) override {}
        void send_private(const std::string& /*from*/, const std::string& /*to*/,
                         const std::string& /*message*/) override {}

        std::vector<ISession*> last_recipients;
        std::string last_message;

        void broadcast_to_recipients(const std::string& /*sender*/,
                                    const std::string& message,
                                    const std::vector<ISession*>& recipients) override {
            last_message = message;
            last_recipients = recipients;
        }
    };
}

TEST_CASE("RoomManager joins, leaves, and broadcasts") {
    Cipher cipher;
    FakeBroadcast broadcast;
    RoomManager rooms(cipher, broadcast);

    FakeSession alice("alice");
    FakeSession bob("bob");

    rooms.join_room("room1", &alice);
    rooms.join_room("room1", &bob);

    rooms.broadcast_to_room("room1", "alice", "hello");

    // Should delegate to broadcast with room-tagged message
    CHECK(broadcast.last_recipients.size() == 2);
    CHECK(broadcast.last_message.find("[room1]") != std::string::npos);
}

