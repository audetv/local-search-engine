#include "chunker.hpp"
#include <algorithm>
#include <regex>

namespace lse
{

    Chunker::Chunker(ChunkerConfig config) : config_(config) {}

    auto Chunker::chunkify(std::string_view text) -> std::vector<std::string>
    {
        std::vector<std::string> chunks;
        std::string current_chunk;

        // Разбиваем входной текст на параграфы по \n
        size_t start = 0;
        while (start < text.size())
        {
            size_t end = text.find('\n', start);
            if (end == std::string::npos)
            {
                end = text.size();
            }

            std::string paragraph(text.substr(start, end - start));
            start = end + 1;

            // Нормализуем троеточия
            paragraph = normalizeTripleDots(paragraph);

            // Пропускаем пустые параграфы
            if (paragraph.empty())
            {
                continue;
            }

            int paragraph_len = static_cast<int>(paragraph.size());

            // Если параграф длиннее max_chunk_size — разбиваем по предложениям
            if (paragraph_len > config_.max_chunk_size)
            {
                // Сначала сохраним накопленный чанк
                if (!current_chunk.empty())
                {
                    chunks.push_back(std::move(current_chunk));
                    current_chunk.clear();
                }

                // Разбиваем длинный параграф
                auto sub_chunks = splitLongParagraph(paragraph);
                for (auto &sub : sub_chunks)
                {
                    chunks.push_back(std::move(sub));
                }
                continue;
            }

            // Пытаемся склеить с текущим чанком
            int combined_len = static_cast<int>(current_chunk.size()) + paragraph_len + 1; // +1 за пробел

            if (combined_len <= config_.max_chunk_size)
            {
                // Можно склеить
                if (!current_chunk.empty())
                {
                    current_chunk += ' ';
                }
                current_chunk += paragraph;

                // Если достигли оптимального размера — сохраняем чанк
                if (static_cast<int>(current_chunk.size()) >= config_.opt_chunk_size)
                {
                    chunks.push_back(std::move(current_chunk));
                    current_chunk.clear();
                }
            }
            else
            {
                // Нельзя склеить — сохраняем текущий и начинаем новый
                if (!current_chunk.empty())
                {
                    chunks.push_back(std::move(current_chunk));
                    current_chunk.clear();
                }
                current_chunk = paragraph;
            }
        }

        // Сохраняем оставшийся чанк
        if (!current_chunk.empty())
        {
            // Если остаток слишком мал, склеиваем с последним чанком
            if (static_cast<int>(current_chunk.size()) < config_.min_chunk_size && !chunks.empty())
            {
                std::string &last = chunks.back();
                if (static_cast<int>(last.size() + current_chunk.size() + 1) <= config_.max_chunk_size)
                {
                    last += ' ';
                    last += current_chunk;
                }
                else
                {
                    chunks.push_back(std::move(current_chunk));
                }
            }
            else
            {
                chunks.push_back(std::move(current_chunk));
            }
        }

        return chunks;
    }

    auto Chunker::splitLongParagraph(std::string_view paragraph) -> std::vector<std::string>
    {
        std::vector<std::string> result;
        auto sentences = splitSentences(paragraph);

        std::string current;
        for (auto &sentence : sentences)
        {
            if (sentence.empty())
                continue;

            int combined_len = static_cast<int>(current.size()) + static_cast<int>(sentence.size()) + 1;

            if (combined_len <= config_.opt_chunk_size)
            {
                if (!current.empty())
                    current += ' ';
                current += sentence;
            }
            else
            {
                // Сохраняем накопленное
                if (!current.empty())
                {
                    result.push_back(std::move(current));
                    current.clear();
                }

                // Если одиночное предложение всё ещё длиннее opt — добавляем как есть
                if (static_cast<int>(sentence.size()) > config_.opt_chunk_size)
                {
                    result.push_back(sentence);
                }
                else
                {
                    current = sentence;
                }
            }
        }

        if (!current.empty())
        {
            result.push_back(std::move(current));
        }

        return result;
    }

    auto Chunker::splitSentences(std::string_view text) -> std::vector<std::string>
    {
        std::vector<std::string> sentences;
        size_t start = 0;

        for (size_t i = 0; i < text.size(); ++i)
        {
            char c = text[i];
            // Конец предложения: . ! ? … после пробела или в конце текста
            if (c == '.' || c == '!' || c == '?')
            {
                // Проверяем, что это не часть числа (3.14) или инициала (А.С.)
                bool is_end = false;
                if (i + 1 >= text.size())
                {
                    is_end = true;
                }
                else if (text[i + 1] == ' ' || text[i + 1] == '\n')
                {
                    is_end = true;
                }

                if (is_end)
                {
                    size_t len = i - start + 1;
                    std::string sentence(text.substr(start, len));
                    // Убираем пробелы в начале
                    size_t first = sentence.find_first_not_of(' ');
                    if (first != std::string::npos)
                    {
                        sentence = sentence.substr(first);
                    }
                    if (!sentence.empty())
                    {
                        sentences.push_back(sentence);
                    }
                    start = i + 1;
                    // Пропускаем пробелы после знака
                    while (start < text.size() && (text[start] == ' ' || text[start] == '\n'))
                    {
                        ++start;
                    }
                    i = start - 1; // цикл инкрементирует
                }
            }
            else if (c == '\n')
            {
                // Перенос строки тоже разделитель
                if (i > start)
                {
                    std::string sentence(text.substr(start, i - start));
                    size_t first = sentence.find_first_not_of(' ');
                    if (first != std::string::npos)
                    {
                        sentence = sentence.substr(first);
                    }
                    if (!sentence.empty())
                    {
                        sentences.push_back(sentence);
                    }
                }
                start = i + 1;
            }
        }

        // Последнее предложение
        if (start < text.size())
        {
            std::string sentence(text.substr(start));
            size_t first = sentence.find_first_not_of(' ');
            if (first != std::string::npos)
            {
                sentence = sentence.substr(first);
            }
            if (!sentence.empty())
            {
                sentences.push_back(sentence);
            }
        }

        return sentences;
    }

    std::string Chunker::normalizeTripleDots(std::string_view text)
    {
        std::string result(text);
        // Заменяем "..." на "…"
        for (size_t pos = result.find("..."); pos != std::string::npos; pos = result.find("..."))
        {
            result.replace(pos, 3, "…");
        }
        // Заменяем ". . ." на "…"
        for (size_t pos = result.find(". . ."); pos != std::string::npos; pos = result.find(". . ."))
        {
            result.replace(pos, 5, "…");
        }
        return result;
    }

} // namespace lse