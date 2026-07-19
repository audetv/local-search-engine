#include <catch2/catch_test_macros.hpp>
#include "index/index_writer.hpp"
#include "index/index_reader.hpp"
#include "search/search_engine.hpp"
#include "search/query_builder.hpp"
#include "tokenizer/tokenizer.hpp"
#include "tokenizer/stemmer.hpp"
#include <filesystem>
#include <memory>
#include <algorithm>

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
        CHECK(*doc_id == 1);

        writer.close();
    }

    {
        IndexReader reader;
        REQUIRE(reader.open(index_dir).has_value());
        CHECK(reader.docCount() == 1);

        SearchEngine engine(reader, stemmer);
        auto query = QueryBuilder::queryString("рима");
        REQUIRE(query.has_value());

        auto results = engine.execute(*query, 10);
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

        SearchEngine engine(reader, stemmer);
        auto query = QueryBuilder::queryString("история");
        REQUIRE(query.has_value());

        auto results = engine.execute(*query, 10);
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

        SearchEngine engine(reader, stemmer);
        auto query = QueryBuilder::queryString("документ");
        REQUIRE(query.has_value());

        auto results = engine.execute(*query, 10);
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

        Stemmer stemmer(StemmerLanguage::Russian);
        SearchEngine engine(reader, stemmer);
        auto query = QueryBuilder::queryString("тест");
        REQUIRE(query.has_value());

        auto results = engine.execute(*query, 10);
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
        SearchEngine engine(reader, stemmer);

        auto query = QueryBuilder::queryString("древний");
        REQUIRE(query.has_value());

        auto results = engine.execute(*query, 10);
        REQUIRE(results.has_value());
        CHECK(results->size() == 1);

        auto results_hist = engine.execute(*query, 10, "История");
        REQUIRE(results_hist.has_value());
        CHECK(results_hist->size() == 1);

        auto results_sci = engine.execute(*query, 10, "Наука");
        REQUIRE(results_sci.has_value());
        CHECK(results_sci->empty());
    }

    fs::remove_all(index_dir);
}

TEST_CASE("IndexReader: search with highlight terms", "[persistence]")
{
    auto index_dir = fs::temp_directory_path() / "test_index_highlight";
    fs::remove_all(index_dir);

    Stemmer stemmer(StemmerLanguage::Russian);

    {
        IndexWriter writer;
        writer.open(index_dir);
        writer.upsertBook(1, "Книга про древний Рим", "Автор", "История", "ru", "/tmp/book.fb2");
        auto tokens = tokenize_and_stem("история древнего рима", stemmer);
        writer.addDocument(1, 1, tokens, "история древнего рима");
        writer.close();
    }

    {
        IndexReader reader;
        reader.open(index_dir);
        SearchEngine engine(reader, stemmer);

        auto query = QueryBuilder::queryString("рима");
        REQUIRE(query.has_value());

        std::vector<std::string> highlight_terms = {"рима"};

        auto results = engine.execute(*query, 10, "", "", "", highlight_terms);
        REQUIRE(results.has_value());
        REQUIRE(results->size() == 1);

        auto &hit = results->at(0);
        CHECK(hit->highlights.size() > 0);

        bool found = false;
        for (const auto &hl : hit->highlights)
        {
            std::string highlighted = hit->content.substr(hl.offset, hl.length);
            std::transform(highlighted.begin(), highlighted.end(), highlighted.begin(), ::tolower);
            if (highlighted == "рима")
            {
                found = true;
                break;
            }
        }
        CHECK(found);

        reader.close();
    }

    fs::remove_all(index_dir);
}