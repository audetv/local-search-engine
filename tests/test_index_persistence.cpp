#include <catch2/catch_test_macros.hpp>
#include "index/index_writer.hpp"
#include "index/index_reader.hpp"
#include "tokenizer/tokenizer.hpp"
#include "tokenizer/stemmer.hpp"
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>

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

TEST_CASE("IndexWriter/IndexReader: write and read single document", "[persistence]")
{
    auto index_dir = fs::temp_directory_path() / "test_index_1";
    fs::remove_all(index_dir);

    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        REQUIRE(writer.open(index_dir).has_value());

        int64_t book_id = 42;
        writer.upsertBook(book_id, "Тестовая книга", "Автор", "История", "ru", "/tmp/test.fb2");

        auto tokens = tokenize_and_stem("история древнего рима", stemmer);
        auto doc_id = writer.addDocument(book_id, 1, tokens, "история древнего рима");
        REQUIRE(doc_id.has_value());
        CHECK(*doc_id == 0);

        writer.close();
    }

    {
        IndexReader reader;
        REQUIRE(reader.open(index_dir).has_value());
        CHECK(reader.docCount() == 1);

        Stemmer query_stemmer(StemmerLanguage::Russian);
        auto query_tokens = tokenize_and_stem("рима", query_stemmer);
        std::vector<std::string> query_terms;
        for (const auto &t : query_tokens)
        {
            query_terms.push_back(t.text);
        }

        auto results = reader.search(query_terms, 10);
        REQUIRE(results.has_value());
        REQUIRE(results->size() == 1);
        CHECK(results->at(0)->title == "Тестовая книга");
        CHECK(results->at(0)->author == "Автор");
        CHECK(results->at(0)->bm25_score > 0.0f);

        reader.close();
    }

    fs::remove_all(index_dir);
}

TEST_CASE("IndexWriter/IndexReader: multiple documents", "[persistence]")
{
    auto index_dir = fs::temp_directory_path() / "test_index_2";
    fs::remove_all(index_dir);

    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        REQUIRE(writer.open(index_dir).has_value());

        writer.upsertBook(1, "Книга 1", "Автор 1", "Жанр", "ru", "/tmp/book1.fb2");
        writer.upsertBook(2, "Книга 2", "Автор 2", "Жанр", "ru", "/tmp/book2.fb2");

        auto tokens1 = tokenize_and_stem("римская империя", stemmer);
        writer.addDocument(1, 1, tokens1, "римская империя");

        auto tokens2 = tokenize_and_stem("история греции", stemmer);
        writer.addDocument(2, 1, tokens2, "история греции");

        writer.close();
    }

    {
        IndexReader reader;
        REQUIRE(reader.open(index_dir).has_value());
        CHECK(reader.docCount() == 2);

        auto query_tokens = tokenize_and_stem("история", stemmer);
        std::vector<std::string> query_terms;
        for (const auto &t : query_tokens)
            query_terms.push_back(t.text);

        auto results = reader.search(query_terms, 10);
        REQUIRE(results.has_value());
        REQUIRE(results->size() == 1);
        CHECK(results->at(0)->title == "Книга 2");

        reader.close();
    }

    fs::remove_all(index_dir);
}

TEST_CASE("IndexWriter/IndexReader: persistence survives reopen", "[persistence]")
{
    auto index_dir = fs::temp_directory_path() / "test_index_3";
    fs::remove_all(index_dir);

    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        writer.open(index_dir);
        writer.upsertBook(100, "Книга", "Автор", "Жанр", "ru", "/tmp/book.fb2");
        auto tokens = tokenize_and_stem("тестовый документ", stemmer);
        writer.addDocument(100, 1, tokens, "тестовый документ");
    }

    {
        IndexWriter writer;
        writer.open(index_dir);
        auto tokens = tokenize_and_stem("второй документ", stemmer);
        writer.addDocument(100, 2, tokens, "второй документ");
    }

    {
        IndexReader reader;
        reader.open(index_dir);
        CHECK(reader.docCount() == 2);

        auto query_tokens = tokenize_and_stem("документ", stemmer);
        std::vector<std::string> query_terms;
        for (const auto &t : query_tokens)
            query_terms.push_back(t.text);

        auto results = reader.search(query_terms, 10);
        REQUIRE(results.has_value());
        CHECK(results->size() == 2);
    }

    fs::remove_all(index_dir);
}

TEST_CASE("IndexWriter/IndexReader: empty index", "[persistence]")
{
    auto index_dir = fs::temp_directory_path() / "test_index_empty";
    fs::remove_all(index_dir);

    {
        IndexWriter writer;
        writer.open(index_dir);
        writer.close();
    }

    {
        IndexReader reader;
        REQUIRE(reader.open(index_dir).has_value());
        CHECK(reader.docCount() == 0);

        std::vector<std::string> query = {"тест"};
        auto results = reader.search(query, 10);
        REQUIRE(results.has_value());
        CHECK(results->empty());
    }

    fs::remove_all(index_dir);
}

TEST_CASE("IndexWriter/IndexReader: filter by genre", "[persistence]")
{
    auto index_dir = fs::temp_directory_path() / "test_index_filter";
    fs::remove_all(index_dir);

    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        writer.open(index_dir);

        writer.upsertBook(1, "Книга истории", "Автор", "История", "ru", "/tmp/history.fb2");
        writer.upsertBook(2, "Книга науки", "Автор", "Наука", "ru", "/tmp/science.fb2");

        auto tokens1 = tokenize_and_stem("древний рим", stemmer);
        writer.addDocument(1, 1, tokens1, "древний рим");

        auto tokens2 = tokenize_and_stem("квантовая физика", stemmer);
        writer.addDocument(2, 1, tokens2, "квантовая физика");
    }

    {
        IndexReader reader;
        reader.open(index_dir);

        auto query_tokens = tokenize_and_stem("древний", stemmer);
        std::vector<std::string> query_terms;
        for (const auto &t : query_tokens)
            query_terms.push_back(t.text);

        auto results = reader.search(query_terms, 10);
        REQUIRE(results.has_value());
        CHECK(results->size() == 1);

        auto results_hist = reader.search(query_terms, 10, "История");
        REQUIRE(results_hist.has_value());
        CHECK(results_hist->size() == 1);

        auto results_sci = reader.search(query_terms, 10, "Наука");
        REQUIRE(results_sci.has_value());
        CHECK(results_sci->empty());
    }

    fs::remove_all(index_dir);
}