/**
 * @file client_handler.cpp
 * @brief Per-client pthread: registration, authentication, chat loop (MPCC_02–MPCC_08).
 *
 * State machine:
 *   CONNECTED
 *     └─► present_menu()
 *           ├─► 1 → handle_register() ─► [success] ─► recv_loop()
 *           └─► 2 → handle_login()    ─► [success] ─► recv_loop()
 *   DISCONNECTED (recv() returns 0, or client sends EXIT)
 *     └─► cleanup: remove_client, close fd, delete session, return
 */

#include "mpcc/server/client_handler.h"
#include "mpcc/core/session.h"
#include "mpcc/core/message.h"
#include "mpcc/core/cipher.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
ClientHandler::ClientHandler(ISession*       session,
                             IBroadcast&     broadcast,
                             IUserStore&     registry,
                             IRooms&         rooms,
                             ILog&           logger,
                             const IEncrypt& cipher)
    : session_(session)
    , broadcast_(broadcast)
    , registry_(registry)
    , rooms_(rooms)
    , logger_(logger)
    , cipher_(cipher)
    , current_room_("lobby")
{}

ClientHandler::~ClientHandler() = default;

// ─────────────────────────────────────────────────────────────────────────────
// pthread entry point (SOLID Fix #9: Exception-safe unique_ptr ownership)
// ─────────────────────────────────────────────────────────────────────────────
void* ClientHandler::thread_func(void* arg)
{
    // Recover unique_ptr from raw pointer, ensuring cleanup at thread exit
    auto ctx = std::unique_ptr<ThreadCtx>(static_cast<ThreadCtx*>(arg));
    if (ctx && ctx->handler) {
        ctx->handler->run();
    }
    // ctx automatically destroyed when exiting scope (unique_ptr destructor)
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Full client lifecycle (SOLID Fix #4: removed goto, added early returns + RAII)
// ─────────────────────────────────────────────────────────────────────────────
void ClientHandler::run()
{
    logger_.info("ClientHandler::run() – new client from " + session_->get_ip());

    // Try to authenticate; if successful, enter chat loop
    if (!try_authenticate()) {
        cleanup_and_exit();
        return;
    }

    // Authenticated — enter chat loop
    recv_loop();

    // Cleanup on scope exit
    cleanup_and_exit();
}

// ─────────────────────────────────────────────────────────────────────────────
// Protocol helpers
// ─────────────────────────────────────────────────────────────────────────────
bool ClientHandler::send_encrypted(const std::string& plain)
{
    std::string wire = cipher_.encrypt(plain);
    return session_->send_msg(wire);
}

std::string ClientHandler::recv_line()
{
    // Receive bytes until '\n' or disconnect
    std::string buf;
    buf.reserve(256);
    char c = 0;

    while (true) {
        ssize_t n = ::recv(session_->get_fd(), &c, 1, 0);
        if (n <= 0) {
            // 0 = graceful disconnect; -1 = error
            if (n == -1 && errno == EINTR) continue;
            return "";   // signal disconnect to caller
        }

        // XOR decrypt in place
        char plain_c = cipher_.decrypt(std::string(1, c))[0];
        if (plain_c == '\n') break;
        buf.push_back(plain_c);
    }
    return buf;
}

// ─────────────────────────────────────────────────────────────────────────────
int ClientHandler::present_menu()
{
    // Send a protocol-compliant MENU frame so the client can parse it reliably.
    if (!send_encrypted(Message::make_menu().serialise())) return -1;

    std::string line = recv_line();
    if (line.empty()) return -1;

    // Parse "CHOICE:N" or bare "N"
    Message m = Message::parse(line);
    std::string choice_str = (m.type == proto::CHOICE) ? m.payload : m.type;

    try {
        return std::stoi(choice_str);
    } catch (...) {
        return 0;   // invalid; caller will send error
    }
}

// ─────────────────────────────────────────────────────────────────────────────
bool ClientHandler::handle_register()
{
    if (!send_encrypted("Enter username: \n")) return false;
    std::string username = recv_line();
    if (username.empty()) return false;

    if (!validate_username(username)) {
        send_encrypted(Message::make_err(
            "Invalid username. Use [a-zA-Z0-9_], max 64 chars.").serialise());
        return false;
    }

    if (!send_encrypted("Enter password: \n")) return false;
    std::string password = recv_line();
    if (password.empty()) return false;

    if (!validate_password(password)) {
        send_encrypted(Message::make_err(
            "Invalid password. Max 128 chars, no colons.").serialise());
        return false;
    }

    // XOR-encrypt password before storage
    std::string enc_pass = cipher_.encrypt(password);

    if (!registry_.register_user(username, enc_pass)) {
        logger_.warning("Registration failed – user already exists: " + username);
        send_encrypted(Message::make_err("User already exists.").serialise());
        return false;
    }

    logger_.info("New user registered: " + username + " from " + session_->get_ip());

    // SOLID Fix #1: No downcast needed - set_username() now in ISession
    session_->set_username(username);
    broadcast_.add_client(session_);

    send_encrypted(Message::make_ok("Registration successful. Welcome, " + username + "!").serialise());
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
bool ClientHandler::handle_login()
{
    if (!send_encrypted("Enter username: \n")) return false;
    std::string username = recv_line();
    if (username.empty()) return false;

    if (!send_encrypted("Enter password: \n")) return false;
    std::string password = recv_line();
    if (password.empty()) return false;

    std::string enc_pass = cipher_.encrypt(password);

    if (!registry_.authenticate(username, enc_pass)) {
        logger_.warning("Login failed for user: " + username +
                        " from " + session_->get_ip());
        send_encrypted(Message::make_err("Invalid credentials.").serialise());
        return false;
    }

    logger_.info("User logged in: " + username + " from " + session_->get_ip());

    // SOLID Fix #1: No downcast needed - set_username() now in ISession
    session_->set_username(username);
    broadcast_.add_client(session_);

    send_encrypted(Message::make_ok("Login successful. Welcome back, " + username + "!").serialise());
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
void ClientHandler::recv_loop()
{
    logger_.info("recv_loop started for user: " + session_->get_username());

    rooms_.join_room(current_room_, session_);

    send_encrypted("Type your message and press Enter. Type 'exit' to quit.\n"
                   "Commands: /join room, /leave, /pm user msg\n");

    while (true) {
        std::string line = recv_line();

        if (line.empty()) {
            // Disconnect or error
            break;
        }

        // Parse protocol message
        Message m = Message::parse(line);

        if (m.type == proto::EXIT ||
            m.payload == "exit"   ||
            m.payload == "quit"   ||
            line == "exit"        ||
            line == "quit") {
            logger_.info("User " + session_->get_username() + " sent EXIT.");
            send_encrypted(Message::make_ok("Goodbye!").serialise());
            break;
        }

        // Extract actual message text
        std::string text = (m.type == proto::MSG) ? m.payload : line;

        if (text.empty()) continue;

        // Handle commands starting with '/'
        if (!text.empty() && text[0] == '/') {
            // very simple parsing: /cmd arg1 arg2...
            std::istringstream iss(text);
            std::string cmd;
            iss >> cmd;

            if (cmd == "/join") {
                std::string room;
                iss >> room;
                if (room.empty()) {
                    send_encrypted(Message::make_err("Usage: /join room").serialise());
                } else {
                    if (!current_room_.empty()) {
                        rooms_.leave_room(current_room_, session_);
                    }
                    current_room_ = room;
                    rooms_.join_room(current_room_, session_);
                    send_encrypted(Message::make_ok("Joined room: " + current_room_).serialise());
                }
                continue;
            } else if (cmd == "/leave") {
                if (!current_room_.empty()) {
                    rooms_.leave_room(current_room_, session_);
                    current_room_.clear();
                    send_encrypted(Message::make_ok("Left current room.").serialise());
                } else {
                    send_encrypted(Message::make_err("Not in a room.").serialise());
                }
                continue;
            } else if (cmd == "/pm") {
                std::string target;
                iss >> target;
                std::string pm_text;
                std::getline(iss, pm_text);
                if (!pm_text.empty() && pm_text[0] == ' ') pm_text.erase(0,1);

                if (target.empty() || pm_text.empty()) {
                    send_encrypted(Message::make_err("Usage: /pm user message").serialise());
                    continue;
                }

                broadcast_.send_private(session_->get_username(), target, pm_text);
                continue;
            }
            // Unknown /command
            send_encrypted(Message::make_err("Unknown command.").serialise());
            continue;
        }

        if (text.size() > MAX_MSG_LEN) {
            send_encrypted(Message::make_err("Message too long (max 4096 chars).").serialise());
            continue;
        }

        logger_.debug("MSG from " + session_->get_username() + ": " + text);

        if (!current_room_.empty()) {
            // Prepend room tag for recipients; encryption is applied later.
            std::string room_tagged = "[" + current_room_ + "] " + text;
            rooms_.broadcast_to_room(current_room_, session_->get_username(), room_tagged);
        } else {
            broadcast_.broadcast(session_->get_username(), text);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Input validation (MPCC security constraints)
// ─────────────────────────────────────────────────────────────────────────────
bool ClientHandler::validate_username(const std::string& u)
{
    if (u.empty() || u.size() > MAX_USERNAME_LEN) return false;
    for (char c : u) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
    }
    return true;
}

bool ClientHandler::validate_password(const std::string& p)
{
    if (p.empty() || p.size() > MAX_PASS_LEN) return false;
    // Disallow ':' as it is the protocol field separator
    if (p.find(':') != std::string::npos) return false;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// SOLID Fix #4: Helper functions replacing goto pattern
// ─────────────────────────────────────────────────────────────────────────────

bool ClientHandler::try_authenticate()
{
    // Attempt authentication; retry up to 3 times
    static constexpr int MAX_AUTH_ATTEMPTS = 3;

    for (int attempt = 0; attempt < MAX_AUTH_ATTEMPTS; ++attempt) {
        int choice = present_menu();
        if (choice < 0) return false;  // connection lost

        if (choice == 1) {
            if (handle_register()) return true;  // Success!
        } else if (choice == 2) {
            if (handle_login()) return true;  // Success!
        } else {
            send_encrypted(Message::make_err("Invalid choice. Please enter 1 or 2.").serialise());
        }
    }

    // Max attempts reached
    send_encrypted(Message::make_err("Authentication failed. Disconnecting.").serialise());
    return false;
}

void ClientHandler::cleanup_and_exit()
{
    if (!session_) return;

    logger_.info("ClientHandler: client " + session_->get_username() + " disconnecting.");
    rooms_.leave_all(session_);
    broadcast_.remove_client(session_);
    session_->close_session();
    // Note: session_ is deleted here only if ClientHandler owns it.
    // In our design Session is heap-allocated and owned by ClientHandler.
    delete session_;
    session_ = nullptr;
}
