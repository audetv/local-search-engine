#include "filename_parser.hpp"
#include <algorithm>

namespace lse
{

    static void trim(std::string &s)
    {
        s.erase(0, s.find_first_not_of(' '));
        s.erase(s.find_last_not_of(' ') + 1);
    }

    FilenameMetadata parseFilenameMetadata(const std::string &filename)
    {
        std::string base = filename;
        size_t dot = base.find_last_of('.');
        if (dot != std::string::npos)
        {
            base = base.substr(0, dot);
        }

        size_t dash = base.find(" — ");

        if (dash != std::string::npos)
        {
            std::string left = base.substr(0, dash);
            std::string title = base.substr(dash + std::string(" — ").size());

            size_t first_underscore = left.find('_');

            std::string genre;
            std::string author;

            if (first_underscore != std::string::npos)
            {
                genre = left.substr(0, first_underscore);
                author = left.substr(first_underscore + 1);
            }
            else
            {
                author = left;
            }

            trim(genre);
            trim(author);
            trim(title);

            return {genre, author, title};
        }

        return {"", "", base};
    }

} // namespace lse