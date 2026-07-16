#pragma once

/**
 * Local Search Engine - Core Library
 *
 * This is the main public header for the search engine library.
 * All public API is accessible through this header.
 */

#include <string>
#include <vector>
#include <memory>
#include <expected>
#include <filesystem>

namespace lse
{

    // Version
    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 0;

    // Forward declarations for public types
    struct SearchResult;
    struct SearchQuery;
    struct BookMetadata;
    enum class SearchError;

    // Main engine class will be declared here in Phase 1

} // namespace lse