#pragma once

#include "parser/genre_mapper.hpp"
#include "index/index_writer.hpp"
#include "index/index_reader.hpp"
#include "tokenizer/stemmer.hpp"
#include "tokenizer/tokenizer.hpp"
#include "chunker/chunker.hpp"
#include <filesystem>
#include <vector>
#include <string>
#include <memory>

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
        BookIndexer(IndexWriter &writer, Stemmer &stemmer, GenreMapper &genre_mapper);

        auto indexFile(const std::filesystem::path &file_path)
            -> std::expected<std::vector<BookInfo>, IndexError>;

        auto indexDirectory(const std::filesystem::path &dir_path)
            -> std::expected<size_t, IndexError>;

        size_t indexedFiles() const { return indexed_files_; }
        size_t indexedChunks() const { return indexed_chunks_; }

    private:
        IndexWriter &writer_;
        Stemmer &stemmer_;
        GenreMapper &genre_mapper_;
        TokenizerOptions tokenizer_opts_;
        ChunkerConfig chunker_config_;

        size_t indexed_files_ = 0;
        size_t indexed_chunks_ = 0;

        auto tokenizeAndStem(std::string_view text) -> std::vector<Token>;
    };

} // namespace lse