#include <catch2/catch_test_macros.hpp>
#include "parser/filename_parser.hpp"

using namespace lse;

TEST_CASE("FilenameParser: standard pattern", "[filename]")
{
    auto meta = parseFilenameMetadata("История_Сергеев А. — Древний Рим.fb2");

    CHECK(meta.genre == "История");
    CHECK(meta.author == "Сергеев А.");
    CHECK(meta.title == "Древний Рим");
}

TEST_CASE("FilenameParser: no author", "[filename]")
{
    auto meta = parseFilenameMetadata("Фантастика_ — Звёздный путь.txt");

    CHECK(meta.genre == "Фантастика");
    CHECK(meta.author == "");
    CHECK(meta.title == "Звёздный путь");
}

TEST_CASE("FilenameParser: no genre, has author with underscore prefix", "[filename]")
{
    auto meta = parseFilenameMetadata("_Блиш Джеймс — Звёздный путь.txt");

    CHECK(meta.genre == "");
    CHECK(meta.author == "Блиш Джеймс");
    CHECK(meta.title == "Звёздный путь");
}

TEST_CASE("FilenameParser: no genre, has author without underscore", "[filename]")
{
    auto meta = parseFilenameMetadata("Блиш Джеймс — Звёздный путь.txt");

    CHECK(meta.genre == "");
    CHECK(meta.author == "Блиш Джеймс");
    CHECK(meta.title == "Звёздный путь");
}

TEST_CASE("FilenameParser: no pattern match", "[filename]")
{
    auto meta = parseFilenameMetadata("Простая книга.docx");

    CHECK(meta.genre == "");
    CHECK(meta.author == "");
    CHECK(meta.title == "Простая книга");
}

TEST_CASE("FilenameParser: multiple underscores in author", "[filename]")
{
    auto meta = parseFilenameMetadata("История_Иванов И. И. — История Рима.fb2");

    CHECK(meta.genre == "История");
    CHECK(meta.author == "Иванов И. И.");
    CHECK(meta.title == "История Рима");
}

TEST_CASE("FilenameParser: extension only", "[filename]")
{
    auto meta = parseFilenameMetadata(".fb2");

    CHECK(meta.title == ""); // базовое имя пустое
}