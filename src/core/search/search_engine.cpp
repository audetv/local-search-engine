#include "search_engine.hpp"
#include <sqlite3.h>
#include <algorithm>
#include <clocale>
#include <cctype>

namespace lse
{

    SearchEngine::SearchEngine(IndexReader &reader, Stemmer &stemmer)
        : reader_(reader), stemmer_(stemmer) {}

    auto SearchEngine::execute(const QueryNode &query,
                               size_t top_k,
                               const std::string &genre_filter,
                               const std::string &author_filter,
                               const std::string &title_filter,
                               const std::vector<std::string> &highlight_terms)
        -> std::expected<std::vector<std::unique_ptr<SearchHit>>, IndexError>
    {

        std::unordered_map<uint64_t, float> doc_scores;

        auto result = evaluateNode(query, doc_scores);
        if (!result.has_value())
            return std::unexpected(result.error());

        if (doc_scores.empty())
        {
            return std::vector<std::unique_ptr<SearchHit>>{};
        }

        // Сортируем
        std::vector<std::pair<uint64_t, float>> scored_docs(doc_scores.begin(), doc_scores.end());
        std::sort(scored_docs.begin(), scored_docs.end(),
                  [](const auto &a, const auto &b)
                  { return a.second > b.second; });

        if (scored_docs.size() > top_k)
        {
            scored_docs.resize(top_k);
        }

        // Метаданные через IndexReader
        std::vector<std::unique_ptr<SearchHit>> results;
        results.reserve(scored_docs.size());

        for (const auto &[doc_id, score] : scored_docs)
        {
            auto meta = reader_.getChunkMetadata(doc_id);
            if (!meta.has_value())
                continue;

            auto hit = std::move(*meta);
            hit->bm25_score = score;

            if (!title_filter.empty() && hit->title.find(title_filter) == std::string::npos)
                continue;
            if (!genre_filter.empty() && hit->genre != genre_filter)
                continue;
            if (!author_filter.empty() && hit->author != author_filter)
                continue;

            // Подсветка
            if (!highlight_terms.empty() && !hit->content.empty())
            {
                std::string content_lower = hit->content;
                std::transform(content_lower.begin(), content_lower.end(), content_lower.begin(), ::tolower);

                for (const auto &hterm : highlight_terms)
                {
                    std::string term_lower = hterm;
                    std::transform(term_lower.begin(), term_lower.end(), term_lower.begin(), ::tolower);
                    if (term_lower.empty())
                        continue;

                    size_t pos = 0;
                    while ((pos = content_lower.find(term_lower, pos)) != std::string::npos)
                    {
                        bool valid = true;
                        if (pos > 0 && std::isalnum(static_cast<unsigned char>(content_lower[pos - 1])))
                            valid = false;
                        size_t end_pos = pos + term_lower.size();
                        if (end_pos < content_lower.size() && std::isalnum(static_cast<unsigned char>(content_lower[end_pos])))
                            valid = false;

                        if (valid)
                        {
                            hit->highlights.push_back({pos, term_lower.size()});
                        }
                        pos += term_lower.size();
                    }
                }
            }

            results.push_back(std::move(hit));
        }

        return results;
    }

    auto SearchEngine::matchAll([[maybe_unused]] size_t top_k,
                                [[maybe_unused]] const std::string &genre_filter,
                                [[maybe_unused]] const std::string &author_filter,
                                [[maybe_unused]] const std::string &title_filter)
        -> std::expected<std::vector<std::unique_ptr<SearchHit>>, IndexError>
    {

        std::vector<std::unique_ptr<SearchHit>> results;

        // Проходим по всем doc_id через getChunkMetadata
        // Но у нас нет метода для перебора всех doc_id.
        // Пока используем запрос к SQLite напрямую через reader_
        // Добавим метод getAllDocIds в IndexReader позже

        // Временная реализация: возвращаем пустой список
        // TODO: реализовать через IndexReader::getAllDocIds()
        return std::vector<std::unique_ptr<SearchHit>>{};
    }

