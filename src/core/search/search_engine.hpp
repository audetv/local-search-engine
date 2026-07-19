#pragma once

#include "../index/index_reader.hpp"
#include "../tokenizer/stemmer.hpp"
#include "query_node.hpp"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <expected>

namespace lse
{

    class SearchEngine
    {
    public:
        SearchEngine(IndexReader &reader, Stemmer &stemmer);

        // Выполняет QueryNode
        auto execute(const QueryNode &query,
                     size_t top_k = 20,
                     const std::string &genre_filter = "",
                     const std::string &author_filter = "",
                     const std::string &title_filter = "",
                     const std::vector<std::string> &highlight_terms = {})
            -> std::expected<std::vector<std::unique_ptr<SearchHit>>, IndexError>;

        // Все документы (без полнотекстового поиска)
        auto matchAll(size_t top_k = 20,
                      const std::string &genre_filter = "",
                      const std::string &author_filter = "",
                      const std::string &title_filter = "")
            -> std::expected<std::vector<std::unique_ptr<SearchHit>>, IndexError>;

    private:
        IndexReader &reader_;
        Stemmer &stemmer_;

        auto evaluateNode(const QueryNode &node,
                          std::unordered_map<uint64_t, float> &doc_scores) const
            -> std::expected<void, IndexError>;

        // Стеммирует терм
        std::string stemTerm(const std::string &term) const;
    };

} // namespace lse