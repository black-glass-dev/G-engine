#include <doctest/doctest.h>

#include "lumen/version.hpp"

TEST_CASE("engine version string is stable") {
    CHECK(lumen::engine_version_string() == "Lumen2D 0.1.0");
    CHECK(lumen::kVersionMajor == 0);
    CHECK(lumen::kVersionMinor == 1);
}
