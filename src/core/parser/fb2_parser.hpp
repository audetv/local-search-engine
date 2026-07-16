#pragma once

#include "txt_parser.hpp" // для ParsedDocument и ParserError
#include <filesystem>
#include <expected>

namespace lse
{

    class Fb2Parser
    {
    public:
        auto parse(const std::filesystem::path &file_path)
            -> std::expected<ParsedDocument, ParserError>;

        auto parseFromMemory(const std::string &xml_content,
                             const std::string &file_name = "memory.fb2")
            -> std::expected<ParsedDocument, ParserError>;
    };

} // namespace lse