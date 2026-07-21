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

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# MSVC: отключаем предупреждения о небезопасных функциях
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_SANITIZERS "Enable address/undefined sanitizers" OFF)
option(BUILD_GUI "Build Qt GUI" ON)

# Core dependencies
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(minizip CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# Qt6
if(BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
endif()

include(FetchContent)
FetchContent_Declare(
    libstemmer
    URL https://snowballstem.org/dist/libstemmer_c-2.2.0.tar.gz
    URL_HASH SHA256=b941d9fe9cf36b4e2f8d3873cd4d8b8775bd94867a1df8d8c001bb8b688377c3
)
FetchContent_MakeAvailable(libstemmer)

# Add core library
add_subdirectory(src/core)

# Console or GUI executable
if(BUILD_GUI)
    add_subdirectory(src/ui)
else()
    add_executable(local-search-engine src/main.cpp)
    target_link_libraries(local-search-engine PRIVATE lse_core)
    
    # Sanitizers for console executable
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
    
    set_target_properties(local-search-engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src"
    )
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CTEST_ARGUMENTS "--environment;ASAN_OPTIONS=detect_container_overflow=0:detect_odr_violation=0")
    endif()
    
    find_package(Catch2 CONFIG REQUIRED)
    add_subdirectory(tests)
endif()