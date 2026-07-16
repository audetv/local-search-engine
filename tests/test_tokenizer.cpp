#include <catch2/catch_test_macros.hpp>
#include "tokenizer/tokenizer.hpp"

using namespace lse;

TEST_CASE("Tokenizer: basic Russian text", "[tokenizer]")
{
    Tokenizer tokenizer("Привет, мир!");
    auto result = tokenizer.tokenize();

    REQUIRE(result.has_value());
    auto &tokens = *result;

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].text == "привет");
    CHECK(tokens[0].position == 0);
    CHECK(tokens[1].text == "мир");
    CHECK(tokens[1].position == 1);
}

TEST_CASE("Tokenizer: mixed case and punctuation", "[tokenizer]")
{
    Tokenizer tokenizer("Мама мыла Раму. И еще: папа!");
    auto result = tokenizer.tokenize();

    REQUIRE(result.has_value());
    auto &tokens = *result;

    std::vector<std::string> expected = {
        "мама", "мыла", "раму", "и", "еще", "папа"};

    REQUIRE(tokens.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        CHECK(tokens[i].text == expected[i]);
        CHECK(tokens[i].position == i);
    }
}

TEST_CASE("Tokenizer: yo normalization", "[tokenizer]")
{
    Tokenizer tokenizer("ёлка ёж Ёж");
    auto result = tokenizer.tokenize();

    REQUIRE(result.has_value());
    auto &tokens = *result;

    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0].text == "елка"); // ё → е
    CHECK(tokens[1].text == "еж");
    CHECK(tokens[2].text == "еж");
}

TEST_CASE("Tokenizer: yo normalization disabled", "[tokenizer]")
{
    Tokenizer::Options opts;
    opts.normalize_yo = false;

    Tokenizer tokenizer("ёлка", opts);
    auto result = tokenizer.tokenize();

    REQUIRE(result.has_value());
    auto &tokens = *result;

    REQUIRE(tokens.size() == 1);
    CHECK(tokens[0].text == "ёлка"); // ё сохраняется
}

TEST_CASE("Tokenizer: hyphen splits words", "[tokenizer]")
{
    Tokenizer tokenizer("кое-как светло-серый");
    auto result = tokenizer.tokenize();

    REQUIRE(result.has_value());
    auto &tokens = *result;

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0].text == "кое");
    CHECK(tokens[1].text == "как");
    CHECK(tokens[2].text == "светло");
    CHECK(tokens[3].text == "серый");
}

TEST_CASE("Tokenizer: apostrophe removed", "[tokenizer]")
{
    Tokenizer tokenizer("д'Артаньян");
    auto result = tokenizer.tokenize();

    REQUIRE(result.has_value());
    auto &tokens = *result;

    REQUIRE(tokens.size() == 1);
    CHECK(tokens[0].text == "дартаньян");
}

TEST_CASE("Tokenizer: digits preserved", "[tokenizer]")
{
    Tokenizer tokenizer("глава 42 битва 1812 года");
    auto result = tokenizer.tokenize();

    REQUIRE(result.has_value());
    auto &tokens = *result;

    REQUIRE(tokens.size() == 5);
    CHECK(tokens[0].text == "глава");
    CHECK(tokens[1].text == "42");
    CHECK(tokens[2].text == "битва");
    CHECK(tokens[3].text == "1812");
    CHECK(tokens[4].text == "года");
}

TEST_CASE("Tokenizer: empty text", "[tokenizer]")
{
    Tokenizer tokenizer("");
    auto result = tokenizer.tokenize();

    REQUIRE(!result.has_value());
    CHECK(result.error() == TokenizerError::EmptyText);
}

TEST_CASE("Tokenizer: only punctuation", "[tokenizer]")
{
    Tokenizer tokenizer("!!! ... ---");
    auto result = tokenizer.tokenize();

    REQUIRE(result.has_value());
    auto &tokens = *result;

    CHECK(tokens.empty());
}

TEST_CASE("Tokenizer: multi-line text", "[tokenizer]")
{
    Tokenizer tokenizer("первая строка\nвторая строка\r\nтретья");
    auto result = tokenizer.tokenize();

    REQUIRE(result.has_value());
    auto &tokens = *result;

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0].text == "первая");
    CHECK(tokens[1].text == "строка");
    CHECK(tokens[2].text == "вторая");
    CHECK(tokens[3].text == "третья");
}

TEST_CASE("Tokenizer: Greek letters for scientific texts", "[tokenizer]")
{
    Tokenizer tokenizer("альфа α и бета β");
    auto result = tokenizer.tokenize();

    REQUIRE(result.has_value());
    auto &tokens = *result;

    REQUIRE(tokens.size() == 6);
    CHECK(tokens[0].text == "альфа");
    CHECK(tokens[1].text == "α");
    CHECK(tokens[2].text == "и");
    CHECK(tokens[3].text == "бета");
    CHECK(tokens[4].text == "β");
}