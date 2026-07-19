#include "query_parser.hpp"

namespace lse
{

    auto QueryParser::parse(std::string_view query, bool is_query_string)
        -> std::expected<QueryNode, ParseError>
    {

        query_ = query;
        pos_ = 0;
        is_query_string_ = is_query_string;

        if (query.empty())
        {
            return std::unexpected(ParseError::EmptyQuery);
        }

        auto result = parseOrExpression();
        if (!result.has_value())
            return result;

        skipWhitespace();
        if (!eof())
        {
            return std::unexpected(ParseError::UnexpectedCharacter);
        }

        return result;
    }

    // or_expr = and_expr ('|' and_expr)*
    auto QueryParser::parseOrExpression() -> std::expected<QueryNode, ParseError>
    {
        auto left = parseAndExpression();
        if (!left.has_value())
            return left;

        std::vector<QueryNode> children;
        children.push_back(std::move(*left));

        while (!eof())
        {
            skipWhitespace();
            if (eof())
                break;

            if (peek() == '|')
            {
                advance();
                auto right = parseAndExpression();
                if (!right.has_value())
                    return right;
                children.push_back(std::move(*right));
            }
            else
            {
                break;
            }
        }

        if (children.size() == 1)
        {
            return children[0];
        }

        return QueryNode::makeOr(std::move(children));
    }

    // and_expr = factor+
    auto QueryParser::parseAndExpression() -> std::expected<QueryNode, ParseError>
    {
        std::vector<QueryNode> children;

        auto first = parseFactor();
        if (!first.has_value())
            return first;
        children.push_back(std::move(*first));

        while (!eof())
        {
            skipWhitespace();
            if (eof())
                break;

            char c = peek();
            // Если не оператор OR и не закрывающая скобка — продолжаем AND
            if (c == '|' || c == ')')
                break;

            auto factor = parseFactor();
            if (!factor.has_value())
                return factor;
            children.push_back(std::move(*factor));
        }

        if (children.size() == 1)
        {
            return children[0];
        }

        return QueryNode::makeAnd(std::move(children));
    }

    // factor = term | phrase | NOT factor | '(' expression ')'
    auto QueryParser::parseFactor() -> std::expected<QueryNode, ParseError>
    {
        skipWhitespace();
        if (eof())
            return std::unexpected(ParseError::EmptyQuery);

        char c = peek();

        // NOT
        if (c == '-' || c == '!')
        {
            advance();
            skipWhitespace();
            auto factor = parseFactor();
            if (!factor.has_value())
                return factor;
            return QueryNode::makeNot(std::move(*factor));
        }

        // Phrase
        if (c == '"')
        {
            return parsePhrase();
        }

        // Группировка скобками
        if (c == '(')
        {
            advance(); // '('
            auto expr = parseOrExpression();
            if (!expr.has_value())
                return expr;

            skipWhitespace();
            if (peek() != ')')
            {
                return std::unexpected(ParseError::UnexpectedCharacter);
            }
            advance(); // ')'
            return expr;
        }

        // Term
        auto term = parseTerm();
        if (!term.has_value())
            return std::unexpected(term.error());
        if (term->empty())
            return std::unexpected(ParseError::EmptyQuery);
        return QueryNode::makeTerm(*term);
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

        std::vector<PhrasePosition> positions;

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
                positions.push_back(*result);
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
                    positions.push_back(pos);
                }
            }
        }

        if (eof())
            return std::unexpected(ParseError::UnmatchedQuotes);
        advance();

        if (positions.empty())
        {
            return std::unexpected(ParseError::EmptyQuery);
        }

        return QueryNode::makePhrase(std::move(positions));
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