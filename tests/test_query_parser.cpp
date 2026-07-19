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

TEST_CASE("QueryParser: simple phrase", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"привет мир\"");

    REQUIRE(result.has_value());
    auto &node = *result;
    CHECK(node.op == QueryOp::And);
    REQUIRE(node.children.size() == 1);
    auto &phrase = node.children[0];
    CHECK(phrase.op == QueryOp::Phrase);
    REQUIRE(phrase.phrase_positions.size() == 2);
    CHECK(phrase.phrase_positions[0].alternatives[0] == "привет");
    CHECK(phrase.phrase_positions[1].alternatives[0] == "мир");
}

TEST_CASE("QueryParser: phrase with group alternatives", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"я люблю (кофе | чай)\"");

    REQUIRE(result.has_value());
    auto &phrase = result->children[0];
    CHECK(phrase.op == QueryOp::Phrase);
    REQUIRE(phrase.phrase_positions.size() == 3);
    CHECK(phrase.phrase_positions[0].alternatives[0] == "я");
    CHECK(phrase.phrase_positions[1].alternatives[0] == "люблю");
    // Третья позиция — группа альтернатив
    REQUIRE(phrase.phrase_positions[2].alternatives.size() == 2);
    CHECK(phrase.phrase_positions[2].alternatives[0] == "кофе");
    CHECK(phrase.phrase_positions[2].alternatives[1] == "чай");
}

TEST_CASE("QueryParser: complex phrase with multiple groups", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"я люблю (кофе | чай) с (молоком | сахаром)\"");

    REQUIRE(result.has_value());
    auto &phrase = result->children[0];
    REQUIRE(phrase.phrase_positions.size() == 5);
    CHECK(phrase.phrase_positions[0].alternatives[0] == "я");
    CHECK(phrase.phrase_positions[1].alternatives[0] == "люблю");
    CHECK(phrase.phrase_positions[2].alternatives.size() == 2);
    CHECK(phrase.phrase_positions[3].alternatives[0] == "с");
    CHECK(phrase.phrase_positions[4].alternatives.size() == 2);
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
}

TEST_CASE("QueryParser: NOT with phrase", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("привет -\"прекрасный мир\"");

    REQUIRE(result.has_value());
    auto &node = *result;
    REQUIRE(node.children.size() == 1);
    CHECK(node.children[0].negated == true);
    CHECK(node.children[0].op == QueryOp::Phrase);
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

TEST_CASE("QueryParser: unmatched group", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"привет (мир\"");

    REQUIRE(!result.has_value());
    // Парсер интерпретирует это как UnexpectedCharacter
    CHECK(result.error() == ParseError::UnexpectedCharacter);
}

TEST_CASE("QueryParser: unclosed group in phrase", "[query_parser]")
{
    QueryParser parser;
    auto result = parser.parse("\"привет (мир\"");

    REQUIRE(!result.has_value());
}