#pragma once

#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>

namespace test {

inline int& get_failed_count() {
    static int failed = 0;
    return failed;
}

inline int& get_total_count() {
    static int total = 0;
    return total;
}

inline void log_failure(const std::string& expr, const std::string& file, int line, const std::string& msg = "") {
    std::cerr << "\033[31m[FAILED]\033[0m " << file << ":" << line << " Assertion failed: " << expr;
    if (!msg.empty()) {
        std::cerr << " (" << msg << ")";
    }
    std::cerr << std::endl;
    get_failed_count()++;
}

#define ASSERT_TRUE(cond) \
    do { \
        test::get_total_count()++; \
        if (!(cond)) { \
            test::log_failure(#cond, __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        test::get_total_count()++; \
        if (cond) { \
            test::log_failure("!(" #cond ")", __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        test::get_total_count()++; \
        if ((a) != (b)) { \
            test::log_failure(#a " == " #b, __FILE__, __LINE__, \
                std::string("Actual: ") + std::to_string(a) + " vs Expected: " + std::to_string(b)); \
        } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        test::get_total_count()++; \
        if (std::string(a) != std::string(b)) { \
            test::log_failure(#a " == " #b, __FILE__, __LINE__, \
                std::string("Actual: '") + std::string(a) + "' vs Expected: '" + std::string(b) + "'"); \
        } \
    } while (0)

#define ASSERT_NEAR(a, b, eps) \
    do { \
        test::get_total_count()++; \
        if (std::abs(static_cast<double>(a) - static_cast<double>(b)) > (eps)) { \
            test::log_failure(#a " ~== " #b, __FILE__, __LINE__, \
                std::string("Diff: ") + std::to_string(std::abs((a) - (b))) + " > eps: " + std::to_string(eps)); \
        } \
    } while (0)

inline int summarize() {
    std::cout << "\n--------------------------------------------------\n";
    if (get_failed_count() == 0) {
        std::cout << "\033[32m[ALL PASSED]\033[0m " << get_total_count() << " assertions verified successfully.\n";
        return 0;
    } else {
        std::cout << "\033[31m[FAILURES DETECTED]\033[0m " << get_failed_count() << " out of "
                  << get_total_count() << " assertions failed.\n";
        return 1;
    }
}

} // namespace test
