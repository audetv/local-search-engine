#pragma once

#include "tokenizer/tokenizer.hpp" // для Token
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include <expected>

namespace lse
{

    // Одна запись в постинг-листе
    struct Posting
    {
        uint64_t doc_id;                 // идентификатор документа
        uint32_t term_freq;              // частота терма в этом документе
        std::vector<uint32_t> positions; // позиции терма в документе (для proximity)

        // Для сортировки
        bool operator<(const Posting &other) const
        {
            return doc_id < other.doc_id;
        }
    };

    // Информация о терме в индексе
    struct TermInfo
    {
        std::vector<Posting> postings; // список документов с этим термом
        uint32_t df = 0;               // document frequency (в скольких документах)
        bool sorted = true;            // отсортирован ли постинг-лист
    };

    // Результат поиска
    struct SearchHit
    {
        uint64_t doc_id;  // идентификатор документа
        float bm25_score; // BM25 релевантность
        // TODO: добавить proximity_score позже
    };

    enum class IndexError
    {
        DocumentNotFound,
        EmptyQuery
    };

    class InMemoryIndex
    {
    public:
        InMemoryIndex() = default;

        // Добавить документ в индекс.
        // tokens — токенизированные и стеммированные слова с позициями.
        // Возвращает doc_id нового документа.
        auto addDocument(const std::vector<Token> &tokens) -> uint64_t;

        // Подготовить индекс к поиску (сортирует постинг-листы)
        void prepareSearch();

        // Выполнить поиск. Возвращает список doc_id с BM25 score.
        // query_tokens — токенизированные и стеммированные слова запроса.
        auto search(const std::vector<std::string> &query_tokens, size_t top_k = 20)
            -> std::expected<std::vector<SearchHit>, IndexError>;

        // Получить количество документов
        uint64_t docCount() const { return doc_count_; }

        // Получить среднюю длину документа (в токенах)
        double avgDocLength() const { return avg_doc_length_; }

    private:
        // Лексикон: терм → информация о терме
        std::unordered_map<std::string, TermInfo> index_;

        uint64_t doc_count_ = 0;        // всего документов
        uint64_t total_term_count_ = 0; // всего токенов во всех документах
        double avg_doc_length_ = 0.0;   // средняя длина документа

        // Получить IDF (Inverse Document Frequency) для терма
        double calculateIDF(const TermInfo &info) const;

        // Рассчитать BM25 для одного документа по одному терму
        double calculateBM25Score(uint32_t tf, uint32_t doc_length,
                                  double idf, double avg_dl) const;

        // Константы BM25
        static constexpr double k1 = 1.2; // term frequency saturation
        static constexpr double b = 0.75; // length normalization
    };

} // namespace lse