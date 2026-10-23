#pragma once

// Minimal dependency-free test helpers (no external test framework needed).

#include <iostream>
#include <stdexcept>

namespace testutil {
inline int& failures() {
    static int count = 0;
    return count;
}
inline int& checks() {
    static int count = 0;
    return count;
}
}  // namespace testutil

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++testutil::checks();                                                \
        if (!(cond)) {                                                       \
            ++testutil::failures();                                          \
            std::cerr << "FAILED " << __FILE__ << ":" << __LINE__ << "  "    \
                      << #cond << '\n';                                      \
        }                                                                    \
    } while (false)

// Passes only if `expr` throws exactly `ExceptionType` (or a subclass).
#define CHECK_THROWS(expr, ExceptionType)                                    \
    do {                                                                     \
        ++testutil::checks();                                                \
        bool threw_ = false;                                                 \
        try {                                                                \
            expr;                                                            \
        } catch (const ExceptionType&) {                                     \
            threw_ = true;                                                   \
        } catch (...) {                                                      \
        }                                                                    \
        if (!threw_) {                                                       \
            ++testutil::failures();                                          \
            std::cerr << "FAILED " << __FILE__ << ":" << __LINE__            \
                      << "  expected " #ExceptionType " from: " #expr        \
                      << '\n';                                               \
        }                                                                    \
    } while (false)

#define RUN_TEST(fn)                                                         \
    do {                                                                     \
        const int before_ = testutil::failures();                            \
        fn();                                                                \
        std::cout << (testutil::failures() == before_ ? "[ OK ] "            \
                                                      : "[FAIL] ")           \
                  << #fn << std::endl;                                       \
    } while (false)

inline int finishTests() {
    std::cout << testutil::checks() << " checks, " << testutil::failures()
              << " failed\n";
    return testutil::failures() == 0 ? 0 : 1;
}
