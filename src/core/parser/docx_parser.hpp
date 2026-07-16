#pragma once

#include "txt_parser.hpp" // для ParsedDocument и ParserError
#include <filesystem>
#include <expected>

namespace lse
{

    class DocxParser
    {
    public:
        auto parse(const std::filesystem::path &file_path)
            -> std::expected<ParsedDocument, ParserError>;
    };

} // namespace lse