#include "doctest.h"

#include "mpcc/server/user_registry.h"

#include <cstdio>
#include <string>

TEST_CASE("UserRegistry persists records across instances") {
    const std::string path = "./build/test_registered_users_persist.dat";
    std::remove(path.c_str());
    std::remove((path + ".tmp").c_str());

    {
        UserRegistry reg(path);
        CHECK(reg.register_user("alice", std::string("xorbytes1")) == true);
        CHECK(reg.register_user("bob", std::string("xorbytes2")) == true);
    }

    {
        UserRegistry reg2(path);
        CHECK(reg2.user_exists("alice") == true);
        CHECK(reg2.user_exists("bob") == true);
        CHECK(reg2.authenticate("alice", std::string("xorbytes1")) == true);
        CHECK(reg2.authenticate("bob", std::string("xorbytes2")) == true);
        CHECK(reg2.authenticate("bob", std::string("wrong")) == false);
    }

    std::remove(path.c_str());
    std::remove((path + ".tmp").c_str());
}

