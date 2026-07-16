#include <catch2/catch_test_macros.hpp>
#include "chunker/chunker.hpp"

using namespace lse;

TEST_CASE("Chunker: single paragraph within optimal size", "[chunker]")
{
    Chunker chunker;
    auto chunks = chunker.chunkify("Это тестовый параграф из нескольких слов.");

    REQUIRE(chunks.size() == 1);
    CHECK(chunks[0] == "Это тестовый параграф из нескольких слов.");
}

TEST_CASE("Chunker: short paragraphs get merged", "[chunker]")
{
    Chunker chunker({300, 1800, 3500});

    std::string text = "Короткий.\nЕщё короткий.\nТретий короткий.";
    auto chunks = chunker.chunkify(text);

    // Три коротких параграфа должны склеиться в один
    REQUIRE(chunks.size() == 1);
    CHECK(chunks[0].find("Короткий.") != std::string::npos);
    CHECK(chunks[0].find("Ещё короткий.") != std::string::npos);
}

TEST_CASE("Chunker: long paragraph split by sentences", "[chunker]")
{
    Chunker chunker({300, 500, 1000});

    // Создаём длинный текст из множества предложений
    std::string sentence = "Это предложение для тестирования разбиения длинного текста на части по границам предложений.";
    std::string text;
    for (int i = 0; i < 20; ++i)
    {
        text += sentence + " ";
    }

    auto chunks = chunker.chunkify(text);

    // Должен разбить на несколько чанков
    REQUIRE(chunks.size() > 1);

    // Каждый чанк не должен превышать max_chunk_size
    for (const auto &chunk : chunks)
    {
        CHECK(static_cast<int>(chunk.size()) <= 1000);
    }
}

TEST_CASE("Chunker: triple dots normalization", "[chunker]")
{
    Chunker chunker;
    auto chunks = chunker.chunkify("Текст... с троеточием.");

    REQUIRE(chunks.size() == 1);
    CHECK(chunks[0].find("…") != std::string::npos);
}

TEST_CASE("Chunker: empty text", "[chunker]")
{
    Chunker chunker;
    auto chunks = chunker.chunkify("");

    CHECK(chunks.empty());
}

TEST_CASE("Chunker: text with only newlines", "[chunker]")
{
    Chunker chunker;
    auto chunks = chunker.chunkify("\n\n\n");

    CHECK(chunks.empty());
}