#include "book_indexer.hpp"
#include "parser/txt_parser.hpp"
#include "parser/fb2_parser.hpp"
#include "parser/docx_parser.hpp"
#include "chunker/chunker.hpp"
#include "tokenizer/tokenizer.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <openssl/sha.h>

namespace lse
{

    // Детерминированный ID книги
    static auto generateBookId(const std::string &file_path,
                               const std::string &title,
                               const std::string &author) -> int64_t
    {
        std::string normalized_path = file_path;
        std::transform(normalized_path.begin(), normalized_path.end(),
                       normalized_path.begin(), ::tolower);

        auto strip_ext = [](std::string &s, const std::string &ext)
        {
            if (s.size() > ext.size() &&
                std::equal(ext.rbegin(), ext.rend(), s.rbegin(),
                           [](char a, char b)
                           { return std::tolower(a) == std::tolower(b); }))
            {
                s.resize(s.size() - ext.size());
            }
        };
        strip_ext(normalized_path, ".gz");
        strip_ext(normalized_path, ".txt");

        std::string data = normalized_path + "|" + title + "|" + author;

        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash);

        int64_t result = 0;
        for (int i = 0; i < 8; ++i)
        {
            result = (result << 8) | hash[i];
        }

        return result;
    }

    BookIndexer::BookIndexer(IndexWriter &writer, Stemmer &stemmer)
        : writer_(writer), stemmer_(stemmer)
    {
        tokenizer_opts_.normalize_yo = true;
        tokenizer_opts_.keep_digits = true;
        chunker_config_.min_chunk_size = 300;
        chunker_config_.opt_chunk_size = 1800;
        chunker_config_.max_chunk_size = 3500;
    }

    auto BookIndexer::tokenizeAndStem(std::string_view text) -> std::vector<Token>
    {
        Tokenizer tokenizer(text, tokenizer_opts_);
        auto result = tokenizer.tokenize();
        if (!result.has_value())
        {
            return {};
        }

        for (auto &token : *result)
        {
            token.text = std::string(stemmer_.stem(token.text));
        }

        return *result;
    }

    auto BookIndexer::indexFile(const std::filesystem::path &file_path)
        -> std::expected<std::vector<BookInfo>, IndexError>
    {

        auto ext = file_path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        std::expected<ParsedDocument, ParserError> parse_result =
            std::unexpected(ParserError::FileNotFound);

        if (ext == ".txt")
        {
            TxtParser parser;
            parse_result = parser.parse(file_path);
        }
        else if (ext == ".fb2")
        {
            Fb2Parser parser;
            parse_result = parser.parse(file_path);
        }
        else if (ext == ".docx")
        {
            DocxParser parser;
            parse_result = parser.parse(file_path);
        }
        else
        {
            spdlog::warn("Unsupported format: {}", ext);
            return std::vector<BookInfo>{};
        }

        if (!parse_result.has_value())
        {
            spdlog::error("Failed to parse {}: error_code={}", file_path.string(),
                          static_cast<int>(parse_result.error()));
            return std::unexpected(IndexError::FileError);
        }

        auto &doc = *parse_result;
        spdlog::info("Indexing: {}", doc.title);

        // Вычисляем book_id
        int64_t book_id = generateBookId(file_path.string(), doc.title, "");

        // Upsert книги
        auto upsert_result = writer_.upsertBook(book_id, doc.title, "", "", "ru", file_path.string());
        if (!upsert_result.has_value())
        {
            spdlog::error("Failed to upsert book: {}", doc.title);
            return std::unexpected(IndexError::WriteError);
        }

        // Чанкаем
        Chunker chunker(chunker_config_);
        auto chunks = chunker.chunkify(doc.content);

        if (chunks.empty())
        {
            spdlog::warn("No chunks produced for: {}", doc.title);
            return std::vector<BookInfo>{};
        }

        std::vector<BookInfo> books;
        uint32_t chunk_num = 1;
        for (const auto &chunk : chunks)
        {
            auto tokens = tokenizeAndStem(chunk);

            if (tokens.empty())
            {
                continue;
            }

            auto doc_id = writer_.addDocument(book_id, chunk_num, tokens, chunk);
            if (!doc_id.has_value())
            {
                spdlog::error("Failed to add document for chunk {} of {}", chunk_num, doc.title);
                continue;
            }

            books.push_back(BookInfo{*doc_id, doc.title, doc.file_path});
            indexed_chunks_++;
            chunk_num++;
        }

        indexed_files_++;
        return books;
    }

    auto BookIndexer::indexDirectory(const std::filesystem::path &dir_path)
        -> std::expected<size_t, IndexError>
    {

        if (!std::filesystem::exists(dir_path))
        {
            spdlog::error("Directory not found: {}", dir_path.string());
            return std::unexpected(IndexError::FileError);
        }

        size_t count = 0;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(dir_path))
        {
            if (entry.is_regular_file())
            {
                auto result = indexFile(entry.path());
                if (result.has_value() && !result->empty())
                {
                    count++;
                }
            }
        }

        spdlog::info("Indexed {} files, {} chunks", count, indexed_chunks_);
        return count;
    }

} // namespace lse