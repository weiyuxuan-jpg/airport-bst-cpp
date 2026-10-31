#ifndef TEST_CHECK_H
#define TEST_CHECK_H

// Minimal helper shared by the test programs. CHECK(cond) records a failure and keeps
// going, so one run shows every failing check; finishTests() prints the summary and
// returns main's exit code (0 only if every check passed).

#include <iostream>

inline int g_checks = 0;
inline int g_failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        g_checks++;                                                     \
        if (!(cond)) {                                                  \
            g_failures++;                                               \
            std::cout << "FAIL (line " << __LINE__ << "): " #cond "\n"; \
        }                                                               \
    } while (0)

inline int finishTests() {
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}

#endif
