#include <catch2/catch_test_macros.hpp>
#include "parser/fb2_parser.hpp"

using namespace lse;

TEST_CASE("Fb2Parser: simple fb2 document", "[parser]")
{
    std::string fb2 = R"(<?xml version="1.0" encoding="UTF-8"?>
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0">
  <description>
    <title-info>
      <book-title>Тестовая книга</book-title>
      <author>
        <first-name>Иван</first-name>
        <last-name>Петров</last-name>
      </author>
    </title-info>
  </description>
  <body>
    <section>
      <p>Первый параграф текста.</p>
      <p>Второй параграф.</p>
    </section>
  </body>
</FictionBook>)";

    Fb2Parser parser;
    auto result = parser.parseFromMemory(fb2, "test.fb2");

    REQUIRE(result.has_value());
    CHECK(result->title == "Тестовая книга");
    CHECK(result->content == "Первый параграф текста.\nВторой параграф.");
    CHECK(result->encoding == "UTF-8");
}

TEST_CASE("Fb2Parser: nested sections", "[parser]")
{
    std::string fb2 = R"(<?xml version="1.0" encoding="UTF-8"?>
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0">
  <description>
    <title-info>
      <book-title>Глубокая структура</book-title>
    </title-info>
  </description>
  <body>
    <section>
      <p>Уровень 1</p>
      <section>
        <p>Уровень 2</p>
      </section>
    </section>
  </body>
</FictionBook>)";

    Fb2Parser parser;
    auto result = parser.parseFromMemory(fb2, "nested.fb2");

    REQUIRE(result.has_value());
    CHECK(result->content == "Уровень 1\nУровень 2");
}

TEST_CASE("Fb2Parser: empty body", "[parser]")
{
    std::string fb2 = R"(<?xml version="1.0" encoding="UTF-8"?>
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0">
  <description>
    <title-info>
      <book-title>Пустая книга</book-title>
    </title-info>
  </description>
  <body>
  </body>
</FictionBook>)";

    Fb2Parser parser;
    auto result = parser.parseFromMemory(fb2, "empty.fb2");

    REQUIRE(!result.has_value());
    CHECK(result.error() == ParserError::EmptyContent);
}

TEST_CASE("Fb2Parser: missing title", "[parser]")
{
    std::string fb2 = R"(<?xml version="1.0" encoding="UTF-8"?>
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0">
  <description>
    <title-info>
    </title-info>
  </description>
  <body>
    <p>Текст без названия.</p>
  </body>
</FictionBook>)";

    Fb2Parser parser;
    auto result = parser.parseFromMemory(fb2, "notitle.fb2");

    REQUIRE(result.has_value());
    CHECK(result->title == "notitle"); // fallback to filename
    CHECK(result->content == "Текст без названия.");
}

TEST_CASE("Fb2Parser: multiple authors", "[parser]")
{
    std::string fb2 = R"(<?xml version="1.0" encoding="UTF-8"?>
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0">
  <description>
    <title-info>
      <book-title>Соавторы</book-title>
      <author>
        <first-name>Иван</first-name>
        <last-name>Петров</last-name>
      </author>
      <author>
        <first-name>Мария</first-name>
        <last-name>Сидорова</last-name>
      </author>
    </title-info>
  </description>
  <body>
    <p>Текст книги.</p>
  </body>
</FictionBook>)";

    Fb2Parser parser;
    auto result = parser.parseFromMemory(fb2, "coauthors.fb2");

    REQUIRE(result.has_value());
    CHECK(result->title == "Соавторы");
    CHECK(result->content == "Текст книги.");
}