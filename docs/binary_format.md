# Бинарный формат индекса Local Search Engine

Версия: 2.0  
Дата: 2026-07-17

## Обзор

Индекс на диске состоит из трёх файлов:

| Файл | Назначение |
|------|-----------|
| `index.meta` | Метаданные индекса (фиксированный размер) |
| `lexicon.idx` | Словарь термов с указателями на блоки постинг-листов |
| `postings.idx` | Постинг-листы (блоки документов для каждого терма) |

Плюс SQLite база данных `documents.db` для метаданных документов и книг.

### Ключевое изменение в версии 2.0

Для поддержки инкрементального добавления (без перестроения индекса) каждый терм может иметь **несколько блоков** постинг-листов в `postings.idx`. Блоки одного терма не обязательно расположены последовательно. При чтении IndexReader собирает все блоки терма и обрабатывает их как единый поток.

---

## 1. `index.meta` — метаданные индекса

Фиксированный размер: **128 байт**.

| Offset | Size | Type | Field | Описание |
|--------|------|------|-------|----------|
| 0 | 4 | uint32 | magic | `0x4C534531` ("LSE1") |
| 4 | 4 | uint32 | version | Версия формата (2) |
| 8 | 8 | uint64 | doc_count | Количество документов в индексе |
| 16 | 8 | uint64 | total_term_count | Общее количество токенов во всех документах |
| 24 | 8 | double | avg_doc_length | Средняя длина документа в токенах |
| 32 | 8 | uint64 | last_doc_id | Последний выданный doc_id |
| 40 | 88 | — | reserved | Зарезервировано (нули) |

---

## 2. `lexicon.idx` — словарь термов

Состоит из четырёх секций: заголовок (A), таблица термов (B), тексты термов (C), массивы блоков (D).

### 2.1 Секция A: Заголовок (16 байт)

| Offset | Size | Type | Field | Описание |
|--------|------|------|-------|----------|
| 0 | 4 | uint32 | magic | `0x4C455831` ("LEX1") |
| 4 | 4 | uint32 | term_count | Количество термов в словаре |
| 8 | 8 | uint64 | data_offset | Смещение начала секции C (тексты термов) |

### 2.2 Секция B: Таблица термов

Каждая запись — **56 байт** (TermEntry v2).

| Offset | Size | Type | Field | Описание |
|--------|------|------|-------|----------|
| 0 | 8 | uint64 | term_hash | xxHash64 текста терма |
| 8 | 4 | uint32 | term_length | Длина текста терма в байтах |
| 12 | 4 | uint32 | df | Document frequency (в скольких документах встречается) |
| 16 | 4 | uint32 | block_count | Количество блоков постинг-листа для этого терма |
| 20 | 4 | uint32 | reserved | Выравнивание (нули) |
| 24 | 8 | uint64 | blocks_offset | Смещение массива блоков в секции D |
| 32 | 8 | uint64 | total_postings_size | Суммарный размер всех блоков в байтах |
| 40 | 8 | uint64 | text_offset | Смещение текста терма в секции C |
| 48 | 8 | uint64 | reserved2 | Зарезервировано (нули) |

Таблица отсортирована по `term_hash` (по возрастанию). Поиск — бинарный.

### 2.3 Секция C: Тексты термов

Непрерывный блок UTF-8 строк, каждая завершается нулевым байтом (`\0`).  
Порядок строк совпадает с порядком записей в таблице термов.  
Смещение начала секции C указано в заголовке (`data_offset`).

### 2.4 Секция D: Массивы блоков

Начинается сразу после секции C (`data_offset + суммарная длина всех строк`).

Для каждого терма (в порядке таблицы термов) записывается массив блоков:

| Field | Type | Описание |
|-------|------|----------|
| block_count | uint32 | Количество блоков (дублирует поле в TermEntry) |
| blocks[0].offset | uint64 | Смещение блока в `postings.idx` |
| blocks[0].size | uint64 | Размер блока в байтах |
| blocks[1].offset | uint64 | |
| blocks[1].size | uint64 | |
| ... | | |

**Структура lexicon.idx (версия 2):**
```
[Header: 16 байт]
[TermEntry 0: 56 байт]
[TermEntry 1: 56 байт]
...
[TermEntry N-1: 56 байт]
[Секция C: тексты термов]
[Секция D: массивы блоков]
  [block_count_0: uint32]
  [offset_0_0: uint64][size_0_0: uint64]
  [offset_0_1: uint64][size_0_1: uint64]
  [block_count_1: uint32]
  [offset_1_0: uint64][size_1_0: uint64]
  ...
```

