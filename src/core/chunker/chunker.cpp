#include "chunker.hpp"
#include <algorithm>

namespace lse
{

    Chunker::Chunker(ChunkerConfig config) : config_(config) {}

    size_t Chunker::utf8_length(std::string_view text)
    {
        size_t len = 0;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80)
            {
                len++;
            }
        }
        return len;
    }

    auto Chunker::chunkify(std::string_view text) -> std::vector<std::string>
    {
        std::vector<std::string> chunks;
        std::string current_chunk;

        // Разбиваем на параграфы
        size_t start = 0;
        while (start < text.size())
        {
            size_t end = text.find('\n', start);
            if (end == std::string::npos)
                end = text.size();

            std::string paragraph(text.substr(start, end - start));
            start = end + 1;

            // Нормализуем троеточия
            paragraph = normalizeTripleDots(paragraph);

            // Пропускаем пустые
            if (paragraph.empty())
                continue;

            int par_len = static_cast<int>(utf8_length(paragraph));

            // Длинный параграф — разбиваем
            if (par_len > config_.max_chunk_size)
            {
                // Сохраняем накопленное
                if (!current_chunk.empty())
                {
                    chunks.push_back(std::move(current_chunk));
                    current_chunk.clear();
                }

                auto sub_chunks = splitLongParagraph(paragraph);
                for (auto &sub : sub_chunks)
                {
                    chunks.push_back(std::move(sub));
                }
                continue;
            }

            // Пытаемся склеить
            int combined = static_cast<int>(utf8_length(current_chunk)) + par_len;
            if (!current_chunk.empty())
                combined++; // пробел

            if (combined <= config_.max_chunk_size)
            {
                if (!current_chunk.empty())
                    current_chunk += ' ';
                current_chunk += paragraph;

                // Достигли оптимального — сохраняем
                if (static_cast<int>(utf8_length(current_chunk)) >= config_.opt_chunk_size)
                {
                    chunks.push_back(std::move(current_chunk));
                    current_chunk.clear();
                }
            }
            else
            {
                // Не влезает — сохраняем текущий, начинаем новый
                if (!current_chunk.empty())
                {
                    chunks.push_back(std::move(current_chunk));
                    current_chunk.clear();
                }
                current_chunk = paragraph;
            }
        }

        // Остаток
        if (!current_chunk.empty())
        {
            // Если слишком мал — склеиваем с последним
            if (static_cast<int>(utf8_length(current_chunk)) < config_.min_chunk_size && !chunks.empty())
            {
                std::string &last = chunks.back();
                if (static_cast<int>(utf8_length(last) + utf8_length(current_chunk) + 1) <= config_.max_chunk_size)
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
        auto sentences = splitSentences(paragraph);

        // Если одно предложение и оно длиннее max_chunk_size — разбиваем по словам
        if (sentences.size() == 1 && static_cast<int>(utf8_length(sentences[0])) > config_.max_chunk_size)
        {
            return splitByWords(sentences[0], config_.max_chunk_size);
        }

        // Если предложений нет — разбиваем по словам
        if (sentences.empty())
        {
            return splitByWords(paragraph, config_.max_chunk_size);
        }

        std::vector<std::string> result;
        std::string current;

        for (auto &sentence : sentences)
        {
            if (sentence.empty())
                continue;

            int sent_len = static_cast<int>(utf8_length(sentence));

            // Одиночное предложение длиннее opt — добавляем как есть
            if (sent_len > config_.opt_chunk_size)
            {
                if (!current.empty())
                {
                    result.push_back(std::move(current));
                    current.clear();
                }
                result.push_back(sentence);
                continue;
            }

            int combined = static_cast<int>(utf8_length(current)) + sent_len;
            if (!current.empty())
                combined++;

            if (combined <= config_.opt_chunk_size)
            {
                if (!current.empty())
                    current += ' ';
                current += sentence;
            }
            else
            {
                if (!current.empty())
                {
                    result.push_back(std::move(current));
                    current.clear();
                }
                current = sentence;
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
            if (c == '.' || c == '!' || c == '?')
            {
                // Проверяем, что это конец предложения (пробел или конец строки после)
                bool is_end = false;
                if (i + 1 >= text.size())
                {
                    is_end = true;
                }
                else if (text[i + 1] == ' ' || text[i + 1] == '\n' || text[i + 1] == '\r')
                {
                    is_end = true;
                }

                if (is_end)
                {
                    size_t len = i - start + 1;
                    std::string sentence(text.substr(start, len));
                    // Убираем начальные пробелы
                    size_t first = sentence.find_first_not_of(' ');
                    if (first != std::string::npos && first > 0)
                    {
                        sentence = sentence.substr(first);
                    }
                    if (!sentence.empty())
                    {
                        sentences.push_back(sentence);
                    }
                    start = i + 1;
                    // Пропускаем пробелы после знака
                    while (start < text.size() && (text[start] == ' ' || text[start] == '\n' || text[start] == '\r'))
                    {
                        ++start;
                    }
                    i = start - 1;
                }
            }
        }

        // Последнее предложение (может быть без знака)
        if (start < text.size())
        {
            std::string sentence(text.substr(start));
            size_t first = sentence.find_first_not_of(' ');
            if (first != std::string::npos && first > 0)
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

    auto Chunker::splitByWords(std::string_view text, int max_size) -> std::vector<std::string>
    {
        std::vector<std::string> result;
        std::string current;

        size_t pos = 0;
        while (pos < text.size())
        {
            // Пропускаем пробелы
            while (pos < text.size() && text[pos] == ' ')
                pos++;
            if (pos >= text.size())
                break;

            // Ищем конец слова
            size_t word_end = text.find(' ', pos);
            if (word_end == std::string::npos)
                word_end = text.size();

            std::string word(text.substr(pos, word_end - pos));
            pos = word_end;

            int combined = static_cast<int>(utf8_length(current)) + static_cast<int>(utf8_length(word));
            if (!current.empty())
                combined++;

            if (combined <= max_size)
            {
                if (!current.empty())
                    current += ' ';
                current += word;
            }
            else
            {
                if (!current.empty())
                {
                    result.push_back(std::move(current));
                    current.clear();
                }
                current = word;
            }
        }

        if (!current.empty())
        {
            result.push_back(std::move(current));
        }

        return result;
    }

    std::string Chunker::normalizeTripleDots(std::string_view text)
    {
        std::string result(text);
        for (size_t pos = result.find("..."); pos != std::string::npos; pos = result.find("..."))
        {
            result.replace(pos, 3, "…");
        }
        for (size_t pos = result.find(". . ."); pos != std::string::npos; pos = result.find(". . ."))
        {
            result.replace(pos, 5, "…");
        }
        return result;
    }

} // namespace lse