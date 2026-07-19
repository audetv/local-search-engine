#pragma once

#include "query_node.hpp"
#include "query_parser.hpp"
#include <string>
#include <expected>

namespace lse
{

    class QueryBuilder
    {
    public:
        // AND по умолчанию, операторы включены
        static auto queryString(const std::string &query)
            -> std::expected<QueryNode, ParseError>;

        // OR по умолчанию, операторы как текст
        static auto match(const std::string &query)
            -> std::expected<QueryNode, ParseError>;

        // Весь запрос как фраза
        static auto matchPhrase(const std::string &query)
            -> std::expected<QueryNode, ParseError>;

        // Все документы (возвращает пустой And-узел)
        static auto matchAll() -> QueryNode;
    };

} // namespace lse