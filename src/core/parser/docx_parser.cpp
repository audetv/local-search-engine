#include "docx_parser.hpp"
#include <pugixml.hpp>
#include <minizip/unzip.h>
#include <fstream>
#include <sstream>
#include <vector>

namespace lse
{

    // RAII обёртка для unzFile
    class UnzipFile
    {
    public:
        explicit UnzipFile(const std::filesystem::path &path)
            : file_(unzOpen(path.string().c_str())) {}

        ~UnzipFile()
        {
            if (file_)
                unzClose(file_);
        }

        UnzipFile(const UnzipFile &) = delete;
        UnzipFile &operator=(const UnzipFile &) = delete;

        operator bool() const { return file_ != nullptr; }
        unzFile get() const { return file_; }

    private:
        unzFile file_;
    };

    // Читает файл из ZIP-архива в строку
    static auto readZipFile(unzFile uf, const std::string &filename) -> std::expected<std::string, ParserError>
    {
        if (unzLocateFile(uf, filename.c_str(), 0) != UNZ_OK)
        {
            return std::unexpected(ParserError::ReadError);
        }

        if (unzOpenCurrentFile(uf) != UNZ_OK)
        {
            return std::unexpected(ParserError::ReadError);
        }

        std::string result;
        constexpr size_t buf_size = 8192;
        std::vector<char> buf(buf_size);
        int bytes_read;

        while ((bytes_read = unzReadCurrentFile(uf, buf.data(), buf_size)) > 0)
        {
            result.append(buf.data(), bytes_read);
        }

        unzCloseCurrentFile(uf);

        if (result.empty())
        {
            return std::unexpected(ParserError::EmptyContent);
        }

        return result;
    }

    auto DocxParser::parse(const std::filesystem::path &file_path)
        -> std::expected<ParsedDocument, ParserError>
    {

        std::error_code ec;
        if (!std::filesystem::exists(file_path, ec))
        {
            return std::unexpected(ParserError::FileNotFound);
        }

        UnzipFile uf(file_path);
        if (!uf)
        {
            return std::unexpected(ParserError::ReadError);
        }

        ParsedDocument result;
        result.file_path = file_path.filename().string();
        result.encoding = "UTF-8";
        result.file_size = std::filesystem::file_size(file_path);

        // Читаем документ
        auto doc_xml = readZipFile(uf.get(), "word/document.xml");
        if (!doc_xml.has_value())
        {
            return std::unexpected(doc_xml.error());
        }

        // Парсим текст из document.xml
        pugi::xml_document doc;
        if (!doc.load_string(doc_xml->c_str()))
        {
            return std::unexpected(ParserError::ReadError);
        }

        std::string content;
        auto para_nodes = doc.select_nodes("//w:p");

        for (const auto &para : para_nodes)
        {
            std::string para_text;
            for (const auto &t : para.node().select_nodes(".//w:t"))
            {
                para_text += t.node().text().as_string();
            }

            if (!para_text.empty())
            {
                if (!content.empty())
                {
                    content += '\n';
                }
                content += para_text;
            }
        }

        if (content.empty())
        {
            return std::unexpected(ParserError::EmptyContent);
        }

        result.content = content;

        // Читаем метаданные
        auto core_xml = readZipFile(uf.get(), "docProps/core.xml");
        if (core_xml.has_value())
        {
            pugi::xml_document core_doc;
            if (core_doc.load_string(core_xml->c_str()))
            {
                auto title_node = core_doc.select_node("//dc:title");
                if (title_node.node())
                {
                    result.title = title_node.node().text().as_string();
                }
            }
        }

        // Fallback: имя файла
        if (result.title.empty())
        {
            result.title = file_path.filename().string();
            size_t dot = result.title.find_last_of('.');
            if (dot != std::string::npos)
            {
                result.title = result.title.substr(0, dot);
            }
        }

        return result;
    }

} // namespace lse