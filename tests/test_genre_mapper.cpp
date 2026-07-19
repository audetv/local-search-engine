#include <catch2/catch_test_macros.hpp>
#include "parser/genre_mapper.hpp"
#include <filesystem>
#include <fstream>

using namespace lse;
namespace fs = std::filesystem;

TEST_CASE("GenreMapper: load and map", "[genre_mapper]")
{
    auto path = fs::temp_directory_path() / "test_genres.csv";

    {
        std::ofstream f(path);
        f << "Военная история,Военное дело\n";
        f << "Путешествия и география,География\n";
        f << "Геология и география,География\n";
        f << "\"Маркетинг, PR\",Бизнес\n";
    }

    GenreMapper mapper;
    REQUIRE(mapper.load(path));

    CHECK(mapper.map("Военная история") == "Военное дело");
    CHECK(mapper.map("Путешествия и география") == "География");
    CHECK(mapper.map("Геология и география") == "География");
    CHECK(mapper.map("Маркетинг, PR") == "Бизнес");
    CHECK(mapper.map("Неизвестный жанр") == "Неизвестный жанр");
    CHECK(mapper.map("") == "");

    fs::remove(path);
}

TEST_CASE("GenreMapper: empty when no file", "[genre_mapper]")
{
    GenreMapper mapper;
    CHECK(mapper.empty());
    CHECK(mapper.map("История") == "История");
}