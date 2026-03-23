#include "doctest.h"

#include "mpcc/core/cipher.h"
#include <string>

TEST_CASE("Cipher encrypt/decrypt roundtrip") {
    Cipher c(0x5A);
    const char raw[] = "hello world\nwith\0nul"; // includes NUL
    const std::string plain(raw, sizeof(raw) - 1);
    std::string enc = c.encrypt(plain);
    std::string dec = c.decrypt(enc);
    CHECK(dec == plain);
}

TEST_CASE("Cipher with different keys produces different ciphertext") {
    Cipher c1(0x01);
    Cipher c2(0x02);
    const std::string plain = "same";
    CHECK(c1.encrypt(plain) != c2.encrypt(plain));
}

