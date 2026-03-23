/**
 * @file client.cpp
 * @brief MPCC client: connect, authenticate, concurrent send/receive (MPCC_01–MPCC_08).
 *
 * Two threads:
 *   Main thread  – user input → send (send_loop)
 *   Recv thread  – blocking recv() → print received broadcasts (recv_loop)
 *
 * XOR encryption is applied to all data sent to the server.
 * XOR decryption is applied to all data received from the server.
 */

#include "mpcc/client/client.h"
#include "mpcc/core/message.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sstream>

// ─────────────────────────────────────────────────────────────────────────────
Client::Client(const std::string& host,
               int port,
               const std::string& log_path,
               LogLevel log_level)
    : host_(host)
    , port_(port)
    , sock_fd_()
    , cipher_()
    , logger_(log_path, log_level)
    , running_(false)
    , recv_thread_()
{}

Client::~Client()
{
    running_.store(false, std::memory_order_release);
    if (sock_fd_.valid()) {
        sock_fd_.shutdown_rdwr();
        sock_fd_.reset();
    }
    // recv_thread_ joins itself (if started) on destruction.
}

// ─────────────────────────────────────────────────────────────────────────────
void Client::run()
{
    connect_to_server();
    auth_loop();
    send_loop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Connection
// ─────────────────────────────────────────────────────────────────────────────
void Client::connect_to_server()
{
    sock_fd_.reset(::socket(AF_INET, SOCK_STREAM, 0));
    if (!sock_fd_.valid()) {
        logger_.fatal("socket() failed: " + std::string(std::strerror(errno)));
        throw std::runtime_error("socket() failed");
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<uint16_t>(port_));

    if (::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
        logger_.fatal("Invalid address: " + host_);
        throw std::runtime_error("Invalid server address");
    }

    if (::connect(sock_fd_.get(),
                  reinterpret_cast<struct sockaddr*>(&addr),
                  sizeof(addr)) < 0) {
        logger_.fatal("connect() failed: " + std::string(std::strerror(errno)));
        throw std::runtime_error("connect() failed");
    }

    logger_.info("Connected to " + host_ + ":" + std::to_string(port_));
    std::cout << "[MPCC] Connected to server " << host_ << ":" << port_ << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// Auth UI
// ─────────────────────────────────────────────────────────────────────────────
void Client::auth_loop()
{
    bool authenticated = false;

    while (!authenticated) {
        // Server will send a MENU frame — display it
        std::string menu = recv_line();
        if (menu.empty()) throw std::runtime_error("Server disconnected during menu.");

        Message menu_msg = Message::parse(menu);
        if (menu_msg.type == proto::MENU) {
            std::cout << "=== MPCC - Multi Party Conference Chat ===\n";
            std::cout << menu_msg.payload << "\n";
        } else {
            std::cout << menu << "\n";
        }
        std::cout << "Your choice: " << std::flush;

        std::string choice;
        if (!std::getline(std::cin, choice)) break;

        // Send choice as "CHOICE:N\n" encrypted
        send_encrypted(Message{proto::CHOICE, choice}.serialise());

        if (choice == "1") {
            authenticated = do_register();
        } else if (choice == "2") {
            authenticated = do_login();
        } else {
            std::cout << "[ERROR] Invalid choice.\n";
        }
    }
}

bool Client::do_register()
{
    // Server sends "Enter username: " prompt
    std::string prompt = recv_line();
    std::cout << prompt << std::flush;

    std::string username;
    if (!std::getline(std::cin, username)) return false;
    send_encrypted(username + "\n");

    // Server sends "Enter password: " prompt
    prompt = recv_line();
    std::cout << prompt << std::flush;

    std::string password;
    if (!std::getline(std::cin, password)) return false;
    send_encrypted(password + "\n");

    // Server responds OK or ERR
    std::string response = recv_line();
    std::cout << "[SERVER] " << response << "\n";

    Message m = Message::parse(response);
    return m.type == proto::OK;
}

bool Client::do_login()
{
    std::string prompt = recv_line();
    std::cout << prompt << std::flush;

    std::string username;
    if (!std::getline(std::cin, username)) return false;
    send_encrypted(username + "\n");

    prompt = recv_line();
    std::cout << prompt << std::flush;

    std::string password;
    if (!std::getline(std::cin, password)) return false;
    send_encrypted(password + "\n");

    std::string response = recv_line();
    std::cout << "[SERVER] " << response << "\n";

    Message m = Message::parse(response);
    return m.type == proto::OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// Chat
// ─────────────────────────────────────────────────────────────────────────────
void Client::send_loop()
{
    running_.store(true, std::memory_order_release);

    // Receive the "Type your message..." server banner
    std::string banner = recv_line();
    if (!banner.empty()) std::cout << "[SERVER] " << banner << "\n";

    // Spawn background receive thread
    int rc = recv_thread_.start(Client::recv_thread_func, this);
    if (rc != 0) {
        logger_.fatal("pthread_create for recv thread failed.");
        return;
    }

    std::cout << "You: " << std::flush;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "/help") {
            std::cout
                << "\n[COMMANDS]\n"
                << "  /join <room>        - join or create a room\n"
                << "  /leave              - leave the current room\n"
                << "  /pm <user> <msg>    - send private message\n"
                << "  exit | quit         - disconnect\n\n"
                << "You: " << std::flush;
            continue;
        }

        if (line == "exit" || line == "quit") {
            send_encrypted(Message::make_exit().serialise());
            break;
        }

        // Send as MSG:text\n
        send_encrypted(Message::make_msg(line).serialise());
        std::cout << "You: " << std::flush;
    }

    running_.store(false, std::memory_order_release);
    sock_fd_.shutdown_rdwr();
    recv_thread_.join();

    std::cout << "\n[MPCC] Session ended. Goodbye!\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// Receive thread
// ─────────────────────────────────────────────────────────────────────────────
void* Client::recv_thread_func(void* arg)
{
    static_cast<Client*>(arg)->recv_loop();
    return nullptr;
}

void Client::recv_loop()
{
    while (running_.load(std::memory_order_acquire)) {
        std::string line = recv_line();
        if (line.empty()) break;  // disconnect

        Message m = Message::parse(line);
        if (m.type == proto::BCAST) {
            // Print broadcast cleanly above the "You: " prompt
            std::cout << "\r" << m.payload << "\nYou: " << std::flush;
        } else if (m.type == proto::OK || m.type == proto::ERR) {
            std::cout << "\r[SERVER] " << m.payload << "\nYou: " << std::flush;
        }
        // Ignore unknown types silently
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Socket helpers
// ─────────────────────────────────────────────────────────────────────────────
bool Client::send_encrypted(const std::string& plain)
{
    std::string wire = cipher_.encrypt(plain);
    const char* buf  = wire.data();
    std::size_t total = 0;

    while (total < wire.size()) {
        ssize_t n = ::send(sock_fd_.get(), buf + total, wire.size() - total, MSG_NOSIGNAL);
        if (n <= 0) {
            if (n == -1 && errno == EINTR) continue;
            return false;
        }
        total += static_cast<std::size_t>(n);
    }
    return true;
}

std::string Client::recv_line()
{
    std::string buf;
    buf.reserve(256);
    char c = 0;

    while (true) {
        ssize_t n = ::recv(sock_fd_.get(), &c, 1, 0);
        if (n <= 0) {
            if (n == -1 && errno == EINTR) continue;
            return "";
        }
        char plain_c = cipher_.decrypt(std::string(1, c))[0];
        if (plain_c == '\n') break;
        buf.push_back(plain_c);
    }
    return buf;
}
