#pragma once

#include <cstdint>

namespace lse
{

    enum class IndexError
    {
        AlreadyOpen,
        NotOpen,
        FileError,
        EmptyDocument,
        WriteError,
        EmptyQuery,
        CorruptedIndex
    };

} // namespace lse