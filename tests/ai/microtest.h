// microtest.h -- a ~30-line, dependency-free test harness.
// No framework to vendor; just CHECK / CHECK_EQ and a summary that returns an
// exit code (0 = all passed, 1 = at least one failure).

#ifndef MICROTEST_H
#define MICROTEST_H

#include <cstdio>

namespace microtest {
inline int& Checks() { static int n = 0; return n; }
inline int& Fails()  { static int n = 0; return n; }
inline int& Known()  { static int n = 0; return n; }  // documented latent bugs
inline int  Summary() {
    std::printf("\n[ai_tests] %d checks, %d failure%s, %d known-bug%s\n",
                Checks(), Fails(), Fails() == 1 ? "" : "s",
                Known(), Known() == 1 ? "" : "s");
    return Fails() == 0 ? 0 : 1;
}
} // namespace microtest

#define CHECK(expr)                                                            \
    do {                                                                       \
        ++microtest::Checks();                                                 \
        if (!(expr)) {                                                         \
            ++microtest::Fails();                                              \
            std::printf("FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, #expr); \
        }                                                                      \
    } while (0)

#define CHECK_EQ(a, b)                                                         \
    do {                                                                       \
        ++microtest::Checks();                                                 \
        long long _a = (long long)(a), _b = (long long)(b);                    \
        if (_a != _b) {                                                        \
            ++microtest::Fails();                                              \
            std::printf("FAIL %s:%d  CHECK_EQ(%s, %s)  got %lld vs %lld\n",    \
                        __FILE__, __LINE__, #a, #b, _a, _b);                   \
        }                                                                      \
    } while (0)

// Documents a KNOWN latent bug. `shouldHold` is the property that SHOULD be true
// but currently is NOT. Stays green (records a known-bug) while the bug exists;
// FAILS loudly (XPASS) once the property starts holding -- so it auto-promotes
// to a real regression guard the moment the production bug is fixed.
#define KNOWN_BUG(shouldHold, desc)                                            \
    do {                                                                       \
        ++microtest::Checks();                                                 \
        if (shouldHold) {                                                      \
            ++microtest::Fails();                                              \
            std::printf("XPASS %s:%d  KNOWN_BUG now holds -- promote to CHECK: %s\n", \
                        __FILE__, __LINE__, desc);                             \
        } else {                                                              \
            ++microtest::Known();                                              \
            std::printf("known-bug %s:%d  %s\n", __FILE__, __LINE__, desc);    \
        }                                                                      \
    } while (0)

#endif // MICROTEST_H
