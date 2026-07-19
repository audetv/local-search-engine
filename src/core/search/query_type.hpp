#pragma once

namespace lse
{

    enum class QueryType
    {
        QueryString, // AND по умолчанию, операторы включены
        Match,       // OR по умолчанию, операторы как текст
        MatchPhrase, // весь запрос как фраза
        MatchAll     // все документы
    };

} // namespace lse