cmake_minimum_required(VERSION 3.27)

project(
    LocalSearchEngine
    VERSION 1.0.0
    DESCRIPTION "Local full-text search engine for Russian books"
    LANGUAGES CXX
)

# C++23 required
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_SANITIZERS "Enable address/undefined sanitizers" OFF)

# Find packages (using actual vcpkg target names)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

# Add core library
add_subdirectory(src/core)

# Main executable
add_executable(local-search-engine src/main.cpp)
target_link_libraries(local-search-engine PRIVATE lse_core)

# Положить бинарник рядом с исходниками (удобно)
set_target_properties(local-search-engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src"
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    find_package(Catch2 CONFIG REQUIRED)
    add_subdirectory(tests)
endif()