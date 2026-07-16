cmake_minimum_required(VERSION 3.27)

project(
    LocalSearchEngine
    VERSION 1.0.0
    DESCRIPTION "Local full-text search engine for Russian books"
    LANGUAGES C CXX
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

# libstemmer (Snowball stemmer) via FetchContent
include(FetchContent)
FetchContent_Declare(
    libstemmer
    URL https://snowballstem.org/dist/libstemmer_c-2.2.0.tar.gz
    URL_HASH SHA256=b941d9fe9cf36b4e2f8d3873cd4d8b8775bd94867a1df8d8c001bb8b688377c3
)
FetchContent_MakeAvailable(libstemmer)

# Собираем libstemmer из исходников — только русский и английский
add_library(libstemmer_c STATIC
    # Русский стеммер
    ${libstemmer_SOURCE_DIR}/src_c/stem_UTF_8_russian.c
    # Английский стеммер
    ${libstemmer_SOURCE_DIR}/src_c/stem_UTF_8_english.c
    # Runtime
    ${libstemmer_SOURCE_DIR}/runtime/api.c
    ${libstemmer_SOURCE_DIR}/runtime/utilities.c
)

target_include_directories(libstemmer_c PUBLIC
    ${libstemmer_SOURCE_DIR}/include
    ${libstemmer_SOURCE_DIR}/runtime
    ${libstemmer_SOURCE_DIR}/src_c
    ${libstemmer_SOURCE_DIR}/libstemmer
)

if(MSVC)
    target_compile_options(libstemmer_c PRIVATE /W0)
else()
    target_compile_options(libstemmer_c PRIVATE -w)
endif()

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