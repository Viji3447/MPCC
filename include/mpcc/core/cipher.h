#pragma once
/**
 * @file cipher.h
 * @brief XOR-based symmetric cipher (MPCC_10 / MPCC_11)
 *
 * XOR is its own inverse: encrypt(encrypt(x)) == x.
 * The same Cipher instance is used for both encrypt and decrypt operations.
 *
 * Design note: Cipher is stateless aside from the key; it implements
 * IEncrypt so it can be replaced with AES in a future iteration without
 * changing any caller code.
 *
 * Upgrade path: Swap this class for an AesEncrypt : IEncrypt implementation
 * that uses OpenSSL EVP_aes_128_cbc() — all callers remain unchanged.
 */

#include "mpcc/core/interfaces.h"
#include <cstdint>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Protocol constants (wire format limits)
// ─────────────────────────────────────────────────────────────────────────────
static constexpr std::size_t MAX_USERNAME_LEN = 64;
static constexpr std::size_t MAX_PASS_LEN     = 128;
static constexpr std::size_t MAX_MSG_LEN      = 4096;

// Default XOR key — change per deployment
static constexpr uint8_t DEFAULT_XOR_KEY = 0x5A;

// ─────────────────────────────────────────────────────────────────────────────
class Cipher final : public IEncrypt {
public:
    /**
     * @param key  Single-byte XOR key.  Default: DEFAULT_XOR_KEY (0x5A).
     */
    explicit Cipher(uint8_t key = DEFAULT_XOR_KEY) noexcept;
    ~Cipher() override = default;

    // IEncrypt interface
    std::string encrypt(const std::string& data) const override;
    std::string decrypt(const std::string& data) const override; // identical to encrypt()

    uint8_t key() const noexcept { return key_; }

    // Non-copyable
    Cipher(const Cipher&)            = delete;
    Cipher& operator=(const Cipher&) = delete;

private:
    uint8_t key_;
};
