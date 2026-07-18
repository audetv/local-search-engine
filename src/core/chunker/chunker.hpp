#pragma once

#include <string>
#include <vector>
#include <string_view>

namespace lse
{

    struct ChunkerConfig
    {
        int min_chunk_size = 300;  // минимальный размер чанка в символах
        int opt_chunk_size = 1800; // оптимальный размер
        int max_chunk_size = 3500; // максимальный размер
    };

    class Chunker
    {
    public:
        explicit Chunker(ChunkerConfig config = {});

        // Разбивает текст на чанки. Параграфы разделены \n.
        auto chunkify(std::string_view text) -> std::vector<std::string>;

    private:
        ChunkerConfig config_;

        // Разбивает длинный параграф на предложения и группирует в чанки
        auto splitLongParagraph(std::string_view paragraph) -> std::vector<std::string>;

        // Разбивает текст на предложения
        static auto splitSentences(std::string_view text) -> std::vector<std::string>;

        // Разбивает текст по словам (fallback, когда нет знаков препинания)
        static auto splitByWords(std::string_view text, int max_size) -> std::vector<std::string>;

        // Нормализует троеточия
        static std::string normalizeTripleDots(std::string_view text);

        // Подсчёт UTF-8 символов
        static size_t utf8_length(std::string_view text);
    };

} // namespace lse