#include <catch2/catch_test_macros.hpp>
#include "search/query_builder.hpp"

using namespace lse;

TEST_CASE("QueryBuilder: queryString", "[query_builder]")
{
    auto result = QueryBuilder::queryString("привет мир");
    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::And);
    REQUIRE(result->children.size() == 2);
}

TEST_CASE("QueryBuilder: match", "[query_builder]")
{
    auto result = QueryBuilder::match("привет мир");
    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::Or);
    REQUIRE(result->children.size() == 2);
}

TEST_CASE("QueryBuilder: matchPhrase", "[query_builder]")
{
    auto result = QueryBuilder::matchPhrase("привет мир");
    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::Phrase);
    REQUIRE(result->phrase_positions.size() == 2);
}

TEST_CASE("QueryBuilder: matchAll", "[query_builder]")
{
    auto result = QueryBuilder::matchAll();
    CHECK(result.op == QueryOp::And);
    CHECK(result.children.empty());
}