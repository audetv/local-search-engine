#include "index_writer.hpp"
#include "../tokenizer/tokenizer.hpp"
#include <sqlite3.h>
#include <algorithm>
#include <cstring>
#include <spdlog/spdlog.h>

namespace lse
{

    // Константы формата
    constexpr uint32_t META_MAGIC = 0x4C534531;
    constexpr uint32_t LEXICON_MAGIC = 0x4C455831;
    constexpr uint32_t POSTINGS_MAGIC = 0x504F5331;
    constexpr uint32_t INDEX_VERSION = 1;
    constexpr size_t META_SIZE = 128;
    constexpr size_t LEXICON_HEADER_SIZE = 16;
    constexpr size_t TERM_ENTRY_SIZE = 40;
    constexpr size_t POSTINGS_HEADER_SIZE = 12;

    IndexWriter::~IndexWriter()
    {
        close();
    }

    auto IndexWriter::open(const std::filesystem::path &index_dir) -> std::expected<void, IndexError>
    {
        if (is_open_)
            return std::unexpected(IndexError::AlreadyOpen);

        index_dir_ = index_dir;
        std::error_code ec;
        std::filesystem::create_directories(index_dir, ec);

        // Открываем meta
        auto meta_path = index_dir / "index.meta";
        bool meta_exists = std::filesystem::exists(meta_path);

        if (!meta_file_.open(meta_path, meta_exists ? MemoryMappedFile::Mode::ReadWrite : MemoryMappedFile::Mode::CreateNew, META_SIZE))
        {
            return std::unexpected(IndexError::FileError);
        }

        // Читаем или инициализируем meta
        if (meta_exists)
        {
            auto magic = read_le<uint32_t>(meta_file_.data(), 0);
            if (magic != META_MAGIC)
                return std::unexpected(IndexError::FileError);

            doc_count_ = read_le<uint64_t>(meta_file_.data(), 8);
            total_term_count_ = read_le<uint64_t>(meta_file_.data(), 16);
            avg_doc_length_ = read_le<double>(meta_file_.data(), 24);
            last_doc_id_ = read_le<uint64_t>(meta_file_.data(), 32);
        }
        else
        {
            write_le<uint32_t>(meta_file_.data(), 0, META_MAGIC);
            write_le<uint32_t>(meta_file_.data(), 4, INDEX_VERSION);
        }

        // Открываем postings (всегда ReadWrite)
        auto postings_path = index_dir / "postings.idx";
        bool postings_exists = std::filesystem::exists(postings_path);

        if (!postings_file_.open(postings_path,
                                 postings_exists ? MemoryMappedFile::Mode::ReadWrite
                                                 : MemoryMappedFile::Mode::CreateNew,
                                 postings_exists ? 0 : POSTINGS_HEADER_SIZE))
        {
            return std::unexpected(IndexError::FileError);
        }

        // Инициализируем заголовок postings
        if (!postings_exists || postings_file_.size() == 0)
        {
            postings_file_.grow(POSTINGS_HEADER_SIZE);
            write_le<uint32_t>(postings_file_.data(), 0, POSTINGS_MAGIC);
            write_le<uint32_t>(postings_file_.data(), 4, 0); // posting_count
            write_le<uint32_t>(postings_file_.data(), 8, 1); // flags: positions_present
        }

        // Инициализируем SQLite
        auto db_result = initDatabase();
        if (!db_result.has_value())
            return std::unexpected(db_result.error());

        // Загружаем lexicon в память
        auto lexicon_path = index_dir / "lexicon.idx";
        if (std::filesystem::exists(lexicon_path))
        {
            MemoryMappedFile lex_mmap;
            if (!lex_mmap.open(lexicon_path, MemoryMappedFile::Mode::ReadOnly))
            {
                return std::unexpected(IndexError::FileError);
            }

            auto magic = read_le<uint32_t>(lex_mmap.data(), 0);
            if (magic != LEXICON_MAGIC)
                return std::unexpected(IndexError::FileError);

            uint32_t term_count = read_le<uint32_t>(lex_mmap.data(), 4);
            uint64_t data_offset = read_le<uint64_t>(lex_mmap.data(), 8);

            for (uint32_t i = 0; i < term_count; ++i)
            {
                size_t entry_offset = LEXICON_HEADER_SIZE + i * TERM_ENTRY_SIZE;

                LexiconEntry entry;
                entry.term_hash = read_le<uint64_t>(lex_mmap.data(), entry_offset);
                entry.df = read_le<uint32_t>(lex_mmap.data(), entry_offset + 12);
                entry.postings_offset = read_le<uint64_t>(lex_mmap.data(), entry_offset + 16);
                entry.postings_size = read_le<uint64_t>(lex_mmap.data(), entry_offset + 24);

                uint64_t text_offset = read_le<uint64_t>(lex_mmap.data(), entry_offset + 32);
                entry.term_text = reinterpret_cast<const char *>(lex_mmap.data() + data_offset + text_offset);

                lexicon_buffer_[entry.term_text] = entry;
            }
        }

        is_open_ = true;
        spdlog::info("IndexWriter opened: {} ({} docs)", index_dir.string(), doc_count_);
        return {};
    }

