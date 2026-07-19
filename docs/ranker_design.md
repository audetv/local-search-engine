# Дизайн ранкеров

Дата: 2026-07-19  
Статус: Проектирование (v1)

## Текущее состояние (MVP)

- Локальный поиск: только BM25
- Удалённый поиск: proximity_bm25 (жёстко в ManticoreClient)
- UI: без настройки ранкера

## Целевое состояние (v2)

Поддержка нескольких ранкеров с возможностью переключения в UI.

### Ранкеры для локального поиска

| Ранкер | Статус |
|--------|--------|
| BM25 | ✅ Реализован |
| ProximityBM25 | ⬜ Планируется |
| WordCount | ⬜ Опционально |
| None (boolean) | ⬜ Опционально |

### API

```cpp
enum class Ranker {
    BM25,
    ProximityBM25,
    WordCount,
    None
};

// SearchEngine
auto execute(const QueryNode& query, Ranker ranker = Ranker::BM25, ...) -> ...;

// ManticoreClient
auto search(const std::string& query, 
            const std::string& query_type = "query_string",
            Ranker ranker = Ranker::ProximityBM25, ...) -> ...;
```

### ProximityBM25 (локальный)

Формула: `sum(lcs * user_weight) * 1000 + bm25`

Где `lcs` — длина наибольшей общей подпоследовательности термов запроса в документе. Вычисляется на основе позиций токенов из `postings.idx`.

Потребуется:
- `ProximityBm25Ranker` — утилита, принимает позиции термов и вычисляет LCS
- Интеграция в `SearchEngine::evaluateNode` для `QueryOp::And` (документы где все термы совпали получают proximity bonus)

### UI

Переключатель ранкера в строке поиска (выпадающий список или radio buttons). При переключении поиск выполняется заново с новым ранкером.

### Маппинг ранкеров для Manticore

```cpp
std::string rankerToString(Ranker r) {
    switch (r) {
        case Ranker::BM25: return "bm25";
        case Ranker::ProximityBM25: return "proximity_bm25";
        case Ranker::WordCount: return "wordcount";
        case Ranker::None: return "none";
    }
}
```

## Реализация

1. Добавить `enum class Ranker` в `search/ranker.hpp`
2. Добавить параметр в `SearchEngine::execute()` и `ManticoreClient::search()`
3. Реализовать `ProximityBm25Ranker`
4. Добавить UI-переключатель
```
