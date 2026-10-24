#pragma once


#include <iostream>
#include <stdexcept>
using namespace std;

inline int& failures() {
    static int count = 0;
    return count;
}
inline int& checks() {
    static int count = 0;
    return count;
}

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++checks();                                                          \
        if (!(cond)) {                                                       \
            ++failures();                                                    \
            cerr << "FAILED " << __FILE__ << ":" << __LINE__ << "  "    \
                      << #cond << '\n';                                      \
        }                                                                    \
    } while (false)


#define CHECK_THROWS(expr, ExceptionType)                                    \
    do {                                                                     \
        ++checks();                                                          \
        bool threw_ = false;                                                 \
        try {                                                                \
            expr;                                                            \
        } catch (const ExceptionType&) {                                     \
            threw_ = true;                                                   \
        } catch (...) {                                                      \
        }                                                                    \
        if (!threw_) {                                                       \
            ++failures();                                                    \
            cerr << "FAILED " << __FILE__ << ":" << __LINE__            \
                      << "  expected " #ExceptionType " from: " #expr        \
                      << '\n';                                               \
        }                                                                    \
    } while (false)

#define RUN_TEST(fn)                                                         \
    do {                                                                     \
        const int before_ = failures();                                      \
        fn();                                                                \
        cout << (failures() == before_ ? "[ OK ] "                      \
                                              : "[FAIL] ")                   \
                  << #fn << endl;                                       \
    } while (false)

inline int finishTests() {
    cout << checks() << " checks, " << failures()
              << " failed\n";
    return failures() == 0 ? 0 : 1;
}
