#pragma once

#include "index/in_memory_index.hpp"
#include "tokenizer/stemmer.hpp"
#include "chunker/chunker.hpp"
#include <filesystem>
#include <vector>
#include <string>

namespace lse
{

    struct BookInfo
    {
        uint64_t doc_id;
        std::string title;
        std::string file_path;
    };

    class BookIndexer
    {
    public:
        BookIndexer(InMemoryIndex &index, Stemmer &stemmer);

        auto indexFile(const std::filesystem::path &file_path)
            -> std::expected<std::vector<BookInfo>, IndexError>;

        auto indexDirectory(const std::filesystem::path &dir_path)
            -> std::expected<size_t, IndexError>;

        size_t indexedFiles() const { return indexed_files_; }
        size_t indexedChunks() const { return indexed_chunks_; }

    private:
        InMemoryIndex &index_;
        Stemmer &stemmer_;
        TokenizerOptions tokenizer_opts_;
        ChunkerConfig chunker_config_;

        size_t indexed_files_ = 0;
        size_t indexed_chunks_ = 0;

        auto tokenizeAndStem(std::string_view text) -> std::vector<Token>;
    };

} // namespace lse