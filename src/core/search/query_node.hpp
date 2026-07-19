#pragma once

#include <string>
#include <vector>
#include <memory>

namespace lse
{

    enum class QueryOp
    {
        And,   // все термы должны присутствовать
        Or,    // хотя бы один терм
        Not,   // исключить терм
        Phrase // термы идут подряд (с возможными альтернативами)
    };

    // Одна позиция во фразе: либо одно слово, либо группа альтернатив
    struct PhrasePosition
    {
        std::vector<std::string> alternatives; // слова-альтернативы (1 элемент = обычное слово)
    };

    struct QueryNode
    {
        QueryOp op = QueryOp::And;
        std::vector<std::string> terms;               // термы для AND/OR
        std::vector<QueryNode> children;              // вложенные узлы (для NOT, фраз)
        std::vector<PhrasePosition> phrase_positions; // позиции для Phrase
        bool negated = false;

        bool isLeaf() const { return children.empty() && phrase_positions.empty(); }
    };

} // namespace lse