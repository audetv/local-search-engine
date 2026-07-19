#pragma once

#include <string>
#include <vector>

namespace lse
{

    enum class QueryOp
    {
        And,    // все children должны присутствовать
        Or,     // хотя бы один child
        Not,    // исключить child
        Phrase, // phrase_positions
        Term    // листовой узел: одно слово
    };

    // Одна позиция во фразе: либо одно слово, либо группа альтернатив
    struct PhrasePosition
    {
        std::vector<std::string> alternatives;
    };

    struct QueryNode
    {
        QueryOp op = QueryOp::And;
        std::string term;                             // для Term
        std::vector<QueryNode> children;              // для And/Or/Not
        std::vector<PhrasePosition> phrase_positions; // для Phrase
        bool negated = false;                         // для Not

        // Фабрики для удобства
        static QueryNode makeTerm(const std::string &t)
        {
            QueryNode n;
            n.op = QueryOp::Term;
            n.term = t;
            return n;
        }

        static QueryNode makeAnd(std::vector<QueryNode> children)
        {
            QueryNode n;
            n.op = QueryOp::And;
            n.children = std::move(children);
            return n;
        }

        static QueryNode makeOr(std::vector<QueryNode> children)
        {
            QueryNode n;
            n.op = QueryOp::Or;
            n.children = std::move(children);
            return n;
        }

        static QueryNode makeNot(QueryNode child)
        {
            QueryNode n;
            n.op = QueryOp::Not;
            n.children.push_back(std::move(child));
            return n;
        }

        static QueryNode makePhrase(std::vector<PhrasePosition> positions)
        {
            QueryNode n;
            n.op = QueryOp::Phrase;
            n.phrase_positions = std::move(positions);
            return n;
        }
    };

} // namespace lse