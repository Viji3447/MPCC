#include "doctest.h"

#include "mpcc/core/message.h"

TEST_CASE("Message factory helpers produce expected wire format") {
    CHECK(Message::make_exit().serialise() == std::string("EXIT\n"));
    CHECK(Message::make_menu().serialise() == std::string("MENU:1=Register 2=Login\n"));

    CHECK(Message::make_msg("hi").serialise() == std::string("MSG:hi\n"));
    CHECK(Message::make_ok("done").serialise() == std::string("OK:done\n"));
    CHECK(Message::make_err("bad").serialise() == std::string("ERR:bad\n"));

    CHECK(Message::make_reg("alice", "p").serialise() == std::string("REG:alice:p\n"));
    CHECK(Message::make_login("alice", "p").serialise() == std::string("LOGIN:alice:p\n"));
    CHECK(Message::make_bcast("alice", "hello").serialise() == std::string("BCAST:[alice]: hello\n"));
}

TEST_CASE("Message parse handles empty/whitespace lines") {
    Message m1 = Message::parse("\n");
    CHECK(m1.type.empty());
    CHECK(m1.payload.empty());

    Message m2 = Message::parse("\r\n");
    CHECK(m2.type.empty());
    CHECK(m2.payload.empty());
}

