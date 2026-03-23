#include "doctest.h"

#include "mpcc/server/user_registry.h"

#include <cstdio>
#include <string>

TEST_CASE("UserRegistry register/authenticate roundtrip") {
    const std::string path = "./build/test_registered_users.dat";
    std::remove(path.c_str());
    std::remove((path + ".tmp").c_str());

    UserRegistry reg(path);

    CHECK(reg.user_exists("alice") == false);
    CHECK(reg.register_user("alice", std::string("xorbytes")) == true);
    CHECK(reg.user_exists("alice") == true);

    CHECK(reg.authenticate("alice", std::string("xorbytes")) == true);
    CHECK(reg.authenticate("alice", std::string("wrong")) == false);

    // duplicate registration should fail
    CHECK(reg.register_user("alice", std::string("xorbytes")) == false);

    std::remove(path.c_str());
    std::remove((path + ".tmp").c_str());
}

