#pragma once

#include <string>

namespace lse
{

    struct FilenameMetadata
    {
        std::string genre;
        std::string author;
        std::string title;
    };

    // Парсит имя файла (без пути) по шаблону: Жанр_Автор — Название
    // Возвращает структуру с извлечёнными полями.
    // Если шаблон не подходит — все поля пустые, кроме title = имя файла без расширения.
    FilenameMetadata parseFilenameMetadata(const std::string &filename);

} // namespace lse