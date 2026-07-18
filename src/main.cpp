#include <iostream>
#include <chrono>
#include <spdlog/spdlog.h>
#include "core/tokenizer/tokenizer.hpp"
#include "core/tokenizer/stemmer.hpp"
#include "core/index/index_writer.hpp"
#include "core/index/index_reader.hpp"
#include "core/book_indexer.hpp"

using namespace lse;
using namespace std::chrono;

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::info);

    std::cout << "\n========================================\n";
    std::cout << "  Local Search Engine - Console Prototype\n";
    std::cout << "========================================\n\n";

    // Путь к индексу
    std::string index_path = "./user_index";
    if (argc == 4 && std::string(argv[1]) == "index")
    {
        index_path = argv[3];
    }

    Stemmer stemmer(StemmerLanguage::Russian);
    TokenizerOptions tokenizer_opts;
    tokenizer_opts.normalize_yo = true;
    tokenizer_opts.keep_digits = true;

    // Режим 1: индексация
    if (argc >= 3 && std::string(argv[1]) == "index")
    {
        std::string path = argv[2];
        std::cout << "Indexing directory: " << path << std::endl;
        std::cout << "Index path: " << index_path << std::endl;

        IndexWriter writer;
        auto open_result = writer.open(index_path);
        if (!open_result.has_value())
        {
            std::cerr << "Failed to open index: " << index_path << std::endl;
            return 1;
        }

        BookIndexer book_indexer(writer, stemmer);

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

        writer.close();
        return 0;
    }

    // Режим 2: интерактивный поиск
    IndexReader reader;
    auto open_result = reader.open(index_path);
    if (!open_result.has_value())
    {
        std::cout << "No index found at " << index_path << ". Use 'index <path>' first.\n";
    }

    std::cout << "Commands:\n";
    std::cout << "  index <path> [index_dir]  - index directory\n";
    std::cout << "  <query>                   - search (type 'quit' to exit)\n\n";

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
            // Разбираем: index <path> [index_dir]
            std::string args = line.substr(6);
            size_t space = args.find(' ');
            std::string dir_path = args;
            std::string idx_path = index_path;
            if (space != std::string::npos)
            {
                dir_path = args.substr(0, space);
                idx_path = args.substr(space + 1);
            }

            reader.close();

            IndexWriter writer;
            auto wr_result = writer.open(idx_path);
            if (!wr_result.has_value())
            {
                std::cerr << "Failed to open index: " << idx_path << std::endl;
                continue;
            }

            BookIndexer book_indexer(writer, stemmer);
            auto result = book_indexer.indexDirectory(dir_path);
            if (result.has_value())
            {
                std::cout << "Indexed " << *result << " files into " << idx_path << "\n";
            }
            writer.close();

            // Переоткрываем reader
            reader.open(idx_path);
            continue;
        }

        // Поиск
        if (!reader.docCount())
        {
            std::cout << "Index is empty. Use 'index <path>' first.\n";
            continue;
        }

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

        auto start = high_resolution_clock::now();
        auto search_result = reader.search(query_terms, 10);
        auto end = high_resolution_clock::now();

        if (!search_result.has_value())
        {
            std::cout << "Search error\n";
            continue;
        }

        auto duration = duration_cast<microseconds>(end - start);

        std::cout << "\nFound " << search_result->size() << " results"
                  << " in " << duration.count() << " µs"
                  << " (" << reader.docCount() << " docs indexed)\n\n";

        for (const auto &hit : *search_result)
        {
            std::cout << "  doc_id=" << hit->doc_id
                      << " score=" << hit->bm25_score
                      << " title=" << hit->title << "\n";
        }
        std::cout << std::endl;
    }

    reader.close();
    return 0;
}