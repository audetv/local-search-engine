#include <catch2/catch_test_macros.hpp>
#include <expected>
#include <span>

TEST_CASE("Infrastructure is working", "[infrastructure]")
{
    SECTION("Basic assertions work")
    {
        REQUIRE(1 + 1 == 2);
    }

    SECTION("C++20 features available")
    {
        int arr[] = {1, 2, 3, 4, 5};
        std::span<int> s(arr);
        REQUIRE(s.size() == 5);
        REQUIRE(s[0] == 1);
    }

    SECTION("std::expected available")
    {
        std::expected<int, std::string> result = 42;
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 42);

        std::expected<int, std::string> error =
            std::unexpected<std::string>("something went wrong");
        REQUIRE(!error.has_value());
        REQUIRE(error.error() == "something went wrong");
    }
}