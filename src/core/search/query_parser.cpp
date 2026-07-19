#include "query_parser.hpp"
#include <cctype>

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

                if (peek() == '"')
                {
                    auto result = parsePhrase();
                    if (!result.has_value())
                        return std::unexpected(result.error());
                    result->negated = true;
                    node.children.push_back(std::move(*result));
                }
                else
                {
                    auto result = parseTerm();
                    if (!result.has_value())
                        return std::unexpected(result.error());
                    if (!result->empty())
                    {
                        QueryNode notNode;
                        notNode.op = QueryOp::And;
                        notNode.terms.push_back(*result);
                        notNode.negated = true;
                        node.children.push_back(std::move(notNode));
                    }
                }
                continue;
            }

            // Phrase
            if (c == '"')
            {
                auto result = parsePhrase();
                if (!result.has_value())
                    return std::unexpected(result.error());
                node.children.push_back(std::move(*result));
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
            if (c == '|' || c == '-' || c == '!' || c == '"' || c == '(' || c == ')')
                break;
            term += advance();
        }

        return term;
    }

    auto QueryParser::parsePhrase() -> std::expected<QueryNode, ParseError>
    {
        if (peek() != '"')
            return std::unexpected(ParseError::UnmatchedQuotes);
        advance();

        QueryNode phraseNode;
        phraseNode.op = QueryOp::Phrase;

        while (!eof() && peek() != '"')
        {
            skipWhitespace();
            if (eof() || peek() == '"')
                break;

            if (peek() == '(')
            {
                auto result = parsePhraseGroup();
                if (!result.has_value())
                    return std::unexpected(result.error());
                phraseNode.phrase_positions.push_back(*result);
            }
            else
            {
                auto result = parseTerm();
                if (!result.has_value())
                    return std::unexpected(result.error());
                if (!result->empty())
                {
                    PhrasePosition pos;
                    pos.alternatives.push_back(*result);
                    phraseNode.phrase_positions.push_back(pos);
                }
            }
        }

        if (eof())
            return std::unexpected(ParseError::UnmatchedQuotes);
        advance();

        if (phraseNode.phrase_positions.empty())
        {
            return std::unexpected(ParseError::EmptyQuery);
        }

        return phraseNode;
    }

    auto QueryParser::parsePhraseGroup() -> std::expected<PhrasePosition, ParseError>
    {
        if (peek() != '(')
            return std::unexpected(ParseError::UnexpectedCharacter);
        advance();

        PhrasePosition pos;

        while (!eof() && peek() != ')')
        {
            skipWhitespace();
            if (eof() || peek() == ')')
                break;

            auto term = parseTerm();
            if (!term.has_value())
                return std::unexpected(term.error());
            if (!term->empty())
            {
                pos.alternatives.push_back(*term);
            }

            skipWhitespace();
            if (peek() == '|')
            {
                advance();
            }
            else if (peek() != ')')
            {
                return std::unexpected(ParseError::UnexpectedCharacter);
            }
        }

        if (eof() || peek() != ')')
        {
            return std::unexpected(ParseError::UnmatchedQuotes);
        }
        advance();

        if (pos.alternatives.empty())
        {
            return std::unexpected(ParseError::EmptyGroup);
        }

        return pos;
    }

    void QueryParser::skipWhitespace()
    {
        while (!eof() && (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r'))
        {
            advance();
        }
    }

} // namespace lse