#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main()
{
    // Setup logger
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("Local Search Engine v1.0.0 starting...");
    spdlog::info("Build: {} {}", __DATE__, __TIME__);
    spdlog::info("C++ Standard: {}", __cplusplus);

    std::cout << "\n========================================\n";
    std::cout << "  Local Search Engine - Ready\n";
    std::cout << "========================================\n";
    std::cout << "  Phase 0: Infrastructure complete\n";
    std::cout << "  Phase 1: Core engine (TBD)\n";
    std::cout << "========================================\n\n";

    spdlog::info("Shutting down. Goodbye!");

    return 0;
}