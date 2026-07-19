#include <catch2/catch_test_macros.hpp>
#include "index/index_writer.hpp"
#include "index/index_reader.hpp"
#include "search/search_engine.hpp"
#include "search/query_parser.hpp"
#include "tokenizer/tokenizer.hpp"
#include "tokenizer/stemmer.hpp"
#include <filesystem>
#include <memory>

using namespace lse;
namespace fs = std::filesystem;

static auto tokenize_and_stem(const std::string &text, Stemmer &stemmer) -> std::vector<Token>
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

// Вспомогательная функция: создать индекс с документами
struct TestIndex
{
    fs::path dir;
    TestIndex() : dir(fs::temp_directory_path() / "test_se")
    {
        fs::remove_all(dir);
    }
    ~TestIndex() { fs::remove_all(dir); }
};

TEST_CASE("SearchEngine: single term", "[search_engine]")
{
    TestIndex ti;
    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        writer.open(ti.dir);
        writer.upsertBook(1, "Книга", "Автор", "История", "ru", "/tmp/book.fb2");
        auto tokens = tokenize_and_stem("история древнего рима", stemmer);
        writer.addDocument(1, 1, tokens, "история древнего рима");
        writer.close();
    }

    IndexReader reader;
    reader.open(ti.dir);
    SearchEngine engine(reader, stemmer);

    QueryParser parser;
    auto query = parser.parse("рима");
    REQUIRE(query.has_value());

    auto results = engine.execute(*query);
    REQUIRE(results.has_value());
    REQUIRE(results->size() == 1);
    CHECK(results->at(0)->title == "Книга");
}

TEST_CASE("SearchEngine: AND two terms", "[search_engine]")
{
    TestIndex ti;
    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        writer.open(ti.dir);
        writer.upsertBook(1, "Книга 1", "Автор", "Жанр", "ru", "/tmp/book1.fb2");
        writer.upsertBook(2, "Книга 2", "Автор", "Жанр", "ru", "/tmp/book2.fb2");

        auto tokens1 = tokenize_and_stem("история рима", stemmer);
        writer.addDocument(1, 1, tokens1, "история рима");

        auto tokens2 = tokenize_and_stem("история греции", stemmer);
        writer.addDocument(2, 1, tokens2, "история греции");
        writer.close();
    }

    IndexReader reader;
    reader.open(ti.dir);
    SearchEngine engine(reader, stemmer);

    QueryParser parser;
    // "история рима" → AND (должен найти только книгу 1)
    auto query = parser.parse("история рима");
    REQUIRE(query.has_value());

    auto results = engine.execute(*query);
    REQUIRE(results.has_value());
    REQUIRE(results->size() == 1);
    CHECK(results->at(0)->title == "Книга 1");
}

TEST_CASE("SearchEngine: OR operator", "[search_engine]")
{
    TestIndex ti;
    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        writer.open(ti.dir);
        writer.upsertBook(1, "Книга 1", "Автор", "Жанр", "ru", "/tmp/book1.fb2");
        writer.upsertBook(2, "Книга 2", "Автор", "Жанр", "ru", "/tmp/book2.fb2");

        auto tokens1 = tokenize_and_stem("рим", stemmer);
        writer.addDocument(1, 1, tokens1, "рим");

        auto tokens2 = tokenize_and_stem("греция", stemmer);
        writer.addDocument(2, 1, tokens2, "греция");
        writer.close();
    }

    IndexReader reader;
    reader.open(ti.dir);
    SearchEngine engine(reader, stemmer);

    QueryParser parser;
    auto query = parser.parse("рим | греция");
    REQUIRE(query.has_value());

    auto results = engine.execute(*query);
    REQUIRE(results.has_value());
    REQUIRE(results->size() == 2);
}

TEST_CASE("SearchEngine: NOT operator", "[search_engine]")
{
    TestIndex ti;
    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        writer.open(ti.dir);
        writer.upsertBook(1, "Книга 1", "Автор", "Жанр", "ru", "/tmp/book1.fb2");
        writer.upsertBook(2, "Книга 2", "Автор", "Жанр", "ru", "/tmp/book2.fb2");

        auto tokens1 = tokenize_and_stem("история рима", stemmer);
        writer.addDocument(1, 1, tokens1, "история рима");

        auto tokens2 = tokenize_and_stem("история греции", stemmer);
        writer.addDocument(2, 1, tokens2, "история греции");
        writer.close();
    }

    IndexReader reader;
    reader.open(ti.dir);
    SearchEngine engine(reader, stemmer);

    QueryParser parser;
    auto query = parser.parse("история -рима");
    REQUIRE(query.has_value());

    auto results = engine.execute(*query);
    REQUIRE(results.has_value());
    REQUIRE(results->size() == 1);
    CHECK(results->at(0)->title == "Книга 2");
}

TEST_CASE("SearchEngine: phrase", "[search_engine]")
{
    TestIndex ti;
    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        writer.open(ti.dir);
        writer.upsertBook(1, "Книга 1", "Автор", "Жанр", "ru", "/tmp/book1.fb2");
        writer.upsertBook(2, "Книга 2", "Автор", "Жанр", "ru", "/tmp/book2.fb2");

        auto tokens1 = tokenize_and_stem("история древнего рима", stemmer);
        writer.addDocument(1, 1, tokens1, "история древнего рима");

        auto tokens2 = tokenize_and_stem("древнего рима история", stemmer);
        writer.addDocument(2, 1, tokens2, "древнего рима история");
        writer.close();
    }

    IndexReader reader;
    reader.open(ti.dir);
    SearchEngine engine(reader, stemmer);

    QueryParser parser;
    // "древнего рима" — фраза
    auto query = parser.parse("\"древнего рима\"");
    REQUIRE(query.has_value());

    auto results = engine.execute(*query);
    REQUIRE(results.has_value());
    REQUIRE(results->size() == 2); // обе книги содержат фразу
}

TEST_CASE("SearchEngine: grouping with parentheses", "[search_engine]")
{
    TestIndex ti;
    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        writer.open(ti.dir);
        writer.upsertBook(1, "Кофе", "Автор", "Жанр", "ru", "/tmp/book1.fb2");
        writer.upsertBook(2, "Чай", "Автор", "Жанр", "ru", "/tmp/book2.fb2");
        writer.upsertBook(3, "Круассан", "Автор", "Жанр", "ru", "/tmp/book3.fb2");

        auto tokens1 = tokenize_and_stem("кофе круассан", stemmer);
        writer.addDocument(1, 1, tokens1, "кофе круассан");

        auto tokens2 = tokenize_and_stem("чай круассан", stemmer);
        writer.addDocument(2, 1, tokens2, "чай круассан");

        auto tokens3 = tokenize_and_stem("кофе", stemmer);
        writer.addDocument(3, 1, tokens3, "кофе");
        writer.close();
    }

    IndexReader reader;
    reader.open(ti.dir);
    SearchEngine engine(reader, stemmer);

    QueryParser parser;
    // (кофе | чай) круассан — должен найти книги 1 и 2, но не 3
    auto query = parser.parse("(кофе | чай) круассан");
    REQUIRE(query.has_value());

    auto results = engine.execute(*query);
    REQUIRE(results.has_value());
    REQUIRE(results->size() == 2);
}