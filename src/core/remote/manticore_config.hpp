#pragma once

#include <string>
#include <cstdint>

namespace lse
{

    struct ManticoreConfig
    {
        std::string server_url;
        std::string api_key;
        std::string table = "library2026";
        std::string ranker = "proximity_bm25";
        int timeout_seconds = 10;
        int max_matches = 1000;

        // Загружает конфиг из переменных окружения и файла
        void load();

        // Проверяет, что конфиг валиден для запроса
        bool valid() const { return !api_key.empty() && !server_url.empty(); }
    };

} // namespace lse