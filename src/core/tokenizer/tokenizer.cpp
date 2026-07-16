#include "tokenizer.hpp"
#include <algorithm>
#include <stdexcept>

namespace lse
{

    Tokenizer::Tokenizer(std::string_view text, TokenizerOptions opts)
        : normalized_text_(normalize_text(text)), text_(normalized_text_), opts_(opts)
    {
    }

    std::string Tokenizer::normalize_text(std::string_view text)
    {
        std::string result;
        result.reserve(text.size());

        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == '\r')
            {
                if (i + 1 < text.size() && text[i + 1] == '\n')
                {
                    result += '\n';
                    ++i; // пропускаем следующий \n
                }
                else
                {
                    result += '\n'; // одиночный \r → \n
                }
            }
            else
            {
                result += text[i];
            }
        }

        return result;
    }

    auto Tokenizer::tokenize() -> std::expected<std::vector<Token>, TokenizerError>
    {
        if (text_.empty())
        {
            return std::unexpected(TokenizerError::EmptyText);
        }

        std::vector<Token> tokens;
        std::string current_word;
        size_t position = 0;

        const char *it = text_.data();
        const char *end = it + text_.size();

        while (it < end)
        {
            // Пропускаем пробелы и разделители (\r уже нормализован в \n)
            if (*it == ' ' || *it == '\n' || *it == '\t')
            {
                if (!current_word.empty())
                {
                    tokens.push_back(Token{std::move(current_word), position++});
                    current_word.clear();
                }
                ++it;
                continue;
            }

            // Дефис — разбивает слово на два токена
            if (*it == '-')
            {
                if (!current_word.empty())
                {
                    tokens.push_back(Token{std::move(current_word), position++});
                    current_word.clear();
                }
                ++it;
                continue;
            }

            // Апостроф — разделитель (как пробел)
            if (*it == '\'')
            {
                if (!current_word.empty())
                {
                    tokens.push_back(Token{std::move(current_word), position++});
                    current_word.clear();
                }
                ++it;
                continue;
            }

            // UTF-8 декодирование
            auto cp_result = decode_utf8(it, end);
            if (!cp_result.has_value())
            {
                return std::unexpected(cp_result.error());
            }

            char32_t cp = *cp_result;

            // Буква — добавляем к слову
            if (is_letter(cp))
            {
                cp = normalize(cp);
                cp = to_lower(cp);
                encode_utf8(cp, current_word);
            }
            // Цифра — добавляем если разрешено
            else if (is_digit(cp) && opts_.keep_digits)
            {
                encode_utf8(cp, current_word);
            }
            // Всё остальное — разделитель
            else
            {
                if (!current_word.empty())
                {
                    tokens.push_back(Token{std::move(current_word), position++});
                    current_word.clear();
                }
            }
        }

        // Последнее слово
        if (!current_word.empty())
        {
            tokens.push_back(Token{std::move(current_word), position++});
        }

        return tokens;
    }

    auto Tokenizer::decode_utf8(const char *&it, const char *end)
        -> std::expected<char32_t, TokenizerError>
    {

        if (it >= end)
        {
            return std::unexpected(TokenizerError::InvalidUtf8);
        }

        unsigned char c = static_cast<unsigned char>(*it);
        char32_t cp;
        int len;

        if ((c & 0x80) == 0)
        {
            // 1-byte ASCII
            cp = c;
            len = 1;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            // 2-byte
            cp = c & 0x1F;
            len = 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            // 3-byte
            cp = c & 0x0F;
            len = 3;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            // 4-byte
            cp = c & 0x07;
            len = 4;
        }
        else
        {
            return std::unexpected(TokenizerError::InvalidUtf8);
        }

        if (it + len > end)
        {
            return std::unexpected(TokenizerError::InvalidUtf8);
        }

        for (int i = 1; i < len; ++i)
        {
            unsigned char byte = static_cast<unsigned char>(*(it + i));
            if ((byte & 0xC0) != 0x80)
            {
                return std::unexpected(TokenizerError::InvalidUtf8);
            }
            cp = (cp << 6) | (byte & 0x3F);
        }

        it += len;
        return cp;
    }

    void Tokenizer::encode_utf8(char32_t cp, std::string &out)
    {
        if (cp < 0x80)
        {
            out += static_cast<char>(cp);
        }
        else if (cp < 0x800)
        {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else
        {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    bool Tokenizer::is_letter(char32_t cp)
    {
        // Кириллица
        if (cp >= 0x0400 && cp <= 0x04FF)
            return true;
        // Латиница базовая
        if (cp >= 0x0041 && cp <= 0x005A)
            return true; // A-Z
        if (cp >= 0x0061 && cp <= 0x007A)
            return true; // a-z
        // Латиница расширенная (буквы с диакритикой)
        if (cp >= 0x00C0 && cp <= 0x024F)
            return true;
        // Греческий (для научных текстов)
        if (cp >= 0x0370 && cp <= 0x03FF)
            return true;

        return false;
    }

    bool Tokenizer::is_digit(char32_t cp)
    {
        return cp >= 0x0030 && cp <= 0x0039; // 0-9
    }

    char32_t Tokenizer::to_lower(char32_t cp)
    {
        // Кириллица верхний регистр
        if (cp >= 0x0410 && cp <= 0x042F)
        {
            return cp + 0x20; // А-Я → а-я
        }
        if (cp == 0x0401)
            return 0x0451; // Ё → ё

        // Латиница верхний регистр
        if (cp >= 0x0041 && cp <= 0x005A)
        {
            return cp + 0x20; // A-Z → a-z
        }

        // Греческий верхний регистр
        if (cp >= 0x0391 && cp <= 0x03A9)
        {
            return cp + 0x20;
        }

        return cp; // уже нижний или не буква
    }

    char32_t Tokenizer::normalize(char32_t cp) const
    {
        if (opts_.normalize_yo)
        {
            if (cp == 0x0451)
                return 0x0435; // ё → е
            if (cp == 0x0401)
                return 0x0415; // Ё → Е
        }
        return cp;
    }

} // namespace lse