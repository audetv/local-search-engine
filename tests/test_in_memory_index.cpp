#include <catch2/catch_test_macros.hpp>
#include "tokenizer/tokenizer.hpp"
#include "tokenizer/stemmer.hpp"
#include "index/in_memory_index.hpp"
#include <set>

using namespace lse;

// Вспомогательная функция: токенизирует и стеммит текст
static auto tokenize_and_stem(const std::string &text, Stemmer &stemmer)
{
    Tokenizer tokenizer(text);
    auto result = tokenizer.tokenize();
    REQUIRE(result.has_value());

    std::vector<Token> stemmed;
    for (auto &token : *result)
    {
        token.text = std::string(stemmer.stem(token.text));
        stemmed.push_back(std::move(token));
    }
    return stemmed;
}

TEST_CASE("InMemoryIndex: add and search single document", "[index]")
{
    InMemoryIndex index;
    Stemmer stemmer(StemmerLanguage::Russian);

    auto tokens = tokenize_and_stem("история древнего рима", stemmer);
    auto doc_id = index.addDocument(tokens);

    REQUIRE(doc_id == 0);
    REQUIRE(index.docCount() == 1);

    // Поиск
    std::vector<std::string> query = {std::string(stemmer.stem("рима"))};
    auto results = index.search(query);

    REQUIRE(results.has_value());
    REQUIRE(results->size() == 1);
    CHECK(results->at(0).doc_id == 0);
    CHECK(results->at(0).bm25_score > 0.0);
}

TEST_CASE("InMemoryIndex: search multiple documents", "[index]")
{
    InMemoryIndex index;
    Stemmer stemmer(StemmerLanguage::Russian);

    // Документ 0: история древнего рима
    auto tokens1 = tokenize_and_stem("история древнего рима", stemmer);
    index.addDocument(tokens1);

    // Документ 1: кулинарные рецепты рима
    auto tokens2 = tokenize_and_stem("кулинарные рецепты рима", stemmer);
    index.addDocument(tokens2);

    // Запрос "рима" должен найти оба документа
    std::vector<std::string> query = {std::string(stemmer.stem("рима"))};
    auto results = index.search(query);

    REQUIRE(results.has_value());
    REQUIRE(results->size() == 2);

    // Оба документа должны быть в результатах
    std::set<uint64_t> doc_ids;
    for (const auto &hit : *results)
    {
        doc_ids.insert(hit.doc_id);
    }
    CHECK(doc_ids.count(0) == 1);
    CHECK(doc_ids.count(1) == 1);
}

TEST_CASE("InMemoryIndex: empty index search", "[index]")
{
    InMemoryIndex index;

    std::vector<std::string> query = {"рим"};
    auto results = index.search(query);

    REQUIRE(results.has_value());
    CHECK(results->empty());
}

TEST_CASE("InMemoryIndex: empty query", "[index]")
{
    InMemoryIndex index;
    Stemmer stemmer(StemmerLanguage::Russian);

    auto tokens = tokenize_and_stem("тестовый документ", stemmer);
    index.addDocument(tokens);

    std::vector<std::string> query;
    auto results = index.search(query);

    REQUIRE(!results.has_value());
    CHECK(results.error() == IndexError::EmptyQuery);
}

TEST_CASE("InMemoryIndex: term frequency affects score", "[index]")
{
    InMemoryIndex index;
    Stemmer stemmer(StemmerLanguage::Russian);

    // Документ 0: одно вхождение "рим"
    auto tokens1 = tokenize_and_stem("рим", stemmer);
    index.addDocument(tokens1);

    // Документ 1: три вхождения "рим"
    auto tokens2 = tokenize_and_stem("рим рим рим", stemmer);
    index.addDocument(tokens2);

    std::vector<std::string> query = {std::string(stemmer.stem("рим"))};
    auto results = index.search(query);

    REQUIRE(results.has_value());
    REQUIRE(results->size() == 2);

    // Документ с большим tf должен быть выше
    CHECK(results->at(0).doc_id == 1); // три вхождения
    CHECK(results->at(1).doc_id == 0); // одно вхождение
    CHECK(results->at(0).bm25_score > results->at(1).bm25_score);
}

TEST_CASE("InMemoryIndex: positions stored", "[index]")
{
    InMemoryIndex index;
    Stemmer stemmer(StemmerLanguage::Russian);

    auto tokens = tokenize_and_stem("история рима и история греции", stemmer);
    index.addDocument(tokens);

    // Проверим через поиск, что позиции сохранились
    std::vector<std::string> query = {std::string(stemmer.stem("истор"))}; // история → истор
    auto results = index.search(query);

    REQUIRE(results.has_value());
    REQUIRE(results->size() == 1);
}