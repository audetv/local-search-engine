#include <catch2/catch_test_macros.hpp>
#include "search/query_parser.hpp"

using namespace lse;

TEST_CASE("QueryParser: simple term", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет");

    REQUIRE(result.has_value());
    auto &node = *result;
    CHECK(node.op == QueryOp::And);
    REQUIRE(node.terms.size() == 1);
    CHECK(node.terms[0] == "привет");
    CHECK(node.children.empty());
}

TEST_CASE("QueryParser: multiple terms (AND by default)", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет мир");

    REQUIRE(result.has_value());
    auto &node = *result;
    CHECK(node.op == QueryOp::And);
    REQUIRE(node.terms.size() == 2);
    CHECK(node.terms[0] == "привет");
    CHECK(node.terms[1] == "мир");
}

TEST_CASE("QueryParser: OR operator", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет | мир");

    REQUIRE(result.has_value());
    auto &node = *result;
    CHECK(node.op == QueryOp::Or);
    REQUIRE(node.terms.size() == 2);
    CHECK(node.terms[0] == "привет");
    CHECK(node.terms[1] == "мир");
}

TEST_CASE("QueryParser: NOT operator", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет -мир");

    REQUIRE(result.has_value());
    auto &node = *result;
    CHECK(node.op == QueryOp::And);
    REQUIRE(node.terms.size() == 1);
    CHECK(node.terms[0] == "привет");
    REQUIRE(node.children.size() == 1);
    CHECK(node.children[0].negated == true);
    CHECK(node.children[0].terms[0] == "мир");
}

TEST_CASE("QueryParser: phrase", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"привет мир\"");

    REQUIRE(result.has_value());
    auto &node = *result;
    CHECK(node.op == QueryOp::And); // верхний узел — And
    REQUIRE(node.children.size() == 1);
    CHECK(node.children[0].op == QueryOp::Phrase);
    CHECK(node.children[0].terms[0] == "привет мир");
    CHECK(node.terms.empty());
}

TEST_CASE("QueryParser: phrase with other terms", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет \"прекрасный мир\"");

    REQUIRE(result.has_value());
    auto &node = *result;
    CHECK(node.op == QueryOp::And);
    REQUIRE(node.terms.size() == 1);
    CHECK(node.terms[0] == "привет");
    REQUIRE(node.children.size() == 1);
    CHECK(node.children[0].op == QueryOp::Phrase);
    CHECK(node.children[0].terms[0] == "прекрасный мир");
}

TEST_CASE("QueryParser: empty query", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("");

    REQUIRE(!result.has_value());
    CHECK(result.error() == ParseError::EmptyQuery);
}

TEST_CASE("QueryParser: unmatched quotes", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"привет мир");

    REQUIRE(!result.has_value());
    CHECK(result.error() == ParseError::UnmatchedQuotes);
}

TEST_CASE("QueryParser: NOT with phrase", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет -\"прекрасный мир\"");

    REQUIRE(result.has_value());
    auto &node = *result;
    REQUIRE(node.children.size() == 1);
    CHECK(node.children[0].negated == true);
    CHECK(node.children[0].terms[0] == "прекрасный мир");
}

TEST_CASE("QueryParser: lowercase conversion", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("Привет МИР");

    REQUIRE(result.has_value());
    auto &node = *result;
    CHECK(node.terms[0] == "Привет"); // оригинальный регистр
    CHECK(node.terms[1] == "МИР");
}