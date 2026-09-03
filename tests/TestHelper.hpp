#pragma once

#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>

namespace test {

static int g_failed_tests = 0;
static int g_total_tests = 0;

inline void log_failure(const std::string& expr, const std::string& file, int line, const std::string& msg = "") {
    std::cerr << "\033[31m[FAILED]\033[0m " << file << ":" << line << " Assertion failed: " << expr;
    if (!msg.empty()) {
        std::cerr << " (" << msg << ")";
    }
    std::cerr << std::endl;
    g_failed_tests++;
}

#define ASSERT_TRUE(cond) \
    do { \
        test::g_total_tests++; \
        if (!(cond)) { \
            test::log_failure(#cond, __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        test::g_total_tests++; \
        if (cond) { \
            test::log_failure("!(" #cond ")", __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        test::g_total_tests++; \
        if ((a) != (b)) { \
            test::log_failure(#a " == " #b, __FILE__, __LINE__, \
                std::string("Actual: ") + std::to_string(a) + " vs Expected: " + std::to_string(b)); \
        } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        test::g_total_tests++; \
        if (std::string(a) != std::string(b)) { \
            test::log_failure(#a " == " #b, __FILE__, __LINE__, \
                std::string("Actual: '") + std::string(a) + "' vs Expected: '" + std::string(b) + "'"); \
        } \
    } while (0)

#define ASSERT_NEAR(a, b, eps) \
    do { \
        test::g_total_tests++; \
        if (std::abs(static_cast<double>(a) - static_cast<double>(b)) > (eps)) { \
            test::log_failure(#a " ~== " #b, __FILE__, __LINE__, \
                std::string("Diff: ") + std::to_string(std::abs((a) - (b))) + " > eps: " + std::to_string(eps)); \
        } \
    } while (0)

inline int summarize() {
    std::cout << "\n--------------------------------------------------\n";
    if (g_failed_tests == 0) {
        std::cout << "\033[32m[ALL PASSED]\033[0m " << g_total_tests << " assertions verified successfully.\n";
        return 0;
    } else {
        std::cout << "\033[31m[FAILURES DETECTED]\033[0m " << g_failed_tests << " out of "
                  << g_total_tests << " assertions failed.\n";
        return 1;
    }
}

} // namespace test
