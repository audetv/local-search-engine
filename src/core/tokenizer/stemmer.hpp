#pragma once

#include <string>
#include <string_view>
#include <memory>

// Forward declaration для C-структуры libstemmer
struct sb_stemmer;

namespace lse
{

    enum class StemmerLanguage
    {
        Russian,
        English
        // Добавим другие языки позже при необходимости
    };

    class Stemmer
    {
    public:
        explicit Stemmer(StemmerLanguage lang);
        ~Stemmer();

        // Некопируемый (владеет C-ресурсом)
        Stemmer(const Stemmer &) = delete;
        Stemmer &operator=(const Stemmer &) = delete;

        // Перемещаемый
        Stemmer(Stemmer &&other) noexcept;
        Stemmer &operator=(Stemmer &&other) noexcept;

        // Стемминг одного слова. Возвращает основу.
        // Результат действителен до следующего вызова stem().
        auto stem(std::string_view word) -> std::string_view;

        // Язык стеммера
        StemmerLanguage language() const { return lang_; }

    private:
        sb_stemmer *stemmer_ = nullptr;
        StemmerLanguage lang_;
    };

} // namespace lse