// G-Engine — tests/test_framework.h
// A tiny dependency-free unit test framework.
#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace getest {

struct TestCase {
    const char* name;
    void (*fn)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline int& failureCount() {
    static int failures = 0;
    return failures;
}

inline int& checkCount() {
    static int checks = 0;
    return checks;
}

struct Registrar {
    Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

inline void reportFailure(const char* file, int line, const std::string& message) {
    ++failureCount();
    std::printf("    FAILED %s:%d\n      %s\n", file, line, message.c_str());
}

inline std::string show(const std::string& v) { return "\"" + v + "\""; }
inline std::string show(std::string_view v) { return "\"" + std::string(v) + "\""; }
inline std::string show(const char* v) { return std::string("\"") + v + "\""; }
inline std::string show(bool v) { return v ? "true" : "false"; }
inline std::string show(char32_t v) {
    char b[32];
    std::snprintf(b, sizeof(b), "U+%04X", static_cast<unsigned>(v));
    return b;
}
template <class T>
std::string show(const T& v) {
    if constexpr (std::is_enum_v<T>) {
        return std::to_string(static_cast<long long>(v));
    } else {
        return std::to_string(v);
    }
}

inline int runAll(std::string_view filter = {}) {
    int ran = 0;
    for (const TestCase& t : registry()) {
        if (!filter.empty() && std::string_view(t.name).find(filter) == std::string_view::npos) continue;
        const int before = failureCount();
        t.fn();
        ++ran;
        std::printf("%s %s\n", failureCount() == before ? "[ ok ]" : "[FAIL]", t.name);
    }
    std::printf("\n%d tests, %d checks, %d failures\n", ran, checkCount(), failureCount());
    return failureCount() == 0 ? 0 : 1;
}

}  // namespace getest

#define GE_TEST(name)                                                     \
    static void getest_fn_##name();                                       \
    static ::getest::Registrar getest_reg_##name(#name, getest_fn_##name); \
    static void getest_fn_##name()

#define CHECK(cond)                                                               \
    do {                                                                          \
        ++::getest::checkCount();                                                 \
        if (!(cond)) ::getest::reportFailure(__FILE__, __LINE__, "CHECK(" #cond ")"); \
    } while (0)

#define CHECK_MSG(cond, msg)                                                      \
    do {                                                                          \
        ++::getest::checkCount();                                                 \
        if (!(cond)) ::getest::reportFailure(__FILE__, __LINE__, std::string("CHECK(" #cond ") ") + (msg)); \
    } while (0)

#define CHECK_EQ(a, b)                                                            \
    do {                                                                          \
        ++::getest::checkCount();                                                 \
        const auto& getest_a = (a);                                               \
        const auto& getest_b = (b);                                               \
        if (!(getest_a == getest_b))                                              \
            ::getest::reportFailure(__FILE__, __LINE__,                           \
                std::string("CHECK_EQ(" #a ", " #b ")  got ") + ::getest::show(getest_a) + "  expected " + ::getest::show(getest_b)); \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                     \
    do {                                                                          \
        ++::getest::checkCount();                                                 \
        const double getest_a = static_cast<double>(a);                           \
        const double getest_b = static_cast<double>(b);                           \
        if (!(std::fabs(getest_a - getest_b) <= (eps)))                           \
            ::getest::reportFailure(__FILE__, __LINE__,                           \
                std::string("CHECK_NEAR(" #a ", " #b ")  got ") + std::to_string(getest_a) + "  expected " + std::to_string(getest_b)); \
    } while (0)
