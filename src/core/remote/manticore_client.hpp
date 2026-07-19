#pragma once

#include "manticore_config.hpp"
#include <string>
#include <vector>
#include <expected>

namespace lse
{

    struct RemoteSearchHit
    {
        uint64_t id;
        float score;
        std::string source;
        std::string genre;
        std::string author;
        std::string title;
        std::string content;
        std::string highlight_content; // с HTML-тегами
        std::string language;
        int chunk;
        int char_count;
        int word_count;
    };

    struct RemoteSearchResult
    {
        int took_ms;
        int total;
        std::vector<RemoteSearchHit> hits;
    };

    enum class RemoteError
    {
        NetworkError,
        ServerError,
        Unauthorized,
        ParseError
    };

    class ManticoreClient
    {
    public:
        explicit ManticoreClient(const ManticoreConfig &config);

        // Выполняет поиск
        auto search(const std::string &query,
                    const std::string &query_type = "query_string",
                    int limit = 20,
                    int offset = 0)
            -> std::expected<RemoteSearchResult, RemoteError>;

    private:
        ManticoreConfig config_;

        std::string buildRequestBody(const std::string &query,
                                     const std::string &query_type,
                                     int limit,
                                     int offset) const;

        auto parseResponse(const std::string &json) const
            -> std::expected<RemoteSearchResult, RemoteError>;
    };

} // namespace lse