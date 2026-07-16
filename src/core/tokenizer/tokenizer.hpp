#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <expected>
#include <cstdint>

namespace lse
{

    struct Token
    {
        std::string text; // нормализованное слово (lowercase)
        size_t position;  // позиция в исходном тексте (номер токена, для proximity)
    };

    enum class TokenizerError
    {
        InvalidUtf8,
        EmptyText
    };

    class Tokenizer
    {
    public:
        struct Options
        {
            bool normalize_yo = true; // ё → е
            bool keep_digits = true;  // сохранять ли числа
        };

        explicit Tokenizer(std::string_view text, Options opts = {});

        // Основной метод: токенизировать весь текст
        auto tokenize() -> std::expected<std::vector<Token>, TokenizerError>;

    private:
        std::string_view text_;
        Options opts_;

        // UTF-8 декодирование
        static auto decode_utf8(const char *&it, const char *end)
            -> std::expected<char32_t, TokenizerError>;

        // Является ли кодпоинт буквой (кириллица, латиница, включая расширенные)
        static bool is_letter(char32_t cp);

        // Является ли кодпоинт цифрой (0-9)
        static bool is_digit(char32_t cp);

        // Нижний регистр с поддержкой кириллицы
        static char32_t to_lower(char32_t cp);

        // Нормализация: ё → е
        static char32_t normalize(char32_t cp);
    };

} // namespace lse