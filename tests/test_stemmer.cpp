#include <catch2/catch_test_macros.hpp>
#include "tokenizer/stemmer.hpp"

using namespace lse;

TEST_CASE("Stemmer: Russian stemming", "[stemmer]")
{
    Stemmer stemmer(StemmerLanguage::Russian);

    // Стеммер Портера для русского языка
    CHECK(stemmer.stem("история") == "истор");
    CHECK(stemmer.stem("древнего") == "древн");
    CHECK(stemmer.stem("рима") == "рим");
    CHECK(stemmer.stem("рим") == "рим");
    CHECK(stemmer.stem("книга") == "книг");
    CHECK(stemmer.stem("книги") == "книг");
    CHECK(stemmer.stem("книге") == "книг");
}

TEST_CASE("Stemmer: same stem for different forms", "[stemmer]")
{
    Stemmer stemmer(StemmerLanguage::Russian);

    // Разные формы одного слова должны давать одинаковую основу
    auto stem1 = stemmer.stem("мальчик");
    auto stem2 = stemmer.stem("мальчика");
    auto stem3 = stemmer.stem("мальчику");
    auto stem4 = stemmer.stem("мальчиком");

    CHECK(stem1 == stem2);
    CHECK(stem2 == stem3);
    CHECK(stem3 == stem4);
}

TEST_CASE("Stemmer: English stemming", "[stemmer]")
{
    Stemmer stemmer(StemmerLanguage::English);

    CHECK(stemmer.stem("running") == "run");
    CHECK(stemmer.stem("runs") == "run");
    CHECK(stemmer.stem("easily") == "easili");
    CHECK(stemmer.stem("books") == "book");
}

TEST_CASE("Stemmer: empty word", "[stemmer]")
{
    Stemmer stemmer(StemmerLanguage::Russian);

    auto result = stemmer.stem("");
    CHECK(result.empty());
}

TEST_CASE("Stemmer: non-existent word", "[stemmer]")
{
    Stemmer stemmer(StemmerLanguage::Russian);

    // Несуществующее слово — стеммер всё равно обработает
    auto result = stemmer.stem("ывапаывап");
    CHECK(!result.empty());
}

TEST_CASE("Stemmer: move semantics", "[stemmer]")
{
    Stemmer stemmer1(StemmerLanguage::Russian);
    auto result1 = stemmer1.stem("история");

    // Перемещаем стеммер
    Stemmer stemmer2 = std::move(stemmer1);
    auto result2 = stemmer2.stem("история");

    CHECK(result1 == result2);
}

TEST_CASE("Stemmer: language property", "[stemmer]")
{
    Stemmer russian(StemmerLanguage::Russian);
    Stemmer english(StemmerLanguage::English);

    CHECK(russian.language() == StemmerLanguage::Russian);
    CHECK(english.language() == StemmerLanguage::English);
}