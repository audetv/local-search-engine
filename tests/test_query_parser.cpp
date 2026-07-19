#include <catch2/catch_test_macros.hpp>
#include "search/query_parser.hpp"

using namespace lse;

TEST_CASE("QueryParser: simple term", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет");

    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::Term);
    CHECK(result->term == "привет");
}

TEST_CASE("QueryParser: AND by default", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет мир");

    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::And);
    REQUIRE(result->children.size() == 2);
    CHECK(result->children[0].op == QueryOp::Term);
    CHECK(result->children[0].term == "привет");
    CHECK(result->children[1].term == "мир");
}

TEST_CASE("QueryParser: OR operator", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет | мир");

    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::Or);
    REQUIRE(result->children.size() == 2);
    CHECK(result->children[0].term == "привет");
    CHECK(result->children[1].term == "мир");
}

TEST_CASE("QueryParser: AND has priority over OR", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("кофе | чай круассан");

    REQUIRE(result.has_value());
    // Должно быть: кофе OR (чай AND круассан)
    CHECK(result->op == QueryOp::Or);
    REQUIRE(result->children.size() == 2);
    CHECK(result->children[0].op == QueryOp::Term);
    CHECK(result->children[0].term == "кофе");
    // Второй child — AND узел
    CHECK(result->children[1].op == QueryOp::And);
    CHECK(result->children[1].children[0].term == "чай");
    CHECK(result->children[1].children[1].term == "круассан");
}

TEST_CASE("QueryParser: NOT operator", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет -мир");

    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::And);
    REQUIRE(result->children.size() == 2);
    CHECK(result->children[0].term == "привет");
    CHECK(result->children[1].op == QueryOp::Not);
    CHECK(result->children[1].children[0].term == "мир");
}

TEST_CASE("QueryParser: NOT with phrase", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет -\"прекрасный мир\"");

    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::And);
    REQUIRE(result->children.size() == 2);
    CHECK(result->children[0].term == "привет");
    CHECK(result->children[1].op == QueryOp::Not);
    CHECK(result->children[1].children[0].op == QueryOp::Phrase);
}

TEST_CASE("QueryParser: simple phrase", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"привет мир\"");

    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::Phrase);
    REQUIRE(result->phrase_positions.size() == 2);
    CHECK(result->phrase_positions[0].alternatives[0] == "привет");
    CHECK(result->phrase_positions[1].alternatives[0] == "мир");
}

TEST_CASE("QueryParser: phrase with group alternatives", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"я люблю (кофе | чай)\"");

    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::Phrase);
    REQUIRE(result->phrase_positions.size() == 3);
    CHECK(result->phrase_positions[0].alternatives[0] == "я");
    CHECK(result->phrase_positions[1].alternatives[0] == "люблю");
    REQUIRE(result->phrase_positions[2].alternatives.size() == 2);
    CHECK(result->phrase_positions[2].alternatives[0] == "кофе");
    CHECK(result->phrase_positions[2].alternatives[1] == "чай");
}

TEST_CASE("QueryParser: complex phrase with multiple groups", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"я люблю (кофе | чай) с (молоком | сахаром)\"");

    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::Phrase);
    REQUIRE(result->phrase_positions.size() == 5);
}

TEST_CASE("QueryParser: phrase with other terms", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет \"прекрасный мир\"");

    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::And);
    REQUIRE(result->children.size() == 2);
    CHECK(result->children[0].term == "привет");
    CHECK(result->children[1].op == QueryOp::Phrase);
}

TEST_CASE("QueryParser: grouping with parentheses", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("(кофе | чай) круассан");

    REQUIRE(result.has_value());
    // (кофе | чай) AND круассан
    CHECK(result->op == QueryOp::And);
    REQUIRE(result->children.size() == 2);
    CHECK(result->children[0].op == QueryOp::Or);
    CHECK(result->children[1].term == "круассан");
}

TEST_CASE("QueryParser: nested groups", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("(кофе | (чай молоко)) круассан");

    REQUIRE(result.has_value());
    CHECK(result->op == QueryOp::And);
    REQUIRE(result->children.size() == 2);
    CHECK(result->children[0].op == QueryOp::Or);
    CHECK(result->children[1].term == "круассан");
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

TEST_CASE("QueryParser: empty group", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"привет ()\"");

    REQUIRE(!result.has_value());
    CHECK(result.error() == ParseError::EmptyGroup);
}

TEST_CASE("QueryParser: unbalanced parentheses", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("(кофе | чай");

    REQUIRE(!result.has_value());
}