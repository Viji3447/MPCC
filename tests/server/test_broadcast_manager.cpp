#include "doctest.h"

#include "mpcc/server/broadcast_manager.h"
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
}

TEST_CASE("BroadcastManager global broadcast excludes sender") {
    Cipher cipher;
    BroadcastManager mgr(cipher);

    FakeSession alice("alice");
    FakeSession bob("bob");
    mgr.add_client(&alice);
    mgr.add_client(&bob);

    mgr.broadcast("alice", "hello");

    CHECK(alice.last_msg.empty());
    CHECK(!bob.last_msg.empty());
}

TEST_CASE("BroadcastManager send_private delivers only to target") {
    Cipher cipher;
    BroadcastManager mgr(cipher);

    FakeSession alice("alice");
    FakeSession bob("bob");
    FakeSession charlie("charlie");
    mgr.add_client(&alice);
    mgr.add_client(&bob);
    mgr.add_client(&charlie);

    mgr.send_private("alice", "bob", "secret");

    CHECK(!bob.last_msg.empty());
    CHECK(alice.last_msg.empty());
    CHECK(charlie.last_msg.empty());
}

