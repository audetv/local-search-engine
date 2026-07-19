#include "genre_mapper.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace lse
{

    bool GenreMapper::load(const std::filesystem::path &csv_path)
    {
        std::ifstream file(csv_path);
        if (!file.is_open())
        {
            spdlog::warn("GenreMapper: could not open {}", csv_path.string());
            return false;
        }

        auto trim = [](std::string &s)
        {
            s.erase(0, s.find_first_not_of(" \t\""));
            s.erase(s.find_last_not_of(" \t\"") + 1);
        };

        std::string line;
        int count = 0;

        while (std::getline(file, line))
        {
            if (line.empty())
                continue;

            std::string original, mapped;
            bool in_quotes = false;
            size_t comma_pos = std::string::npos;

            // Ищем первую запятую вне кавычек
            for (size_t i = 0; i < line.size(); ++i)
            {
                if (line[i] == '"')
                {
                    in_quotes = !in_quotes;
                }
                else if (line[i] == ',' && !in_quotes)
                {
                    comma_pos = i;
                    break;
                }
            }

            if (comma_pos == std::string::npos)
                continue;

            original = line.substr(0, comma_pos);
            mapped = line.substr(comma_pos + 1);

            trim(original);
            trim(mapped);

            if (!original.empty() && !mapped.empty())
            {
                mapping_[original] = mapped;
                count++;
            }
        }

        spdlog::info("GenreMapper: loaded {} mappings from {}", count, csv_path.string());
        return count > 0;
    }

    std::string GenreMapper::map(const std::string &genre) const
    {
        if (genre.empty())
            return genre;

        auto it = mapping_.find(genre);
        if (it != mapping_.end())
        {
            return it->second;
        }

        return genre;
    }

} // namespace lse