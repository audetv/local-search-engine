#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>

namespace lse
{

    class GenreMapper
    {
    public:
        GenreMapper() = default;

        // Загружает маппинг из CSV-файла (формат: оригинал,маппинг)
        // Возвращает true при успехе
        bool load(const std::filesystem::path &csv_path);

        // Маппит жанр. Если нет в справочнике — возвращает оригинал.
        std::string map(const std::string &genre) const;

        // Проверяет, загружен ли маппинг
        bool empty() const { return mapping_.empty(); }

    private:
        std::unordered_map<std::string, std::string> mapping_;
    };

} // namespace lse