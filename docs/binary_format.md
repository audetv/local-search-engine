# Бинарный формат индекса Local Search Engine

Версия: 1.1  
Дата: 2026-07-17

## Обзор

Индекс на диске состоит из трёх файлов:

| Файл | Назначение |
|------|-----------|
| `index.meta` | Метаданные индекса (фиксированный размер) |
| `lexicon.idx` | Словарь термов с указателями на постинг-листы |
| `postings.idx` | Постинг-листы (списки документов для каждого терма) |

Плюс SQLite база данных `documents.db` для метаданных документов и книг.

---

## 1. `index.meta` — метаданные индекса

Фиксированный размер: **128 байт**.

| Offset | Size | Type | Field | Описание |
|--------|------|------|-------|----------|
| 0 | 4 | uint32 | magic | `0x4C534531` ("LSE1") |
| 4 | 4 | uint32 | version | Версия формата (1) |
| 8 | 8 | uint64 | doc_count | Количество документов в индексе |
| 16 | 8 | uint64 | total_term_count | Общее количество токенов во всех документах |
| 24 | 8 | double | avg_doc_length | Средняя длина документа в токенах |
| 32 | 8 | uint64 | last_doc_id | Последний выданный doc_id |
| 40 | 88 | — | reserved | Зарезервировано (нули) |

---

## 2. `lexicon.idx` — словарь термов

Состоит из трёх секций: заголовок, таблица термов, тексты термов.

### 2.1 Заголовок (16 байт)

| Offset | Size | Type | Field | Описание |
|--------|------|------|-------|----------|
| 0 | 4 | uint32 | magic | `0x4C455831` ("LEX1") |
| 4 | 4 | uint32 | term_count | Количество термов в словаре |
| 8 | 8 | uint64 | data_offset | Смещение начала секции текстов термов |

### 2.2 Запись терма (TermEntry) — 40 байт

| Offset | Size | Type | Field | Описание |
|--------|------|------|-------|----------|
| 0 | 8 | uint64 | term_hash | xxHash64 текста терма |
| 8 | 4 | uint32 | term_length | Длина текста терма в байтах |
| 12 | 4 | uint32 | df | Document frequency (в скольких документах встречается) |
| 16 | 8 | uint64 | postings_offset | Смещение данных постинг-листа в `postings.idx` |
| 24 | 8 | uint64 | postings_size | Размер данных постинг-листа в байтах |
| 32 | 8 | uint64 | text_offset | Смещение текста терма в секции C |

Таблица отсортирована по `term_hash` (по возрастанию). Поиск — бинарный.

### 2.3 Секция текстов термов

Непрерывный блок UTF-8 строк, каждая завершается нулевым байтом (`\0`).  
Порядок строк совпадает с порядком записей в таблице термов.

---

## 3. `postings.idx` — постинг-листы

### 3.1 Заголовок (12 байт)

| Offset | Size | Type | Field | Описание |
|--------|------|------|-------|----------|
| 0 | 4 | uint32 | magic | `0x504F5331` ("POS1") |
| 4 | 4 | uint32 | posting_count | Количество постинг-листов |
| 8 | 4 | uint32 | flags | Бит 0: positions_present (1 = позиции хранятся) |

### 3.2 Постинг-лист (один терм)

Для каждого терма записывается:

| Field | Type | Описание |
|-------|------|----------|
| doc_count | uint32 (varint) | Количество документов для этого терма |
| posting[0] | Posting | |
| posting[1] | Posting | |
| ... | | |
| posting[doc_count-1] | Posting | |

### 3.3 Формат одного Posting

| Field | Type | Описание |
|-------|------|----------|
| doc_id | uint64 (varint) | Дельта-кодированный doc_id |
| term_freq | uint32 (varint) | Частота терма в этом документе |
| positions_count | uint32 (varint) | Количество позиций (0 если флаг positions_present=0) |
| position[0] | uint32 (varint) | Дельта-кодированная позиция |
| position[1] | uint32 (varint) | |
| ... | | |

### 3.4 Дельта-кодирование

- **doc_id:** первый doc_id в списке записывается как есть. Каждый следующий — как разность с предыдущим: `delta[i] = doc_id[i] - doc_id[i-1]`.
- **positions:** аналогично, внутри одного Posting.

### 3.5 Varint кодирование (LEB128)

Целое число кодируется в 1–10 байт:
- Биты 0–6 каждого байта: данные
- Бит 7: флаг продолжения (1 = есть следующий байт)

| Диапазон значений | Байт |
|-------------------|------|
| 0–127 | 1 |
| 128–16383 | 2 |
| ... | ... |
| до 2^63-1 | до 10 |

---

## 4. `documents.db` — SQLite

