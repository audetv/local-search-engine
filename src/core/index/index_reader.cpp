#include "index_reader.hpp"
#include <sqlite3.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>
#include <xxhash.h>

namespace lse
{

    constexpr uint32_t META_MAGIC = 0x4C534531;
    constexpr uint32_t LEXICON_MAGIC = 0x4C455831;
    constexpr uint32_t POSTINGS_MAGIC = 0x504F5331;
    constexpr size_t LEXICON_HEADER_SIZE = 16;
    constexpr size_t TERM_ENTRY_SIZE = 56;

    IndexReader::~IndexReader()
    {
        close();
    }

    auto IndexReader::open(const std::filesystem::path &index_dir) -> std::expected<void, IndexError>
    {
        if (is_open_)
            return std::unexpected(IndexError::NotOpen);

        index_dir_ = index_dir;

        auto meta_path = index_dir / "index.meta";
        if (!meta_file_.open(meta_path, MemoryMappedFile::Mode::ReadOnly))
        {
            return std::unexpected(IndexError::FileError);
        }

        auto magic = read_le<uint32_t>(meta_file_.data(), 0);
        if (magic != META_MAGIC)
            return std::unexpected(IndexError::CorruptedIndex);

        doc_count_ = read_le<uint64_t>(meta_file_.data(), 8);
        avg_doc_length_ = read_le<double>(meta_file_.data(), 24);

        auto lexicon_path = index_dir / "lexicon.idx";
        if (!lexicon_file_.open(lexicon_path, MemoryMappedFile::Mode::ReadOnly))
        {
            return std::unexpected(IndexError::FileError);
        }

        magic = read_le<uint32_t>(lexicon_file_.data(), 0);
        if (magic != LEXICON_MAGIC)
            return std::unexpected(IndexError::CorruptedIndex);

        term_count_ = read_le<uint32_t>(lexicon_file_.data(), 4);
        lexicon_data_offset_ = read_le<uint64_t>(lexicon_file_.data(), 8);

        auto postings_path = index_dir / "postings.idx";
        if (!postings_file_.open(postings_path, MemoryMappedFile::Mode::ReadOnly))
        {
            return std::unexpected(IndexError::FileError);
        }

        auto db_path = (index_dir / "documents.db").string();
        int rc = sqlite3_open_v2(db_path.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        if (rc != SQLITE_OK)
            return std::unexpected(IndexError::FileError);

        sqlite3_exec(db_, "PRAGMA wal_checkpoint(TRUNCATE)", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "PRAGMA query_only=ON", nullptr, nullptr, nullptr);

        auto stmt_result = prepareStatements();
        if (!stmt_result.has_value())
            return std::unexpected(stmt_result.error());

        is_open_ = true;
        spdlog::info("IndexReader opened: {} terms, {} docs", term_count_, doc_count_);
        return {};
    }

    void IndexReader::close()
    {
        if (db_)
        {
            sqlite3_close(db_);
            db_ = nullptr;
        }

        meta_file_.close();
        lexicon_file_.close();
        postings_file_.close();
        is_open_ = false;
    }

    auto IndexReader::findTerm(std::string_view term) const -> LexiconResult
    {
        if (term_count_ == 0)
            return {false, {}, 0};

        uint64_t target_hash = XXH64(term.data(), term.size(), 0);

        const uint8_t *data = lexicon_file_.data();

        int64_t left = 0, right = term_count_ - 1;
        while (left <= right)
        {
            int64_t mid = left + (right - left) / 2;
            size_t entry_offset = LEXICON_HEADER_SIZE + mid * TERM_ENTRY_SIZE;

            uint64_t mid_hash = read_le<uint64_t>(data, entry_offset);

            if (mid_hash < target_hash)
            {
                left = mid + 1;
            }
            else if (mid_hash > target_hash)
            {
                right = mid - 1;
            }
            else
            {
                uint64_t blocks_offset = read_le<uint64_t>(data, entry_offset + 24);
                uint32_t df = read_le<uint32_t>(data, entry_offset + 12);

                std::vector<PostingBlockInfo> blocks;
                const uint8_t *blocks_ptr = data + blocks_offset;

                uint32_t loaded_block_count = read_le<uint32_t>(blocks_ptr, 0);
                blocks_ptr += 4;

                for (uint32_t j = 0; j < loaded_block_count; ++j)
                {
                    PostingBlockInfo block;
                    block.offset = read_le<uint64_t>(blocks_ptr, 0);
                    block.size = read_le<uint64_t>(blocks_ptr, 8);
                    blocks.push_back(block);
                    blocks_ptr += 16;
                }

                return {true, std::move(blocks), df};
            }
        }

        return {false, {}, 0};
    }

    auto IndexReader::decodePostings(const std::vector<PostingBlockInfo> &blocks) const
        -> std::vector<std::pair<uint64_t, std::pair<uint32_t, std::vector<uint32_t>>>>
    {
        std::vector<std::pair<uint64_t, std::pair<uint32_t, std::vector<uint32_t>>>> result;

        uint64_t prev_doc_id = 0;

        for (const auto &block : blocks)
        {
            const uint8_t *data = postings_file_.data() + block.offset;
            size_t pos = 0;

            uint32_t doc_count = static_cast<uint32_t>(decode_varint(data, pos));

            for (uint32_t i = 0; i < doc_count; ++i)
            {
                uint64_t doc_id = decode_varint(data, pos) + prev_doc_id;
                prev_doc_id = doc_id;

                uint32_t tf = static_cast<uint32_t>(decode_varint(data, pos));
                uint32_t positions_count = static_cast<uint32_t>(decode_varint(data, pos));

                std::vector<uint32_t> positions;
                uint32_t prev_pos = 0;
                for (uint32_t j = 0; j < positions_count; ++j)
                {
                    uint32_t pos_val = static_cast<uint32_t>(decode_varint(data, pos)) + prev_pos;
                    positions.push_back(pos_val);
                    prev_pos = pos_val;
                }

                result.push_back({doc_id, {tf, std::move(positions)}});
            }
        }

        return result;
    }

    auto IndexReader::search(const std::vector<std::string> &query_terms,
                             size_t top_k,
                             const std::string &genre_filter,
                             const std::string &author_filter)
        -> std::expected<std::vector<std::unique_ptr<SearchHit>>, IndexError>
    {

        if (!is_open_)
            return std::unexpected(IndexError::NotOpen);
        if (query_terms.empty())
            return std::unexpected(IndexError::EmptyQuery);

        std::unordered_map<uint64_t, float> doc_scores;

        for (const auto &term : query_terms)
        {
            auto lex_result = findTerm(term);
            if (!lex_result.found)
                continue;

            double idf = calculateIDF(lex_result.df);
            auto postings = decodePostings(lex_result.blocks);

            for (const auto &[doc_id, info] : postings)
            {
                const auto &[tf, positions] = info;
                doc_scores[doc_id] += static_cast<float>(calculateBM25(tf, static_cast<uint32_t>(positions.size()), idf));
            }
        }

        if (doc_scores.empty())
        {
            return std::vector<std::unique_ptr<SearchHit>>{};
        }

        std::vector<std::pair<uint64_t, float>> scored_docs(doc_scores.begin(), doc_scores.end());
        std::sort(scored_docs.begin(), scored_docs.end(),
                  [](const auto &a, const auto &b)
                  { return a.second > b.second; });

        if (scored_docs.size() > top_k)
        {
            scored_docs.resize(top_k);
        }

        std::vector<std::unique_ptr<SearchHit>> results;
        results.reserve(scored_docs.size());

        const char *sql_batch = R"(
        SELECT c.doc_id, c.book_id, c.chunk_num, b.title, b.author, b.genre, c.content
        FROM chunks c
        JOIN books b ON c.book_id = b.id
        WHERE c.doc_id = ?
    )";

        sqlite3_stmt *stmt = nullptr;
        sqlite3_prepare_v2(db_, sql_batch, -1, &stmt, nullptr);

        for (const auto &[doc_id, score] : scored_docs)
        {
            sqlite3_reset(stmt);
            sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(doc_id));

            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                auto hit = std::make_unique<SearchHit>();
                hit->doc_id = doc_id;
                hit->bm25_score = score;
                hit->book_id = sqlite3_column_int64(stmt, 1);
                hit->chunk_num = static_cast<uint32_t>(sqlite3_column_int(stmt, 2));

                const char *title = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                const char *author = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
                const char *genre = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
                const char *content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));

                if (title)
                    hit->title = title;
                if (author)
                    hit->author = author;
                if (genre)
                    hit->genre = genre;
                if (content)
                    hit->content = content;

                if (!genre_filter.empty() && hit->genre != genre_filter)
                    continue;
                if (!author_filter.empty() && hit->author != author_filter)
                    continue;

                results.push_back(std::move(hit));
            }
        }

        sqlite3_finalize(stmt);

        return results;
    }

    auto IndexReader::getChunkMetadata(uint64_t doc_id) -> std::expected<SearchHit, IndexError>
    {
        const char *sql = R"(
        SELECT c.book_id, c.chunk_num, b.title, b.author, b.genre, c.content
        FROM chunks c
        JOIN books b ON c.book_id = b.id
        WHERE c.doc_id = ?
    )";

        sqlite3_stmt *stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            return std::unexpected(IndexError::FileError);
        }

        sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(doc_id));

        std::expected<SearchHit, IndexError> result = std::unexpected(IndexError::CorruptedIndex);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            SearchHit hit;
            hit.doc_id = doc_id;
            hit.book_id = sqlite3_column_int64(stmt, 0);
            hit.chunk_num = static_cast<uint32_t>(sqlite3_column_int(stmt, 1));

            const char *title = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            const char *author = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            const char *genre = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            const char *content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));

            if (title)
                hit.title = title;
            if (author)
                hit.author = author;
            if (genre)
                hit.genre = genre;
            if (content)
                hit.content = content;

            result = std::move(hit);
        }

        sqlite3_finalize(stmt);
        return result;
    }

    auto IndexReader::prepareStatements() -> std::expected<void, IndexError>
    {
        return {};
    }

    double IndexReader::calculateBM25(uint32_t tf, uint32_t doc_length, double idf) const
    {
        double numerator = tf * (k1 + 1.0);
        double denominator = tf + k1 * (1.0 - b_param + b_param * (doc_length / avg_doc_length_));
        return idf * (numerator / denominator);
    }

    double IndexReader::calculateIDF(uint32_t df) const
    {
        if (df == 0)
            return 0.0;
        return std::log(static_cast<double>(doc_count_ - df + 0.5) / (df + 0.5) + 1.0);
    }

} // namespace lse