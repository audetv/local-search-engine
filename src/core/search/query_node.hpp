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
        Phrase // термы идут подряд
    };

    struct QueryNode
    {
        QueryOp op = QueryOp::And;
        std::vector<std::string> terms;  // термы для листового узла
        std::vector<QueryNode> children; // вложенные условия
        bool negated = false;            // для NOT

        bool isLeaf() const { return children.empty(); }
    };

} // namespace lse