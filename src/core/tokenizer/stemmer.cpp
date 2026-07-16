#include "stemmer.hpp"

// Подключаем официальный заголовок libstemmer
extern "C"
{
#include "libstemmer.h"
}

#include <stdexcept>

namespace lse
{

    Stemmer::Stemmer(StemmerLanguage lang) : lang_(lang)
    {
        const char *algorithm = nullptr;

        switch (lang)
        {
        case StemmerLanguage::Russian:
            algorithm = "russian";
            break;
        case StemmerLanguage::English:
            algorithm = "english";
            break;
        }

        stemmer_ = sb_stemmer_new(algorithm, "UTF_8");
        if (!stemmer_)
        {
            throw std::runtime_error(
                "Failed to create stemmer for language: " + std::string(algorithm));
        }
    }

    Stemmer::~Stemmer()
    {
        if (stemmer_)
        {
            sb_stemmer_delete(stemmer_);
        }
    }

    Stemmer::Stemmer(Stemmer &&other) noexcept
        : stemmer_(other.stemmer_), lang_(other.lang_)
    {
        other.stemmer_ = nullptr;
    }

    Stemmer &Stemmer::operator=(Stemmer &&other) noexcept
    {
        if (this != &other)
        {
            if (stemmer_)
            {
                sb_stemmer_delete(stemmer_);
            }
            stemmer_ = other.stemmer_;
            lang_ = other.lang_;
            other.stemmer_ = nullptr;
        }
        return *this;
    }

    auto Stemmer::stem(std::string_view word) -> std::string_view
    {
        if (!stemmer_)
        {
            return word;
        }

        const sb_symbol *result = sb_stemmer_stem(
            stemmer_,
            reinterpret_cast<const sb_symbol *>(word.data()),
            static_cast<int>(word.size()));

        if (!result)
        {
            return word;
        }

        int len = sb_stemmer_length(stemmer_);
        return std::string_view(reinterpret_cast<const char *>(result), static_cast<size_t>(len));
    }

} // namespace lse