---

## 3. `postings.idx` — постинг-листы

### 3.1 Заголовок (12 байт)

| Offset | Size | Type | Field | Описание |
|--------|------|------|-------|----------|
| 0 | 4 | uint32 | magic | `0x504F5331` ("POS1") |
| 4 | 4 | uint32 | posting_count | Количество блоков постинг-листов (не документов!) |
| 8 | 4 | uint32 | flags | Бит 0: positions_present (1 = позиции хранятся) |

### 3.2 Блок постинг-листа

Каждый блок — это независимый фрагмент данных, записанный в момент добавления документа. Блоки одного терма могут быть разбросаны по файлу.

Формат блока:

| Field | Type | Описание |
|-------|------|----------|
| doc_count | uint32 (varint) | Количество документов в этом блоке |
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

- **doc_id:** первый doc_id в первом блоке записывается как есть. Каждый следующий doc_id — как разность с предыдущим: `delta[i] = doc_id[i] - doc_id[i-1]`. При переходе к следующему блоку того же терма дельта-кодирование продолжается от последнего doc_id предыдущего блока.
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
| INDEX_VERSION | 2 | Версия формата |
| META_SIZE | 128 байт | Размер index.meta |
| LEXICON_HEADER_SIZE | 16 байт | Размер заголовка lexicon |
| TERM_ENTRY_SIZE | 56 | Размер одной записи терма (v2) |
| POSTINGS_HEADER_SIZE | 12 байт | Размер заголовка postings |

---

## 6. Алгоритмы

### 6.1 Инкрементальное добавление документа (IndexWriter)

1. Вычислить `book_id = generateBookId(file_path, title, author)`
2. `INSERT OR IGNORE INTO books` — дедупликация
3. Токенизировать + стеммить текст чанка
4. `INSERT INTO chunks` → получить `doc_id`
5. Для каждого уникального терма:
   - Найти терм в `lexicon_buffer` (по тексту)
   - Если не найден — создать новую запись с пустым списком блоков
   - Сериализовать **один** Posting как новый блок (`doc_count=1` для этого блока)
   - Дописать блок в конец `postings.idx`
   - Добавить `BlockInfo{offset, size}` в `lexicon_buffer[term].blocks`
   - Увеличить `df` и `total_postings_size`
6. Обновить `index.meta`

### 6.2 Поиск (IndexReader)

1. Токенизировать + стеммить запрос
2. Для каждого терма запроса:
   - Найти терм в lexicon (бинарный поиск по `term_hash`)
   - Получить `block_count` и `blocks_offset`
   - Прочитать массив `BlockInfo` из секции D
   - Для каждого блока прочитать данные из `postings.idx` по `offset` и `size`
   - Декодировать все блоки последовательно (varint + дельта), передавая `prev_doc_id` между блоками
3. Пересечь списки doc_id (AND) или объединить (OR)
4. Для каждого документа вычислить BM25 score
5. Сортировать по score, вернуть top-K
6. По `doc_id` получить метаданные из SQLite с JOIN по `books.id = chunks.book_id`
7. Применить фильтры (жанр, автор)

### 6.3 Дефрагментация при сохранении (flushLexicon)

При вызове `flushLexicon()` (закрытие IndexWriter) блоки не дефрагментируются — они сохраняются как есть. Это обеспечивает быстрое закрытие. Дефрагментация (слияние всех блоков терма в один непрерывный) может быть выполнена отдельной операцией `compact()` в будущем.

### 6.4 Дедупликация книг при переиндексации

1. Вычислить `book_id` детерминированно
2. `INSERT OR IGNORE INTO books` — если книга уже существует, вставка игнорируется
3. Сравнить `file_path` и `created_at`: если файл новее — обновить метаданные книги и переиндексировать все чанки

---

## 7. Версионирование

При изменении формата:
1. Увеличить `INDEX_VERSION`
2. Добавить код миграции в IndexReader
3. Обновить этот документ

**История версий:**
- v1 (2026-07-17): Исходный формат, TERM_ENTRY_SIZE=40, один блок на терм
- v2 (2026-07-17): TERM_ENTRY_SIZE=56, поддержка множества блоков на терм для инкрементального добавления