```sql
CREATE TABLE books (
    id INTEGER PRIMARY KEY,              -- детерминированный хеш (int64)
    title TEXT NOT NULL,
    author TEXT DEFAULT '',
    genre TEXT DEFAULT '',
    source TEXT DEFAULT 'local',
    language TEXT DEFAULT 'ru',
    file_path TEXT,
    created_at INTEGER
);

CREATE TABLE chunks (
    doc_id INTEGER PRIMARY KEY,          -- совпадает с doc_id из индекса
    book_id INTEGER NOT NULL,            -- ссылка на books.id
    chunk_num INTEGER NOT NULL,
    word_count INTEGER DEFAULT 0,
    char_count INTEGER DEFAULT 0,
    content TEXT,
    FOREIGN KEY (book_id) REFERENCES books(id)
);

CREATE INDEX idx_chunks_book ON chunks(book_id);
CREATE INDEX idx_books_genre ON books(genre);
CREATE INDEX idx_books_author ON books(author);
```

### Детерминированный ID книги

ID книги генерируется как первые 8 байт SHA256-хеша от строки `нормализованный_путь|название|автор`.

**Зачем:** одна и та же книга (тот же файл, название, автор) всегда получает один и тот же `book_id`. Это решает проблему дедупликации при переиндексации без необходимости хранить отдельный UUID.

```cpp
#include <openssl/sha.h>
#include <algorithm>
#include <string>

// Генерирует детерминированный ID книги
inline auto generateBookId(const std::string& file_path,
                            const std::string& title,
                            const std::string& author) -> int64_t {
    // Нормализуем путь: нижний регистр, убираем расширения
    std::string normalized_path = file_path;
    std::transform(normalized_path.begin(), normalized_path.end(),
                   normalized_path.begin(), ::tolower);
    
    auto strip_ext = [](std::string& s, const std::string& ext) {
        if (s.size() > ext.size() && 
            std::equal(ext.rbegin(), ext.rend(), s.rbegin(),
                       [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
            s.resize(s.size() - ext.size());
        }
    };
    strip_ext(normalized_path, ".gz");
    strip_ext(normalized_path, ".txt");
    
    // Формируем строку для хеширования
    std::string data = normalized_path + "|" + title + "|" + author;
    
    // SHA256
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()),
           data.size(), hash);
    
    // Первые 8 байт как int64_t (big-endian)
    int64_t result = 0;
    for (int i = 0; i < 8; ++i) {
        result = (result << 8) | hash[i];
    }
    
    return result;
}
```

**Почему int64, а не TEXT:**
- 8 байт против 16+ символов — компактный индекс
- Быстрые JOIN'ы по INTEGER
- Вероятность коллизии на 100 000 книг: ~3×10⁻¹⁴ (пренебрежимо мала)

---

## 5. Константы формата

| Константа | Значение | Описание |
|-----------|----------|----------|
| META_MAGIC | 0x4C534531 | "LSE1" |
| LEXICON_MAGIC | 0x4C455831 | "LEX1" |
| POSTINGS_MAGIC | 0x504F5331 | "POS1" |
| INDEX_VERSION | 1 | Версия формата |
| META_SIZE | 128 байт | Размер index.meta |
| LEXICON_HEADER_SIZE | 16 байт | Размер заголовка lexicon |
| TERM_ENTRY_SIZE | 40 байт | Размер одной записи терма |
| POSTINGS_HEADER_SIZE | 12 байт | Размер заголовка postings |

---

## 6. Алгоритмы

### 6.1 Добавление документа (IndexWriter)

1. Вычислить `book_id = generateBookId(file_path, title, author)`
2. `INSERT OR IGNORE INTO books (id, title, author, ...) VALUES (book_id, ...)` — если книга уже есть, пропускаем
3. Токенизировать + стеммить текст чанка
4. `INSERT INTO chunks (book_id, chunk_num, ...) VALUES (book_id, ...)` → получить `doc_id`
5. Для каждого уникального терма:
   - Найти терм в lexicon (бинарный поиск по `term_hash`)
   - Если не найден — добавить новую TermEntry, записать текст терма в секцию C
   - Дописать Posting в конец `postings.idx`
   - Обновить `df` и `postings_size` в TermEntry
6. Обновить `index.meta` (`doc_count`, `total_term_count`, `avg_doc_length`)

### 6.2 Поиск (IndexReader)

1. Токенизировать + стеммить запрос
2. Для каждого терма запроса:
   - Бинарный поиск по `lexicon.idx` → получить `postings_offset`
   - Прочитать постинг-лист из `postings.idx` через mmap
   - Декодировать varint + дельта → получить `[doc_id, tf, positions]`
3. Пересечь списки doc_id (AND) или объединить (OR)
4. Для каждого документа вычислить BM25 score
5. Отсортировать по score, вернуть top-K
6. По `doc_id` получить метаданные из SQLite: `SELECT * FROM chunks JOIN books ON chunks.book_id = books.id WHERE chunks.doc_id = ?`
7. Применить фильтры (жанр, автор) — через JOIN по `books.id = chunks.book_id`

### 6.3 Дедупликация книг при переиндексации

1. Вычислить `book_id` детерминированно
2. `INSERT OR IGNORE INTO books` — если книга уже существует, вставка игнорируется
3. Сравнить `file_path` и `created_at`: если файл новее — обновить метаданные книги и переиндексировать все чанки

---

## 7. Версионирование

При изменении формата:
1. Увеличить `INDEX_VERSION`
2. Добавить код миграции в IndexReader
3. Обновить этот документ