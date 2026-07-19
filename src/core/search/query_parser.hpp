#pragma once

#include "query_node.hpp"
#include "query_type.hpp"
#include <string_view>
#include <expected>

namespace lse
{

    enum class ParseError
    {
        EmptyQuery,
        UnmatchedQuotes,
        UnexpectedCharacter,
        EmptyGroup
    };

    class QueryParser
    {
    public:
        auto parse(std::string_view query, QueryType type = QueryType::QueryString)
            -> std::expected<QueryNode, ParseError>;

    private:
        std::string_view query_;
        size_t pos_ = 0;
        QueryType type_ = QueryType::QueryString;

        // expression = or_expr
        auto parseOrExpression() -> std::expected<QueryNode, ParseError>;

        // or_expr = and_expr ('|' and_expr)*
        auto parseAndExpression() -> std::expected<QueryNode, ParseError>;

        // and_expr = factor (factor)*  (AND по умолчанию)

        // factor = term | phrase | NOT factor | '(' expression ')'
        auto parseFactor() -> std::expected<QueryNode, ParseError>;

        auto parseTerm() -> std::expected<std::string, ParseError>;
        auto parsePhrase() -> std::expected<QueryNode, ParseError>;
        auto parsePhraseGroup() -> std::expected<PhrasePosition, ParseError>;

        void skipWhitespace();
        bool eof() const { return pos_ >= query_.size(); }
        char peek() const { return eof() ? '\0' : query_[pos_]; }
        char advance() { return eof() ? '\0' : query_[pos_++]; }
    };

} // namespace lse