#include "book_indexer.hpp"
#include "parser/txt_parser.hpp"
#include "parser/fb2_parser.hpp"
#include "parser/docx_parser.hpp"
#include "chunker/chunker.hpp"
#include "tokenizer/tokenizer.hpp"
#include <spdlog/spdlog.h>

namespace lse
{

    BookIndexer::BookIndexer(InMemoryIndex &index, Stemmer &stemmer)
        : index_(index), stemmer_(stemmer)
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
            spdlog::error("Failed to parse {}: {}", file_path.string(),
                          static_cast<int>(parse_result.error()));
            return std::unexpected(IndexError::DocumentNotFound);
        }

        auto &doc = *parse_result;
        spdlog::info("Indexing: {}", doc.title);

        // Чанкаем
        Chunker chunker(chunker_config_);
        auto chunks = chunker.chunkify(doc.content);

        if (chunks.empty())
        {
            spdlog::warn("No chunks produced for: {}", doc.title);
            return std::vector<BookInfo>{};
        }

        std::vector<BookInfo> books;
        for (const auto &chunk : chunks)
        {
            // Токенизируем и стеммим
            auto tokens = tokenizeAndStem(chunk);

            if (tokens.empty())
            {
                continue;
            }

            // Добавляем в индекс
            uint64_t doc_id = index_.addDocument(tokens);
            books.push_back(BookInfo{doc_id, doc.title, doc.file_path});
            indexed_chunks_++;
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
            return std::unexpected(IndexError::DocumentNotFound);
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