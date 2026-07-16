#include <catch2/catch_test_macros.hpp>
#include "parser/txt_parser.hpp"
#include <fstream>
#include <filesystem>

using namespace lse;

// Вспомогательная функция: записать строку в файл и распарсить
static auto parse_test_file(const std::string &content,
                            const std::string &filename)
{
    auto path = std::filesystem::temp_directory_path() / filename;
    std::ofstream(path, std::ios::binary).write(content.data(), content.size());

    TxtParser parser;
    auto result = parser.parse(path);

    std::filesystem::remove(path);
    return result;
}

TEST_CASE("TxtParser: UTF-8 file", "[parser]")
{
    std::string content = "Привет, мир! Hello, world!";
    auto result = parse_test_file(content, "test_utf8.txt");

    REQUIRE(result.has_value());
    CHECK(result->encoding == "UTF-8");
    CHECK(result->content == content);
    CHECK(result->title == "test_utf8.txt");
}

TEST_CASE("TxtParser: CP1251 file", "[parser]")
{
    // "Привет мир" в кодировке CP1251
    std::vector<uint8_t> cp1251_data = {
        0xCF, 0xF0, 0xE8, 0xE2, 0xE5, 0xF2, // Привет
        0x20,                               // пробел
        0xEC, 0xE8, 0xF0                    // мир
    };

    TxtParser parser;
    auto result = parser.parseFromMemory(cp1251_data, "test_cp1251.txt");

    REQUIRE(result.has_value());
    CHECK(result->encoding == "CP1251");
    CHECK(result->content == "Привет мир");
}

TEST_CASE("TxtParser: empty file", "[parser]")
{
    std::vector<uint8_t> empty;

    TxtParser parser;
    auto result = parser.parseFromMemory(empty, "empty.txt");

    REQUIRE(!result.has_value());
    CHECK(result.error() == ParserError::EmptyContent);
}

TEST_CASE("TxtParser: non-existent file", "[parser]")
{
    TxtParser parser;
    auto result = parser.parse("nonexistent_file_12345.txt");

    REQUIRE(!result.has_value());
    CHECK(result.error() == ParserError::FileNotFound);
}

TEST_CASE("TxtParser: ASCII file", "[parser]")
{
    std::string content = "Hello, world! 123";
    auto result = parse_test_file(content, "test_ascii.txt");

    REQUIRE(result.has_value());
    CHECK(result->content == content);
}

TEST_CASE("TxtParser: BOM removal", "[parser]")
{
    std::string content = "\xEF\xBB\xBFПривет";
    auto result = parse_test_file(content, "test_bom.txt");

    REQUIRE(result.has_value());
    CHECK(result->content == "Привет");
}