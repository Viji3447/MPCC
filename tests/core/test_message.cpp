#include "doctest.h"

#include "mpcc/core/message.h"

TEST_CASE("Message serialise adds newline and colon only when payload non-empty") {
    Message a{proto::EXIT, ""};
    CHECK(a.serialise() == std::string("EXIT\n"));

    Message b{proto::MSG, "hello"};
    CHECK(b.serialise() == std::string("MSG:hello\n"));
}

TEST_CASE("Message parse splits only on first colon and strips newlines") {
    Message m1 = Message::parse("EXIT\n");
    CHECK(m1.type == "EXIT");
    CHECK(m1.payload.empty());

    Message m2 = Message::parse("BCAST:[alice]: hello:world\n");
    CHECK(m2.type == "BCAST");
    CHECK(m2.payload == "[alice]: hello:world");

    Message m3 = Message::parse("OK:done\r\n");
    CHECK(m3.type == "OK");
    CHECK(m3.payload == "done");
}

