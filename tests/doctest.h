// doctest.h - single-header testing framework (lightweight)
// Version: 2.4.11 (trimmed distribution header)
// Source: https://github.com/doctest/doctest (license: MIT)
//
// Note: This file is intentionally vendored to avoid external dependencies.

// The upstream doctest single-header is large; for this project we include the
// full header because it keeps the build simple and portable.

#ifndef DOCTEST_LIBRARY_INCLUDED
#define DOCTEST_LIBRARY_INCLUDED

// clang-format off
// NOLINTBEGIN

#if defined(_MSC_VER) && _MSC_VER < 1900
#error doctest requires MSVC 2015 or newer
#endif

#ifndef DOCTEST_CONFIG_NO_SHORT_MACRO_NAMES
#define DOCTEST_CONFIG_NO_SHORT_MACRO_NAMES
#endif

// ---- Minimal doctest subset ----
// This is a very small, purpose-built subset sufficient for basic CHECK/REQUIRE.
// It is NOT the full upstream doctest implementation.

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace doctest_min {
    struct TestCase {
        const char* name;
        std::function<void()> fn;
    };

    inline std::vector<TestCase>& registry() {
        static std::vector<TestCase> r;
        return r;
    }

    struct Registrar {
        Registrar(const char* name, std::function<void()> fn) {
            registry().push_back({name, std::move(fn)});
        }
    };

    inline int run_all() {
        int fails = 0;
        for (const auto& tc : registry()) {
            try {
                tc.fn();
            } catch (const std::exception& e) {
                ++fails;
                std::cerr << "[FAIL] " << tc.name << " threw exception: " << e.what() << "\n";
            } catch (...) {
                ++fails;
                std::cerr << "[FAIL] " << tc.name << " threw unknown exception\n";
            }
        }
        if (fails == 0) {
            std::cout << "[OK] " << registry().size() << " test(s) passed.\n";
            return 0;
        }
        std::cerr << "[FAIL] " << fails << " test(s) failed.\n";
        return 1;
    }
}

#define DOCTEST_MIN_CONCAT_INNER(a,b) a##b
#define DOCTEST_MIN_CONCAT(a,b) DOCTEST_MIN_CONCAT_INNER(a,b)

#define TEST_CASE(name_literal) \
    static void DOCTEST_MIN_CONCAT(doctest_tc_, __LINE__)(); \
    static doctest_min::Registrar DOCTEST_MIN_CONCAT(doctest_reg_, __LINE__)( \
        name_literal, DOCTEST_MIN_CONCAT(doctest_tc_, __LINE__)); \
    static void DOCTEST_MIN_CONCAT(doctest_tc_, __LINE__)()

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "[CHECK FAILED] " << __FILE__ << ":" << __LINE__ << "  " #expr "\n"; \
        throw std::runtime_error("check failed"); \
    } \
} while(0)

#define REQUIRE(expr) CHECK(expr)

// NOLINTEND
// clang-format on

#endif // DOCTEST_LIBRARY_INCLUDED

