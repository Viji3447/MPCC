#pragma once
/**
 * @file client_handler.h
 * @brief Per-client pthread entry point; drives the full client lifecycle (MPCC_05).
 *
 * Lifecycle state machine:
 *   CONNECTED → (REGISTER | LOGIN) → CHAT_LOOP → DISCONNECTED
 *
 * Thread entry:
 *   pthread_create() calls ClientHandler::thread_func(void*).
 *   thread_func casts void* → ClientHandler* and calls run().
 *   run() returns only when the client disconnects or sends EXIT.
 *
 * Ownership:
 *   Server creates an owning thread context for each worker thread.
 *   The context owns the ClientHandler and destroys it when the thread exits.
 *   The server tracks worker thread IDs and joins them during shutdown.
 */

#include "mpcc/core/interfaces.h"
#include "mpcc/core/cipher.h"
#include <pthread.h>
#include <string>
#include <vector>
#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
class ClientHandler final {
public:
    /**
     * @param session    Heap-allocated Session for this client (ownership taken).
     * @param broadcast  BroadcastManager reference.
     * @param registry   UserRegistry reference.
     * @param logger     Logger reference.
     * @param cipher     Cipher instance for XOR en/decrypt.
     */
    ClientHandler(ISession*   session,
                  IBroadcast& broadcast,
                  IUserStore& registry,
                  IRooms&     rooms,
                  ILog&       logger,
                  const IEncrypt& cipher);

    ~ClientHandler();

    struct ThreadCtx {
        explicit ThreadCtx(std::unique_ptr<ClientHandler> h) : handler(std::move(h)) {}
        std::unique_ptr<ClientHandler> handler;
    };

    /// pthread entry point — cast void* to ClientHandler* and call run()
    static void* thread_func(void* arg);

    /// Full client lifecycle (registration → chat loop → cleanup)
    void run();

    // Non-copyable
    ClientHandler(const ClientHandler&)            = delete;
    ClientHandler& operator=(const ClientHandler&) = delete;

private:
    ISession*        session_;
    IBroadcast&      broadcast_;
    IUserStore&      registry_;
    IRooms&          rooms_;
    ILog&            logger_;
    const IEncrypt&  cipher_;

    std::string      current_room_;

    // ── Protocol helpers ─────────────────────────────────────────────────────

    /// Send a string over the socket (XOR encrypted)
    bool send_encrypted(const std::string& plain);

    /// Receive a newline-terminated frame from the socket; XOR decrypt
    std::string recv_line();

    // ── State-machine handlers ───────────────────────────────────────────────

    /// Sends menu prompt; returns chosen option (1 or 2), or -1 on error
    int  present_menu();

    /// Returns true if registration succeeds
    bool handle_register();

    /// Returns true if login succeeds
    bool handle_login();

    /// Main chat receive/broadcast loop; returns when client exits
    void recv_loop();

    // ── Input validation ─────────────────────────────────────────────────────
    static bool validate_username(const std::string& u);
    static bool validate_password(const std::string& p);

    // ── SOLID Fix #4: Helper functions replacing goto ──────────────────────────

    /// Try to authenticate client (up to 3 attempts); returns true on success
    bool try_authenticate();

    /// Cleanup: remove from broadcast, close session, delete session_
    void cleanup_and_exit();
};
