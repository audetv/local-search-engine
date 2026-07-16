#include "txt_parser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

// uchardet
#include <uchardet/uchardet.h>

namespace lse
{

    auto TxtParser::parse(const std::filesystem::path &file_path)
        -> std::expected<ParsedDocument, ParserError>
    {

        // Проверяем существование файла
        std::error_code ec;
        if (!std::filesystem::exists(file_path, ec))
        {
            return std::unexpected(ParserError::FileNotFound);
        }

        // Читаем файл
        auto file_size = std::filesystem::file_size(file_path);
        std::vector<uint8_t> data(file_size);

        std::ifstream file(file_path, std::ios::binary);
        if (!file.read(reinterpret_cast<char *>(data.data()), file_size))
        {
            return std::unexpected(ParserError::ReadError);
        }

        return parseFromMemory(data, file_path.filename().string());
    }

    auto TxtParser::parseFromMemory(const std::vector<uint8_t> &data,
                                    const std::string &file_name)
        -> std::expected<ParsedDocument, ParserError>
    {

        if (data.empty())
        {
            return std::unexpected(ParserError::EmptyContent);
        }

        // Определяем кодировку
        auto encoding_result = detectEncoding(data);
        if (!encoding_result.has_value())
        {
            return std::unexpected(encoding_result.error());
        }

        std::string encoding = *encoding_result;

        // Конвертируем в UTF-8
        auto utf8_result = convertToUtf8(data, encoding);
        if (!utf8_result.has_value())
        {
            return std::unexpected(utf8_result.error());
        }

        ParsedDocument doc;
        doc.content = *utf8_result;
        doc.encoding = encoding;
        doc.title = file_name;
        doc.file_path = file_name;
        doc.file_size = data.size();

        // Убираем BOM если есть
        if (doc.content.starts_with("\xEF\xBB\xBF"))
        {
            doc.content.erase(0, 3);
        }

        return doc;
    }

    auto TxtParser::detectEncoding(const std::vector<uint8_t> &data)
        -> std::expected<std::string, ParserError>
    {

        uchardet_t ud = uchardet_new();
        if (!ud)
        {
            return std::unexpected(ParserError::EncodingDetectionFailed);
        }

        int ret = uchardet_handle_data(ud, reinterpret_cast<const char *>(data.data()), data.size());
        if (ret != 0)
        {
            uchardet_delete(ud);
            return std::unexpected(ParserError::EncodingDetectionFailed);
        }

        uchardet_data_end(ud);

        const char *encoding = uchardet_get_charset(ud);
        std::string result(encoding ? encoding : "UTF-8");

        uchardet_delete(ud);

        // Нормализуем имена кодировок
        if (result == "WINDOWS-1251" || result == "windows-1251")
        {
            result = "CP1251";
        }
        else if (result == "IBM866" || result == "ibm866")
        {
            result = "CP866";
        }
        else if (result == "KOI8-R" || result == "koi8-r")
        {
            result = "KOI8-R";
        }

        return result;
    }

    auto TxtParser::convertToUtf8(const std::vector<uint8_t> &data,
                                  const std::string &encoding)
        -> std::expected<std::string, ParserError>
    {

        // UTF-8 — используем как есть
        if (encoding == "UTF-8" || encoding == "utf-8" || encoding == "ASCII")
        {
            return std::string(reinterpret_cast<const char *>(data.data()), data.size());
        }

        // CP1251
        if (encoding == "CP1251")
        {
            return convertCp1251ToUtf8(data);
        }

        // CP866
        if (encoding == "CP866")
        {
            return convertCp866ToUtf8(data);
        }

        // KOI8-R
        if (encoding == "KOI8-R")
        {
            return convertKoi8rToUtf8(data);
        }

        // Неизвестная кодировка — пробуем как UTF-8
        return std::string(reinterpret_cast<const char *>(data.data()), data.size());
    }

    // Таблица перекодировки CP1251 → UTF-8 (первые 128 символов — ASCII, не меняются)
    std::string TxtParser::convertCp1251ToUtf8(const std::vector<uint8_t> &data)
    {
        // Таблица для CP1251 символов 0x80-0xFF → Unicode code points
        static const uint16_t cp1251_table[128] = {
            0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
            0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
            0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
            0x0098, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
            0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
            0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
            0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
            0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
            0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
            0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
            0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
            0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
            0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
            0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
            0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
            0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F};

        std::string result;
        result.reserve(data.size() * 2); // UTF-8 русские буквы занимают 2 байта

        for (uint8_t byte : data)
        {
            if (byte < 0x80)
            {
                // ASCII
                result += static_cast<char>(byte);
            }
            else
            {
                uint16_t cp = cp1251_table[byte - 0x80];
                if (cp < 0x800)
                {
                    result += static_cast<char>(0xC0 | (cp >> 6));
                    result += static_cast<char>(0x80 | (cp & 0x3F));
                }
                else
                {
                    // Только для символов вне BMP (редко для CP1251)
                    result += static_cast<char>(0xE0 | (cp >> 12));
                    result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (cp & 0x3F));
                }
            }
        }

        return result;
    }

    std::string TxtParser::convertCp866ToUtf8(const std::vector<uint8_t> &data)
    {
        // Таблица CP866 → Unicode (только верхняя половина, 0x80-0xFF)
        static const uint16_t cp866_table[128] = {
            0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
            0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
            0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
            0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
            0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
            0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
            0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
            0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
            0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
            0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
            0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
            0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
            0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
            0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
            0x0401, 0x0451, 0x0404, 0x0454, 0x0407, 0x0457, 0x040E, 0x045E,
            0x00B0, 0x2219, 0x00B7, 0x221A, 0x2116, 0x00A4, 0x25A0, 0x00A0};

        std::string result;
        result.reserve(data.size() * 2);

        for (uint8_t byte : data)
        {
            if (byte < 0x80)
            {
                result += static_cast<char>(byte);
            }
            else
            {
                uint16_t cp = cp866_table[byte - 0x80];
                if (cp < 0x800)
                {
                    result += static_cast<char>(0xC0 | (cp >> 6));
                    result += static_cast<char>(0x80 | (cp & 0x3F));
                }
                else
                {
                    result += static_cast<char>(0xE0 | (cp >> 12));
                    result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (cp & 0x3F));
                }
            }
        }

        return result;
    }

    std::string TxtParser::convertKoi8rToUtf8(const std::vector<uint8_t> &data)
    {
        // Таблица KOI8-R → Unicode
        static const uint16_t koi8r_table[128] = {
            0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524,
            0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590,
            0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248,
            0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0, 0x00B2, 0x00B7, 0x00F7,
            0x2550, 0x2551, 0x2552, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556,
            0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E,
            0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565,
            0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0x00A9,
            0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
            0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
            0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432,
            0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A,
            0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
            0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
            0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412,
            0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042A};

        std::string result;
        result.reserve(data.size() * 2);

        for (uint8_t byte : data)
        {
            if (byte < 0x80)
            {
                result += static_cast<char>(byte);
            }
            else
            {
                uint16_t cp = koi8r_table[byte - 0x80];
                if (cp < 0x800)
                {
                    result += static_cast<char>(0xC0 | (cp >> 6));
                    result += static_cast<char>(0x80 | (cp & 0x3F));
                }
                else
                {
                    result += static_cast<char>(0xE0 | (cp >> 12));
                    result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (cp & 0x3F));
                }
            }
        }

        return result;
    }

} // namespace lse