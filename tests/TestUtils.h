#pragma once


#include <iostream>
#include <stdexcept>
using namespace std;

namespace testutil {
inline int& failures() {
    static int count = 0;
    return count;
}
inline int& checks() {
    static int count = 0;
    return count;
}
}  

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++testutil::checks();                                                \
        if (!(cond)) {                                                       \
            ++testutil::failures();                                          \
            cerr << "FAILED " << __FILE__ << ":" << __LINE__ << "  "    \
                      << #cond << '\n';                                      \
        }                                                                    \
    } while (false)


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
            cerr << "FAILED " << __FILE__ << ":" << __LINE__            \
                      << "  expected " #ExceptionType " from: " #expr        \
                      << '\n';                                               \
        }                                                                    \
    } while (false)

#define RUN_TEST(fn)                                                         \
    do {                                                                     \
        const int before_ = testutil::failures();                            \
        fn();                                                                \
        cout << (testutil::failures() == before_ ? "[ OK ] "            \
                                                      : "[FAIL] ")           \
                  << #fn << endl;                                       \
    } while (false)

inline int finishTests() {
    cout << testutil::checks() << " checks, " << testutil::failures()
              << " failed\n";
    return testutil::failures() == 0 ? 0 : 1;
}
