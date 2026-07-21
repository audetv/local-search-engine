#include "manticore_client.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace lse
{

    // Callback для curl
    static size_t write_callback(void *contents, size_t size, size_t nmemb, std::string *output)
    {
        size_t total = size * nmemb;
        output->append(static_cast<char *>(contents), total);
        return total;
    }

    ManticoreClient::ManticoreClient(const ManticoreConfig &config) : config_(config) {}

    std::string ManticoreClient::buildRequestBody(const std::string &query,
                                                  const std::string &query_type,
                                                  int limit,
                                                  int offset) const
    {
        nlohmann::json body;
        body["table"] = config_.table;
        body["options"]["ranker"] = config_.ranker;

        if (query_type == "match")
        {
            body["query"]["match"]["content"]["query"] = query;
        }
        else if (query_type == "match_phrase")
        {
            body["query"]["match_phrase"]["content"]["query"] = query;
        }
        else
        {
            // query_string
            body["query"]["query_string"] = "@content " + query;
        }

        body["_source"] = {"source", "genre", "author", "title", "content",
                           "language", "chunk", "char_count", "word_count"};

        body["highlight"]["fields"] = {"content"};
        body["highlight"]["limit"] = 0;
        body["highlight"]["encoder"] = "html";
        body["highlight"]["no_match_size"] = 0;
        body["highlight"]["pre_tags"] = "<span style=\"background-color: #e1e0e0;\">";
        body["highlight"]["post_tags"] = "</span>";

        body["limit"] = limit;
        body["offset"] = offset;
        body["max_matches"] = config_.max_matches;

        return body.dump();
    }

    auto ManticoreClient::search(const std::string &query,
                                 const std::string &query_type,
                                 int limit,
                                 int offset)
        -> std::expected<RemoteSearchResult, RemoteError>
    {

        if (!config_.valid())
        {
            return std::unexpected(RemoteError::Unauthorized);
        }

        std::string request_body = buildRequestBody(query, query_type, limit, offset);
        std::string response_body;

        CURL *curl = curl_easy_init();
        if (!curl)
        {
            return std::unexpected(RemoteError::NetworkError);
        }

        std::string url = config_.server_url + "/search";

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string api_header = "X-API-Key: " + config_.api_key;
        headers = curl_slist_append(headers, api_header.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_body.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);

        CURLcode res = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res != CURLE_OK)
        {
            spdlog::error("ManticoreClient: curl error: {}", curl_easy_strerror(res));
            return std::unexpected(RemoteError::NetworkError);
        }

        if (http_code == 401 || http_code == 403)
        {
            return std::unexpected(RemoteError::Unauthorized);
        }

        if (http_code != 200)
        {
            spdlog::error("ManticoreClient: HTTP {}", http_code);
            return std::unexpected(RemoteError::ServerError);
        }

        return parseResponse(response_body);
    }

    auto ManticoreClient::parseResponse(const std::string &json) const
        -> std::expected<RemoteSearchResult, RemoteError>
    {

        try
        {
            auto j = nlohmann::json::parse(json);

            RemoteSearchResult result;
            result.took_ms = j.value("took", 0);
            result.total = j["hits"].value("total", 0);

            for (const auto &hit : j["hits"]["hits"])
            {
                RemoteSearchHit rh;
                rh.id = hit.value("_id", 0ULL);
                rh.score = hit.value("_score", 0.0f);

                const auto &src = hit["_source"];
                rh.source = src.value("source", "");
                rh.genre = src.value("genre", "");
                rh.author = src.value("author", "");
                rh.title = src.value("title", "");
                rh.content = src.value("content", "");
                rh.language = src.value("language", "");
                rh.chunk = src.value("chunk", 0);
                rh.char_count = src.value("char_count", 0);
                rh.word_count = src.value("word_count", 0);

                // Подсветка
                if (hit.contains("highlight") && hit["highlight"].contains("content"))
                {
                    auto &hl = hit["highlight"]["content"];
                    if (!hl.empty() && hl[0].is_string())
                    {
                        rh.highlight_content = hl[0].get<std::string>();
                    }
                }

                result.hits.push_back(std::move(rh));
            }

            return result;
        }
        catch (const nlohmann::json::exception &e)
        {
            spdlog::error("ManticoreClient: JSON parse error: {}", e.what());
            return std::unexpected(RemoteError::ParseError);
        }
    }

} // namespace lse