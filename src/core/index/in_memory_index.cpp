#include "in_memory_index.hpp"
#include "../tokenizer/tokenizer.hpp" // для Token
#include <algorithm>
#include <cmath>

namespace lse
{

    auto InMemoryIndex::addDocument(const std::vector<Token> &tokens) -> uint64_t
    {
        uint64_t doc_id = doc_count_++;
        uint32_t doc_length = static_cast<uint32_t>(tokens.size());

        // Подсчитываем term frequency для каждого токена в этом документе
        std::unordered_map<std::string, std::pair<uint32_t, std::vector<uint32_t>>> term_stats;

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            const auto &token = tokens[i];
            auto &[tf, positions] = term_stats[token.text];
            tf++;
            positions.push_back(static_cast<uint32_t>(i));
        }

        // Добавляем в индекс
        for (auto &[term, stats] : term_stats)
        {
            auto &[tf, positions] = stats;
            auto &info = index_[term];

            info.postings.push_back(Posting{
                .doc_id = doc_id,
                .term_freq = tf,
                .positions = std::move(positions)});
            info.df++;
            info.sorted = false; // нарушили сортировку
        }

        total_term_count_ += doc_length;
        avg_doc_length_ = static_cast<double>(total_term_count_) / doc_count_;

        return doc_id;
    }

    void InMemoryIndex::prepareSearch()
    {
        for (auto &[term, info] : index_)
        {
            if (!info.sorted)
            {
                std::sort(info.postings.begin(), info.postings.end());
                info.sorted = true;
            }
        }
    }

    auto InMemoryIndex::search(const std::vector<std::string> &query_tokens, size_t top_k)
        -> std::expected<std::vector<SearchHit>, IndexError>
    {

        if (query_tokens.empty())
        {
            return std::unexpected(IndexError::EmptyQuery);
        }

        if (index_.empty())
        {
            return std::vector<SearchHit>{};
        }

        // Убедимся, что индекс отсортирован
        prepareSearch();

        // Накопление BM25 score для каждого документа
        std::unordered_map<uint64_t, double> doc_scores;

        // Значение IDF для каждого терма запроса (одинаково для всех документов)
        std::unordered_map<std::string, double> term_idfs;

        for (const auto &query_term : query_tokens)
        {
            auto it = index_.find(query_term);
            if (it == index_.end())
            {
                continue; // терма нет в индексе
            }

            const auto &info = it->second;
            double idf = calculateIDF(info);
            term_idfs[query_term] = idf;

            // Проходим по всем документам, содержащим этот терм
            for (const auto &posting : info.postings)
            {
                // BM25 для пары (терм, документ)
                double score = calculateBM25Score(
                    posting.term_freq,
                    static_cast<uint32_t>(posting.positions.size()), // было: posting.positions.size()
                    idf,
                    avg_doc_length_);

                doc_scores[posting.doc_id] += score;
            }
        }

        // Сортируем по score и берём top_k
        std::vector<SearchHit> results;
        results.reserve(doc_scores.size());

        for (const auto &[doc_id, score] : doc_scores)
        {
            results.push_back(SearchHit{doc_id, static_cast<float>(score)});
        }

        std::sort(results.begin(), results.end(),
                  [](const SearchHit &a, const SearchHit &b)
                  {
                      return a.bm25_score > b.bm25_score;
                  });

        if (results.size() > top_k)
        {
            results.resize(top_k);
        }

        return results;
    }

    double InMemoryIndex::calculateIDF(const TermInfo &info) const
    {
        if (info.df == 0)
            return 0.0;
        return std::log(static_cast<double>(doc_count_ - info.df + 0.5) /
                            (info.df + 0.5) +
                        1.0);
    }

    double InMemoryIndex::calculateBM25Score(uint32_t tf, uint32_t doc_length,
                                             double idf, double avg_dl) const
    {
        double numerator = tf * (k1 + 1.0);
        double denominator = tf + k1 * (1.0 - b_param + b_param * (doc_length / avg_dl));
        return idf * (numerator / denominator);
    }

} // namespace lse