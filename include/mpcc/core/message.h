#pragma once
/**
 * @file message.h
 * @brief Wire-protocol message serialisation / deserialisation.
 *
 * All messages exchanged between client and server follow the format:
 *
 *   TYPE:payload\n
 *
 * Message types (see HLD §2.2.5):
 *   CHOICE   – menu selection              (client → server)
 *   REG      – registration request        (client → server)
 *   LOGIN    – authentication request      (client → server)
 *   MSG      – chat message                (client → server)
 *   EXIT     – disconnect request          (client → server)
 *   OK       – success acknowledgement     (server → client)
 *   ERR      – failure acknowledgement     (server → client)
 *   BCAST    – broadcast message           (server → client)
 *   MENU     – initial menu prompt         (server → client)
 */

#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Protocol tag literals
// ─────────────────────────────────────────────────────────────────────────────
namespace proto {
    constexpr const char* CHOICE = "CHOICE";
    constexpr const char* REG    = "REG";
    constexpr const char* LOGIN  = "LOGIN";
    constexpr const char* MSG    = "MSG";
    constexpr const char* EXIT   = "EXIT";
    constexpr const char* OK     = "OK";
    constexpr const char* ERR    = "ERR";
    constexpr const char* BCAST  = "BCAST";
    constexpr const char* MENU   = "MENU";
}

// ─────────────────────────────────────────────────────────────────────────────
struct Message {
    std::string type;     ///< Protocol tag (e.g. "REG", "OK", ...)
    std::string payload;  ///< Everything after the first ':' and before '\n'

    // ── Builders ─────────────────────────────────────────────────────────────

    /// Serialise to wire format: "TYPE:payload\n"
    std::string serialise() const;

    // ── Parsers ──────────────────────────────────────────────────────────────

    /**
     * @brief Parse a raw wire-format line (no leading XOR applied here).
     * @param raw  Complete line including the trailing '\n'.
     * @return     Populated Message; type is empty on parse error.
     */
    static Message parse(const std::string& raw);

    // ── Convenience factories ─────────────────────────────────────────────────

    static Message make_reg   (const std::string& user, const std::string& pass);
    static Message make_login (const std::string& user, const std::string& pass);
    static Message make_msg   (const std::string& text);
    static Message make_ok    (const std::string& detail);
    static Message make_err   (const std::string& detail);
    static Message make_bcast (const std::string& sender, const std::string& text);
    static Message make_exit  ();
    static Message make_menu  ();

    bool is_valid() const noexcept { return !type.empty(); }
};