    auto SearchEngine::evaluateNode(const QueryNode &node,
                                    std::unordered_map<uint64_t, float> &doc_scores) const
        -> std::expected<void, IndexError>
    {

        switch (node.op)
        {
        case QueryOp::Term:
        {
            std::string stemmed = stemTerm(node.term);
            auto postings_result = reader_.getTermPostings(stemmed);
            if (!postings_result.has_value())
                return std::unexpected(postings_result.error());

            double idf = reader_.getTermIDF(stemmed);
            if (idf == 0.0)
                break;

            for (const auto &p : *postings_result)
            {
                doc_scores[p.doc_id] += static_cast<float>(
                    reader_.calculateBM25(p.term_freq, static_cast<uint32_t>(p.positions.size()), idf));
            }
            break;
        }

        case QueryOp::And:
        {
            std::unordered_map<uint64_t, float> combined;
            bool first = true;

            for (const auto &child : node.children)
            {
                if (child.op == QueryOp::Not)
                {
                    // Исключаем документы
                    std::unordered_map<uint64_t, float> excluded;
                    auto res = evaluateNode(child.children[0], excluded);
                    if (!res.has_value())
                        return res;
                    for (const auto &[doc_id, _] : excluded)
                    {
                        combined.erase(doc_id);
                    }
                }
                else
                {
                    std::unordered_map<uint64_t, float> child_scores;
                    auto res = evaluateNode(child, child_scores);
                    if (!res.has_value())
                        return res;

                    if (first)
                    {
                        combined = std::move(child_scores);
                        first = false;
                    }
                    else
                    {
                        std::unordered_map<uint64_t, float> intersected;
                        for (const auto &[doc_id, score] : child_scores)
                        {
                            auto it = combined.find(doc_id);
                            if (it != combined.end())
                            {
                                intersected[doc_id] = it->second + score;
                            }
                        }
                        combined = std::move(intersected);
                    }
                }
                if (combined.empty() && first == false)
                    break;
            }

            for (auto &[doc_id, score] : combined)
            {
                doc_scores[doc_id] += score;
            }
            break;
        }

        case QueryOp::Or:
        {
            for (const auto &child : node.children)
            {
                auto res = evaluateNode(child, doc_scores);
                if (!res.has_value())
                    return res;
            }
            break;
        }

        case QueryOp::Not:
        {
            if (node.children.empty())
                break;

            std::unordered_map<uint64_t, float> excluded;
            auto res = evaluateNode(node.children[0], excluded);
            if (!res.has_value())
                return res;

            for (const auto &[doc_id, _] : excluded)
            {
                doc_scores.erase(doc_id);
            }
            break;
        }

        case QueryOp::Phrase:
        {
            if (node.phrase_positions.empty())
                break;

            // Собираем кандидатов по первой позиции
            std::unordered_map<uint64_t, float> candidates;

            for (const auto &alt : node.phrase_positions[0].alternatives)
            {
                std::string stemmed = stemTerm(alt);
                auto postings_result = reader_.getTermPostings(stemmed);
                if (!postings_result.has_value())
                    continue;

                double idf = reader_.getTermIDF(stemmed);
                for (const auto &p : *postings_result)
                {
                    float score = static_cast<float>(
                        reader_.calculateBM25(p.term_freq, static_cast<uint32_t>(p.positions.size()), idf));
                    candidates[p.doc_id] += score;
                }
            }

            if (candidates.empty())
                break;

            // Для каждого кандидата проверяем последовательность позиций
            for (const auto &[doc_id, first_score] : candidates)
            {
                bool phrase_match = false;

                std::string first_stemmed = stemTerm(node.phrase_positions[0].alternatives[0]);
                auto first_postings = reader_.getTermPostings(first_stemmed);
                if (!first_postings.has_value())
                    continue;

                for (const auto &fp : *first_postings)
                {
                    if (fp.doc_id != doc_id)
                        continue;

                    for (uint32_t start_pos : fp.positions)
                    {
                        bool all_match = true;

                        // Проверяем остальные позиции фразы
                        for (size_t i = 1; i < node.phrase_positions.size(); ++i)
                        {
                            uint32_t expected_pos = start_pos + static_cast<uint32_t>(i);
                            bool pos_matched = false;

                            for (const auto &alt : node.phrase_positions[i].alternatives)
                            {
                                std::string alt_stemmed = stemTerm(alt);
                                auto alt_postings = reader_.getTermPostings(alt_stemmed);
                                if (!alt_postings.has_value())
                                    continue;

                                for (const auto &ap : *alt_postings)
                                {
                                    if (ap.doc_id != doc_id)
                                        continue;
                                    if (std::find(ap.positions.begin(), ap.positions.end(), expected_pos) != ap.positions.end())
                                    {
                                        pos_matched = true;
                                        break;
                                    }
                                }
                                if (pos_matched)
                                    break;
                            }

                            if (!pos_matched)
                            {
                                all_match = false;
                                break;
                            }
                        }

                        if (all_match)
                        {
                            phrase_match = true;
                            break;
                        }
                    }
                    if (phrase_match)
                        break;
                }

                if (phrase_match)
                {
                    doc_scores[doc_id] += first_score;
                }
            }
            break;
        }
        }

        return {};
    }

    static std::string to_lower_utf8(const std::string &s)
    {
        std::string result;
        result.reserve(s.size());
        for (size_t i = 0; i < s.size();)
        {
            unsigned char c = s[i];
            // Кириллица А-Я (0xD0 0x90-0xAF) → а-я (0xD0 0xB0-0xBF)
            if (c == 0xD0 && i + 1 < s.size())
            {
                unsigned char next = s[i + 1];
                if (next >= 0x90 && next <= 0xAF)
                {
                    result += char(0xD0);
                    result += char(next + 0x20);
                    i += 2;
                    continue;
                }
                if (next == 0x81)
                { // Ё
                    result += char(0xD1);
                    result += char(0x91);
                    i += 2;
                    continue;
                }
            }
            // ASCII верхний регистр
            if (c >= 'A' && c <= 'Z')
            {
                result += char(c + ('a' - 'A'));
                i++;
                continue;
            }
            result += s[i];
            i++;
        }
        return result;
    }

    std::string SearchEngine::stemTerm(const std::string &term) const
    {
        std::string lower = to_lower_utf8(term);
        return std::string(stemmer_.stem(lower));
    }
} // namespace lse