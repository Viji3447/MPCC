/**
 * @file message.cpp
 * @brief Wire-protocol message serialisation / deserialisation.
 *
 * Wire format (HLD §2.2.5, §4.6.2):
 *   TYPE:payload\n
 *
 * parse() splits on the FIRST ':' only — payload may contain ':'
 * (e.g. BCAST:[alice]: hello:world).
 */

#include "mpcc/core/message.h"
#include <sstream>

// ─────────────────────────────────────────────────────────────────────────────
// Serialise
// ─────────────────────────────────────────────────────────────────────────────
std::string Message::serialise() const
{
    std::string wire;
    wire.reserve(type.size() + 1 + payload.size() + 1);
    wire += type;
    if (!payload.empty()) {
        wire += ':';
        wire += payload;
    }
    wire += '\n';
    return wire;
}

// ─────────────────────────────────────────────────────────────────────────────
// Parse
// ─────────────────────────────────────────────────────────────────────────────
Message Message::parse(const std::string& raw)
{
    Message m;
    // Strip trailing '\n' and '\r'
    std::string line = raw;
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }

    std::size_t colon = line.find(':');
    if (colon == std::string::npos) {
        // No colon — treat entire line as the type (e.g. "EXIT")
        m.type    = line;
        m.payload = "";
    } else {
        m.type    = line.substr(0, colon);
        m.payload = line.substr(colon + 1);
    }
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// Convenience factories
// ─────────────────────────────────────────────────────────────────────────────
Message Message::make_reg(const std::string& user, const std::string& pass)
{
    return {proto::REG, user + ":" + pass};
}

Message Message::make_login(const std::string& user, const std::string& pass)
{
    return {proto::LOGIN, user + ":" + pass};
}

Message Message::make_msg(const std::string& text)
{
    return {proto::MSG, text};
}

Message Message::make_ok(const std::string& detail)
{
    return {proto::OK, detail};
}

Message Message::make_err(const std::string& detail)
{
    return {proto::ERR, detail};
}

Message Message::make_bcast(const std::string& sender, const std::string& text)
{
    return {proto::BCAST, "[" + sender + "]: " + text};
}

Message Message::make_exit()
{
    return {proto::EXIT, ""};
}

Message Message::make_menu()
{
    return {proto::MENU, "1=Register 2=Login"};
}
