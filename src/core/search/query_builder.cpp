#include "query_builder.hpp"
#include <algorithm>

namespace lse
{

    auto QueryBuilder::queryString(const std::string &query)
        -> std::expected<QueryNode, ParseError>
    {
        QueryParser parser;
        return parser.parse(query, true);
    }

    auto QueryBuilder::match(const std::string &query)
        -> std::expected<QueryNode, ParseError>
    {
        QueryParser parser;
        return parser.parse(query, false);
    }

    auto QueryBuilder::matchPhrase(const std::string &query)
        -> std::expected<QueryNode, ParseError>
    {
        QueryParser parser;
        // Оборачиваем в кавычки для фразового поиска
        std::string quoted = "\"" + query + "\"";
        return parser.parse(quoted, true);
    }

    auto QueryBuilder::matchAll() -> QueryNode
    {
        return QueryNode::makeAnd({});
    }

} // namespace lse