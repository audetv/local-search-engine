#include "fb2_parser.hpp"
#include <fstream>
#include <sstream>
#include <pugixml.hpp>

namespace lse
{

    auto Fb2Parser::parse(const std::filesystem::path &file_path)
        -> std::expected<ParsedDocument, ParserError>
    {

        std::error_code ec;
        if (!std::filesystem::exists(file_path, ec))
        {
            return std::unexpected(ParserError::FileNotFound);
        }

        std::ifstream file(file_path);
        if (!file.is_open())
        {
            return std::unexpected(ParserError::ReadError);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return parseFromMemory(buffer.str(), file_path.filename().string());
    }

    auto Fb2Parser::parseFromMemory(const std::string &xml_content,
                                    const std::string &file_name)
        -> std::expected<ParsedDocument, ParserError>
    {

        if (xml_content.empty())
        {
            return std::unexpected(ParserError::EmptyContent);
        }

        pugi::xml_document doc;
        pugi::xml_parse_result parse_result = doc.load_string(xml_content.c_str());

        if (!parse_result)
        {
            return std::unexpected(ParserError::ReadError);
        }

        ParsedDocument result;
        result.file_path = file_name;
        result.encoding = "UTF-8"; // FB2 всегда UTF-8 по стандарту

        // Извлекаем название
        auto title_node = doc.select_node("/FictionBook/description/title-info/book-title");
        if (title_node.node())
        {
            result.title = title_node.node().text().as_string();
        }
        else
        {
            // Fallback: имя файла без расширения
            result.title = file_name;
            size_t dot = result.title.find_last_of('.');
            if (dot != std::string::npos)
            {
                result.title = result.title.substr(0, dot);
            }
        }

        // Извлекаем автора
        std::string author;
        // Извлекаем автора
        auto author_node = doc.select_node("/FictionBook/description/title-info/author");
        if (author_node.node())
        {
            auto first_name = author_node.node().select_node("first-name");
            auto last_name = author_node.node().select_node("last-name");

            if (first_name.node())
            {
                result.author += first_name.node().text().as_string();
            }
            if (last_name.node())
            {
                if (!result.author.empty())
                    result.author += " ";
                result.author += last_name.node().text().as_string();
            }
        }

        // Извлекаем жанр
        auto genre_node = doc.select_node("/FictionBook/description/title-info/genre");
        if (genre_node.node())
        {
            result.genre = genre_node.node().text().as_string();
        }

        // Извлекаем текст из всех <body> → <p>
        std::string content;
        auto body_nodes = doc.select_nodes("/FictionBook/body//p");

        for (const auto &p_node : body_nodes)
        {
            if (!content.empty())
            {
                content += '\n'; // разделяем параграфы
            }
            content += p_node.node().text().as_string();
        }

        if (content.empty())
        {
            return std::unexpected(ParserError::EmptyContent);
        }

        result.content = content;
        result.file_size = xml_content.size();

        return result;
    }

} // namespace lse