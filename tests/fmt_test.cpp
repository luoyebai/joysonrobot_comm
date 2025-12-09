#define CATCH_CONFIG_MAIN

// STD
#include <string>
#include <vector>
// TEST
#include "test_tools/catch_amalgamated.hpp"
// FMT
#define FMT_HEADER_ONLY
#include "fmt/core.h"

TEST_CASE("fmt basic formatting", "[FMT]") {
    REQUIRE(fmt::format("hello {}!", "world") == "hello world!");
    REQUIRE(fmt::format("{} + {} = {}", 1, 2, 3) == "1 + 2 = 3");
    REQUIRE(fmt::format("{:.2f}", 3.14159) == "3.14");
    REQUIRE(fmt::format("{:06d}", 42) == "000042");
}
