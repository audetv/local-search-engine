#include "manticore_config.hpp"
#include <cstdlib>
#include <fstream>
#include <spdlog/spdlog.h>

namespace lse
{

    void ManticoreConfig::load()
    {
        // API ключ из переменной окружения
        const char *env_key = std::getenv("LSE_API_KEY");
        if (env_key)
        {
            api_key = env_key;
        }

        // Или из файла ~/.local-search-engine/api.key
        if (api_key.empty())
        {
            const char *home = std::getenv("HOME");
            if (home)
            {
                std::string key_path = std::string(home) + "/.local-search-engine/api.key";
                std::ifstream f(key_path);
                if (f.is_open())
                {
                    std::getline(f, api_key);
                    if (!api_key.empty())
                    {
                        spdlog::info("ManticoreConfig: loaded API key from {}", key_path);
                    }
                }
            }
        }

        // URL сервера из переменной окружения
        const char *env_url = std::getenv("LSE_SERVER_URL");
        if (env_url)
        {
            server_url = env_url;
        }

        // Или из файла ~/.local-search-engine/server.url
        if (server_url.empty())
        {
            const char *home = std::getenv("HOME");
            if (home)
            {
                std::string url_path = std::string(home) + "/.local-search-engine/server.url";
                std::ifstream f(url_path);
                if (f.is_open())
                {
                    std::getline(f, server_url);
                    if (!server_url.empty())
                    {
                        spdlog::info("ManticoreConfig: loaded server URL from {}", url_path);
                    }
                }
            }
        }

        // Ранкер
        const char *env_ranker = std::getenv("LSE_RANKER");
        if (env_ranker)
        {
            ranker = env_ranker;
        }

        if (api_key.empty() || server_url.empty())
        {
            spdlog::warn("ManticoreConfig: API key or server URL not set. Remote search disabled.");
        }
    }

} // namespace lse