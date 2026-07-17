#pragma once

#include "memory_mapped_file.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <expected>
#include <filesystem>

struct sqlite3;

namespace lse
{

    struct Token;

    enum class IndexError
    {
        AlreadyOpen,
        NotOpen,
        FileError,
        EmptyDocument,
        WriteError
    };

    class IndexWriter
    {
    public:
        IndexWriter() = default;
        ~IndexWriter();

        // Открыть индекс для записи (создаёт новый или открывает существующий)
        auto open(const std::filesystem::path &index_dir) -> std::expected<void, IndexError>;

        // Закрыть индекс
        void close();

        // Добавить документ (чанк) в индекс.
        // book_id — детерминированный ID книги
        // tokens — токенизированный и стеммированный текст
        // Возвращает doc_id
        auto addDocument(int64_t book_id, uint32_t chunk_num,
                         const std::vector<Token> &tokens,
                         const std::string &content)
            -> std::expected<uint64_t, IndexError>;

        // Записать метаданные книги (INSERT OR IGNORE)
        auto upsertBook(int64_t book_id, const std::string &title,
                        const std::string &author, const std::string &genre,
                        const std::string &language, const std::string &file_path)
            -> std::expected<void, IndexError>;

        // Статистика
        uint64_t docCount() const { return doc_count_; }

    private:
        std::filesystem::path index_dir_;

        MemoryMappedFile meta_file_;
        MemoryMappedFile lexicon_file_;
        MemoryMappedFile postings_file_;
        sqlite3 *db_ = nullptr;

        // Буфер для накопления записей lexicon перед записью на диск
        struct LexiconEntry
        {
            uint64_t term_hash;
            std::string term_text;
            uint32_t df = 0;
            uint64_t postings_offset = 0;
            uint64_t postings_size = 0;
        };
        std::unordered_map<std::string, LexiconEntry> lexicon_buffer_;

        uint64_t doc_count_ = 0;
        uint64_t total_term_count_ = 0;
        double avg_doc_length_ = 0.0;
        uint64_t last_doc_id_ = 0;

        bool is_open_ = false;

        // Записать lexicon на диск (перезаписывает файл целиком)
        auto flushLexicon() -> std::expected<void, IndexError>;

        // Записать meta на диск
        auto flushMeta() -> std::expected<void, IndexError>;

        // Инициализировать SQLite
        auto initDatabase() -> std::expected<void, IndexError>;

        // Записать varint в буфер
        static size_t writeVarint(std::vector<uint8_t> &buf, uint64_t value);

        // Посчитать количество уникальных термов в токенах (с частотами и позициями)
        static auto countTerms(const std::vector<Token> &tokens)
            -> std::unordered_map<std::string, std::pair<uint32_t, std::vector<uint32_t>>>;
    };

} // namespace lse