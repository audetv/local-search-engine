#include <iostream>
#include <chrono>
#include <spdlog/spdlog.h>
#include "core/tokenizer/tokenizer.hpp"
#include "core/tokenizer/stemmer.hpp"
#include "core/index/in_memory_index.hpp"
#include "core/book_indexer.hpp"

using namespace lse;
using namespace std::chrono;

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::info);

    std::cout << "\n========================================\n";
    std::cout << "  Local Search Engine - Console Prototype\n";
    std::cout << "========================================\n\n";

    InMemoryIndex index;
    Stemmer stemmer(StemmerLanguage::Russian);
    BookIndexer book_indexer(index, stemmer);

    // Режим 1: индексация
    if (argc == 3 && std::string(argv[1]) == "index")
    {
        std::string path = argv[2];
        std::cout << "Indexing directory: " << path << std::endl;

        auto start = high_resolution_clock::now();
        auto result = book_indexer.indexDirectory(path);
        auto end = high_resolution_clock::now();

        if (result.has_value())
        {
            auto duration = duration_cast<milliseconds>(end - start);
            std::cout << "\nDone! Indexed " << *result << " files, "
                      << book_indexer.indexedChunks() << " chunks"
                      << " in " << duration.count() << " ms\n";
        }
        else
        {
            std::cerr << "Indexing failed!\n";
            return 1;
        }
        return 0;
    }

    // Режим 2: интерактивный поиск
    std::cout << "Commands:\n";
    std::cout << "  index <path>   - index directory\n";
    std::cout << "  <query>        - search (type 'quit' to exit)\n\n";

    TokenizerOptions tokenizer_opts;
    tokenizer_opts.normalize_yo = true;
    tokenizer_opts.keep_digits = true;

    std::string line;
    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line.empty())
            continue;
        if (line == "quit" || line == "exit")
            break;

        // Команда index
        if (line.starts_with("index "))
        {
            std::string path = line.substr(6);
            auto result = book_indexer.indexDirectory(path);
            if (result.has_value())
            {
                std::cout << "Indexed " << *result << " files\n";
            }
            continue;
        }

        // Поиск
        if (index.docCount() == 0)
        {
            std::cout << "Index is empty. Use 'index <path>' first.\n";
            continue;
        }

        // Токенизируем запрос
        Tokenizer tokenizer(line, tokenizer_opts);
        auto tokenize_result = tokenizer.tokenize();
        if (!tokenize_result.has_value())
        {
            std::cout << "Invalid query\n";
            continue;
        }

        std::vector<std::string> query_terms;
        for (auto &token : *tokenize_result)
        {
            query_terms.push_back(std::string(stemmer.stem(token.text)));
        }

        // Ищем с замером времени
        auto start = high_resolution_clock::now();
        auto search_result = index.search(query_terms, 10);
        auto end = high_resolution_clock::now();

        if (!search_result.has_value())
        {
            std::cout << "Search error\n";
            continue;
        }

        auto duration = duration_cast<microseconds>(end - start);

        std::cout << "\nFound " << search_result->size() << " results"
                  << " in " << duration.count() << " µs"
                  << " (" << index.docCount() << " docs indexed)\n\n";

        for (const auto &hit : *search_result)
        {
            std::cout << "  doc_id=" << hit.doc_id
                      << " score=" << hit.bm25_score << "\n";
        }
        std::cout << std::endl;
    }

    return 0;
}