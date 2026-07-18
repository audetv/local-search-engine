#pragma once

#include <string>
#include <expected>
#include <filesystem>
#include <vector>

namespace lse
{

    struct ParsedDocument
    {
        std::string content;   // текст в UTF-8
        std::string title;     // название (из имени файла)
        std::string author;    // автор
        std::string genre;     // жанр
        std::string encoding;  // определённая кодировка
        std::string file_path; // путь к файлу
        size_t file_size = 0;
    };

    enum class ParserError
    {
        FileNotFound,
        ReadError,
        EncodingDetectionFailed,
        ConversionFailed,
        EmptyContent
    };

    class TxtParser
    {
    public:
        // Парсит TXT-файл. content будет в UTF-8.
        auto parse(const std::filesystem::path &file_path)
            -> std::expected<ParsedDocument, ParserError>;

        // Парсит TXT из памяти (для тестов)
        auto parseFromMemory(const std::vector<uint8_t> &data,
                             const std::string &file_name = "memory.txt")
            -> std::expected<ParsedDocument, ParserError>;

    private:
        // Определить кодировку по массиву байтов
        auto detectEncoding(const std::vector<uint8_t> &data)
            -> std::expected<std::string, ParserError>;

        // Перекодировать из detected_encoding в UTF-8
        auto convertToUtf8(const std::vector<uint8_t> &data,
                           const std::string &encoding)
            -> std::expected<std::string, ParserError>;

        // Перекодировка CP1251 → UTF-8 (ручная таблица)
        static std::string convertCp1251ToUtf8(const std::vector<uint8_t> &data);

        // Перекодировка CP866 → UTF-8
        static std::string convertCp866ToUtf8(const std::vector<uint8_t> &data);

        // Перекодировка KOI8-R → UTF-8
        static std::string convertKoi8rToUtf8(const std::vector<uint8_t> &data);
    };

} // namespace lse