    void IndexWriter::close()
    {
        if (!is_open_)
            return;

        flushLexicon();
        flushMeta();

        meta_file_.close();
        lexicon_file_.close();
        postings_file_.close();

        if (db_)
        {
            sqlite3_close(db_);
            db_ = nullptr;
        }

        lexicon_buffer_.clear();
        is_open_ = false;
    }

    auto IndexWriter::upsertBook(int64_t book_id, const std::string &title,
                                 const std::string &author, const std::string &genre,
                                 const std::string &language, const std::string &file_path)
        -> std::expected<void, IndexError>
    {
        if (!is_open_)
            return std::unexpected(IndexError::NotOpen);

        const char *sql = R"(
        INSERT OR IGNORE INTO books (id, title, author, genre, language, file_path, created_at)
        VALUES (?, ?, ?, ?, ?, ?, unixepoch())
    )";

        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            return std::unexpected(IndexError::WriteError);
        }

        sqlite3_bind_int64(stmt, 1, book_id);
        sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, author.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, genre.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, language.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, file_path.c_str(), -1, SQLITE_TRANSIENT);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            return std::unexpected(IndexError::WriteError);
        }

        return {};
    }

    auto IndexWriter::addDocument(int64_t book_id, uint32_t chunk_num,
                                  const std::vector<Token> &tokens,
                                  const std::string &content)
        -> std::expected<uint64_t, IndexError>
    {
        if (!is_open_)
            return std::unexpected(IndexError::NotOpen);
        if (tokens.empty())
            return std::unexpected(IndexError::EmptyDocument);

        // 1. Добавляем чанк в SQLite
        uint64_t doc_id = last_doc_id_++;

        const char *sql = R"(
        INSERT INTO chunks (doc_id, book_id, chunk_num, word_count, char_count, content)
        VALUES (?, ?, ?, ?, ?, ?)
    )";

        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            return std::unexpected(IndexError::WriteError);
        }

        sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(doc_id));
        sqlite3_bind_int64(stmt, 2, book_id);
        sqlite3_bind_int(stmt, 3, chunk_num);
        sqlite3_bind_int(stmt, 4, static_cast<int>(tokens.size()));
        sqlite3_bind_int(stmt, 5, static_cast<int>(content.size()));
        sqlite3_bind_text(stmt, 6, content.c_str(), -1, SQLITE_TRANSIENT);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            return std::unexpected(IndexError::WriteError);
        }

        // 2. Считаем термы
        auto term_stats = countTerms(tokens);

        // 3. Для каждого терма: обновляем lexicon, дописываем posting
        size_t current_postings_end = postings_file_.size();

        for (const auto &[term_text, stats] : term_stats)
        {
            const auto &[tf, positions] = stats;

            // Находим или создаём запись в lexicon
            auto &entry = lexicon_buffer_[term_text];
            if (entry.term_text.empty())
            {
                entry.term_text = term_text;
                entry.term_hash = 0; // TODO: xxHash64(term_text)
                entry.df = 0;
                entry.postings_offset = 0;
                entry.postings_size = 0;
            }

            // Сериализуем posting
            std::vector<uint8_t> posting_data;
            // doc_id (дельта от предыдущего? пока пишем как есть)
            writeVarint(posting_data, doc_id);
            // term_freq
            writeVarint(posting_data, tf);
            // positions_count
            writeVarint(posting_data, positions.size());
            // positions (дельта-кодированные)
            uint32_t prev_pos = 0;
            for (uint32_t pos : positions)
            {
                writeVarint(posting_data, pos - prev_pos);
                prev_pos = pos;
            }

            // Дописываем в postings файл
            size_t new_size = current_postings_end + posting_data.size();
            if (new_size > postings_file_.size())
            {
                postings_file_.grow(new_size);
            }
            std::memcpy(postings_file_.data() + current_postings_end,
                        posting_data.data(), posting_data.size());

            entry.postings_offset = current_postings_end;
            entry.postings_size = posting_data.size();
            entry.df++;

            current_postings_end += posting_data.size();
        }

        // 4. Обновляем метаданные
        doc_count_++;
        total_term_count_ += tokens.size();
        avg_doc_length_ = static_cast<double>(total_term_count_) / doc_count_;

        return doc_id;
    }

    auto IndexWriter::flushLexicon() -> std::expected<void, IndexError>
    {
        auto lexicon_path = index_dir_ / "lexicon.idx";

        // Сортируем записи по term_hash
        std::vector<LexiconEntry *> sorted;
        sorted.reserve(lexicon_buffer_.size());
        for (auto &[_, entry] : lexicon_buffer_)
        {
            sorted.push_back(&entry);
        }
        std::sort(sorted.begin(), sorted.end(),
                  [](const LexiconEntry *a, const LexiconEntry *b)
                  {
                      return a->term_hash < b->term_hash;
                  });

        // Вычисляем размеры
        size_t data_size = 0;
        for (auto *entry : sorted)
        {
            data_size += entry->term_text.size() + 1; // +1 за нулевой терминатор
        }

        size_t header_size = LEXICON_HEADER_SIZE;
        size_t table_size = sorted.size() * TERM_ENTRY_SIZE;
        size_t total_size = header_size + table_size + data_size;

        // Открываем файл для записи
        lexicon_file_.close();
        if (!lexicon_file_.open(lexicon_path, MemoryMappedFile::Mode::CreateNew, total_size))
        {
            return std::unexpected(IndexError::FileError);
        }

        // Заголовок
        write_le<uint32_t>(lexicon_file_.data(), 0, LEXICON_MAGIC);
        write_le<uint32_t>(lexicon_file_.data(), 4, static_cast<uint32_t>(sorted.size()));
        write_le<uint64_t>(lexicon_file_.data(), 8, header_size + table_size);

        // Таблица термов
        uint64_t text_offset = 0;
        for (size_t i = 0; i < sorted.size(); ++i)
        {
            auto *entry = sorted[i];
            size_t entry_offset = header_size + i * TERM_ENTRY_SIZE;

            write_le<uint64_t>(lexicon_file_.data(), entry_offset, entry->term_hash);
            write_le<uint32_t>(lexicon_file_.data(), entry_offset + 8,
                               static_cast<uint32_t>(entry->term_text.size()));
            write_le<uint32_t>(lexicon_file_.data(), entry_offset + 12, entry->df);
            write_le<uint64_t>(lexicon_file_.data(), entry_offset + 16, entry->postings_offset);
            write_le<uint64_t>(lexicon_file_.data(), entry_offset + 24, entry->postings_size);
            write_le<uint64_t>(lexicon_file_.data(), entry_offset + 32, text_offset);

            text_offset += entry->term_text.size() + 1;
        }

        // Тексты термов
        uint8_t *text_start = lexicon_file_.data() + header_size + table_size;
        text_offset = 0;
        for (auto *entry : sorted)
        {
            std::memcpy(text_start + text_offset, entry->term_text.c_str(),
                        entry->term_text.size());
            text_start[text_offset + entry->term_text.size()] = 0;
            text_offset += entry->term_text.size() + 1;
        }

        lexicon_file_.flush();
        return {};
    }

    auto IndexWriter::flushMeta() -> std::expected<void, IndexError>
    {
        write_le<uint64_t>(meta_file_.data(), 8, doc_count_);
        write_le<uint64_t>(meta_file_.data(), 16, total_term_count_);
        write_le<double>(meta_file_.data(), 24, avg_doc_length_);
        write_le<uint64_t>(meta_file_.data(), 32, last_doc_id_);
        meta_file_.flush();
        return {};
    }

    auto IndexWriter::initDatabase() -> std::expected<void, IndexError>
    {
        auto db_path = (index_dir_ / "documents.db").string();

        int rc = sqlite3_open(db_path.c_str(), &db_);
        if (rc != SQLITE_OK)
            return std::unexpected(IndexError::FileError);

        // WAL mode для лучшей производительности
        sqlite3_exec(db_, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "PRAGMA synchronous=NORMAL", nullptr, nullptr, nullptr);

        const char *create_sql = R"(
        CREATE TABLE IF NOT EXISTS books (
            id INTEGER PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT DEFAULT '',
            genre TEXT DEFAULT '',
            source TEXT DEFAULT 'local',
            language TEXT DEFAULT 'ru',
            file_path TEXT,
            created_at INTEGER
        );
        
        CREATE TABLE IF NOT EXISTS chunks (
            doc_id INTEGER PRIMARY KEY,
            book_id INTEGER NOT NULL,
            chunk_num INTEGER NOT NULL,
            word_count INTEGER DEFAULT 0,
            char_count INTEGER DEFAULT 0,
            content TEXT,
            FOREIGN KEY (book_id) REFERENCES books(id)
        );
        
        CREATE INDEX IF NOT EXISTS idx_chunks_book ON chunks(book_id);
        CREATE INDEX IF NOT EXISTS idx_books_genre ON books(genre);
        CREATE INDEX IF NOT EXISTS idx_books_author ON books(author);
    )";

        char *err_msg;
        rc = sqlite3_exec(db_, create_sql, nullptr, nullptr, &err_msg);
        if (rc != SQLITE_OK)
        {
            spdlog::error("SQLite init error: {}", err_msg);
            sqlite3_free(err_msg);
            return std::unexpected(IndexError::WriteError);
        }

        return {};
    }

    size_t IndexWriter::writeVarint(std::vector<uint8_t> &buf, uint64_t value)
    {
        size_t start = buf.size();
        while (value >= 0x80)
        {
            buf.push_back((value & 0x7F) | 0x80);
            value >>= 7;
        }
        buf.push_back(value & 0x7F);
        return buf.size() - start;
    }

    auto IndexWriter::countTerms(const std::vector<Token> &tokens)
        -> std::unordered_map<std::string, std::pair<uint32_t, std::vector<uint32_t>>>
    {

        std::unordered_map<std::string, std::pair<uint32_t, std::vector<uint32_t>>> result;

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            const auto &token = tokens[i];
            auto &[tf, positions] = result[token.text];
            tf++;
            positions.push_back(static_cast<uint32_t>(i));
        }

        return result;
    }

} // namespace lse