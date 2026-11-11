#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "lumen/log.hpp"

namespace {

struct Captured {
    lumen::LogLevel level;
    std::string tag;
    std::string message;
};

void capture(lumen::LogLevel level, std::string_view tag, std::string_view message, void* user) {
    auto* out = static_cast<std::vector<Captured>*>(user);
    out->push_back({level, std::string(tag), std::string(message)});
}

}  // namespace

TEST_CASE("log sink receives tag, level and message") {
    std::vector<Captured> got;
    lumen::set_log_sink(&capture, &got);

    lumen::log(lumen::LogLevel::Error, "physics", "body overflow");
    lumen::set_log_sink(nullptr, nullptr);

    REQUIRE(got.size() == 1);
    CHECK(got[0].level == lumen::LogLevel::Error);
    CHECK(got[0].tag == "physics");
    CHECK(got[0].message == "body overflow");
}

TEST_CASE("log without sink does not crash") {
    lumen::set_log_sink(nullptr, nullptr);
    lumen::log(lumen::LogLevel::Info, "test", "console path");
    CHECK(true);
}
