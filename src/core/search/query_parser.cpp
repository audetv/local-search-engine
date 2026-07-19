#include "query_parser.hpp"
#include <cctype>
#include <algorithm>

namespace lse
{

    auto QueryParser::parse(std::string_view query, [[maybe_unused]] bool is_query_string)
        -> std::expected<QueryNode, ParseError>
    {

        query_ = query;
        pos_ = 0;

        if (query.empty())
        {
            return std::unexpected(ParseError::EmptyQuery);
        }

        auto result = parseExpression();
        if (!result.has_value())
            return result;

        skipWhitespace();
        if (!eof())
        {
            return std::unexpected(ParseError::UnexpectedCharacter);
        }

        return result;
    }

    auto QueryParser::parseExpression() -> std::expected<QueryNode, ParseError>
    {
        QueryNode node;
        node.op = QueryOp::And;

        while (!eof())
        {
            skipWhitespace();
            if (eof())
                break;

            char c = peek();

            // OR
            if (c == '|')
            {
                advance();
                node.op = QueryOp::Or;
                continue;
            }

            // NOT
            if (c == '-' || c == '!')
            {
                advance();
                skipWhitespace();

                std::string term;
                if (peek() == '"')
                {
                    auto result = parsePhrase();
                    if (!result.has_value())
                        return std::unexpected(result.error());
                    term = *result;
                }
                else
                {
                    auto result = parseTerm();
                    if (!result.has_value())
                        return std::unexpected(result.error());
                    term = *result;
                }

                if (!term.empty())
                {
                    QueryNode notNode;
                    notNode.op = QueryOp::And;
                    notNode.terms.push_back(term);
                    notNode.negated = true;
                    node.children.push_back(std::move(notNode));
                }
                continue;
            }

            // Phrase
            if (c == '"')
            {
                auto result = parsePhrase();
                if (!result.has_value())
                    return std::unexpected(result.error());

                QueryNode phraseNode;
                phraseNode.op = QueryOp::Phrase;
                phraseNode.terms.push_back(*result);
                node.children.push_back(std::move(phraseNode));
                continue;
            }

            // Term
            auto result = parseTerm();
            if (!result.has_value())
                return std::unexpected(result.error());

            if (!result->empty())
            {
                node.terms.push_back(*result);
            }
        }

        return node;
    }

    auto QueryParser::parseTerm() -> std::expected<std::string, ParseError>
    {
        skipWhitespace();
        if (eof())
            return std::string{};

        std::string term;
        while (!eof())
        {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                break;
            if (c == '|' || c == '-' || c == '!' || c == '"')
                break;
            term += advance();
        }

        return term;
    }

    auto QueryParser::parsePhrase() -> std::expected<std::string, ParseError>
    {
        if (peek() != '"')
            return std::unexpected(ParseError::UnmatchedQuotes);
        advance();

        std::string phrase;
        while (!eof() && peek() != '"')
        {
            phrase += advance();
        }

        if (eof())
            return std::unexpected(ParseError::UnmatchedQuotes);
        advance();

        return phrase;
    }

    void QueryParser::skipWhitespace()
    {
        while (!eof() && (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r'))
        {
            advance();
        }
    }

} // namespace lse