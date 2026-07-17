#pragma once

#include "memory_mapped_file.hpp"
#include "index_common.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <cstdint>
#include <expected>
#include <filesystem>

struct sqlite3;
struct sqlite3_stmt;

namespace lse
{

    struct SearchHit
    {
        uint64_t doc_id;
        float bm25_score;
        std::string title;
        std::string author;
        std::string genre;
        std::string content;
        int64_t book_id;
        uint32_t chunk_num;

        SearchHit() = default;
        SearchHit(SearchHit &&other) noexcept
            : doc_id(other.doc_id), bm25_score(other.bm25_score), title(std::move(other.title)), author(std::move(other.author)), genre(std::move(other.genre)), content(std::move(other.content)), book_id(other.book_id), chunk_num(other.chunk_num)
        {
        }

        SearchHit &operator=(SearchHit &&other) noexcept
        {
            if (this != &other)
            {
                doc_id = other.doc_id;
                bm25_score = other.bm25_score;
                title = std::move(other.title);
                author = std::move(other.author);
                genre = std::move(other.genre);
                content = std::move(other.content);
                book_id = other.book_id;
                chunk_num = other.chunk_num;
            }
            return *this;
        }

        SearchHit(const SearchHit &) = delete;
        SearchHit &operator=(const SearchHit &) = delete;
    };

    struct PostingBlockInfo
    {
        uint64_t offset;
        uint64_t size;
    };

    class IndexReader
    {
    public:
        IndexReader() = default;
        ~IndexReader();

        auto open(const std::filesystem::path &index_dir) -> std::expected<void, IndexError>;
        void close();

        // Поиск. Возвращает результаты в deque (непрерывная память не требуется).
        auto search(const std::vector<std::string> &query_terms,
                    size_t top_k = 20,
                    const std::string &genre_filter = "",
                    const std::string &author_filter = "")
            -> std::expected<std::deque<SearchHit>, IndexError>;

        // Статистика
        uint64_t docCount() const { return doc_count_; }
        double avgDocLength() const { return avg_doc_length_; }

        // Получить метаданные чанка по doc_id
        auto getChunkMetadata(uint64_t doc_id) -> std::expected<SearchHit, IndexError>;

    private:
        std::filesystem::path index_dir_;

        MemoryMappedFile meta_file_;
        MemoryMappedFile lexicon_file_;
        MemoryMappedFile postings_file_;
        sqlite3 *db_ = nullptr;

        uint64_t doc_count_ = 0;
        double avg_doc_length_ = 0.0;
        uint32_t term_count_ = 0;
        uint64_t lexicon_data_offset_ = 0;

        bool is_open_ = false;

        // BM25 константы
        static constexpr double k1 = 1.2;
        static constexpr double b_param = 0.75;

        // Результат поиска терма в lexicon
        struct LexiconResult
        {
            bool found;
            std::vector<PostingBlockInfo> blocks;
            uint32_t df;
        };
        auto findTerm(std::string_view term) const -> LexiconResult;

        // Декодировать постинг-лист из списка блоков
        auto decodePostings(const std::vector<PostingBlockInfo> &blocks) const
            -> std::vector<std::pair<uint64_t, std::pair<uint32_t, std::vector<uint32_t>>>>;

        // BM25 score
        double calculateBM25(uint32_t tf, uint32_t doc_length, double idf) const;
        double calculateIDF(uint32_t df) const;

        auto prepareStatements() -> std::expected<void, IndexError>;
    };

} // namespace lse