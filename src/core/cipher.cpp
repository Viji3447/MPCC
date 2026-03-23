/**
 * @file cipher.cpp
 * @brief XOR-based symmetric cipher implementation (MPCC_10 / MPCC_11).
 *
 * XOR cipher properties:
 *   encrypt(encrypt(x, k), k) == x   (own inverse)
 *   Therefore encrypt() and decrypt() share the same implementation.
 */

#include "mpcc/core/cipher.h"

// ─────────────────────────────────────────────────────────────────────────────
Cipher::Cipher(uint8_t key) noexcept
    : key_(key)
{}

// ─────────────────────────────────────────────────────────────────────────────
std::string Cipher::encrypt(const std::string& data) const
{
    std::string out;
    out.reserve(data.size());
    for (unsigned char c : data) {
        out.push_back(static_cast<char>(c ^ key_));
    }
    return out;
}

// XOR is its own inverse — decrypt IS encrypt
std::string Cipher::decrypt(const std::string& data) const
{
    return encrypt(data);
}
