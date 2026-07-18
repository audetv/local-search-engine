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
find_package(minizip CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# libstemmer (Snowball stemmer) via FetchContent
include(FetchContent)
FetchContent_Declare(
    libstemmer
    URL https://snowballstem.org/dist/libstemmer_c-2.2.0.tar.gz
    URL_HASH SHA256=b941d9fe9cf36b4e2f8d3873cd4d8b8775bd94867a1df8d8c001bb8b688377c3
)
FetchContent_MakeAvailable(libstemmer)

# Add core library
add_subdirectory(src/core)

# Main executable
add_executable(local-search-engine src/main.cpp)
target_link_libraries(local-search-engine PRIVATE lse_core)

# Sanitizers for main executable
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(local-search-engine PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(local-search-engine PRIVATE
            -fsanitize=address,undefined
        )
    elseif(MSVC)
        target_compile_options(local-search-engine PRIVATE /fsanitize=address)
    endif()
endif()

# Положить бинарник рядом с исходниками (удобно)
set_target_properties(local-search-engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src"
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    # ASan false positive: отключаем проверки контейнеров для всех тестов
    if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CTEST_ARGUMENTS "--environment;ASAN_OPTIONS=detect_container_overflow=0:detect_odr_violation=0")
    endif()
    
    find_package(Catch2 CONFIG REQUIRED)
    add_subdirectory(tests)
endif()