#pragma once

#include "query_node.hpp"
#include <string_view>
#include <expected>

namespace lse
{

    enum class ParseError
    {
        EmptyQuery,
        UnmatchedQuotes,
        UnexpectedCharacter
    };

    class QueryParser
    {
    public:
        // Парсит строку в QueryNode.
        // is_query_string: true → AND по умолчанию + операторы
        //                 false → OR по умолчанию, операторы как текст
        auto parse(std::string_view query, [[maybe_unused]] bool is_query_string = true)
            -> std::expected<QueryNode, ParseError>;

    private:
        std::string_view query_;
        size_t pos_ = 0;

        auto parseExpression() -> std::expected<QueryNode, ParseError>;
        auto parseTerm() -> std::expected<std::string, ParseError>;
        auto parsePhrase() -> std::expected<std::string, ParseError>;

        bool isOperator(char c) const;
        void skipWhitespace();
        bool eof() const { return pos_ >= query_.size(); }
        char peek() const { return eof() ? '\0' : query_[pos_]; }
        char advance() { return eof() ? '\0' : query_[pos_++]; }
    };

} // namespace lse