find_package(Catch2 REQUIRED)

add_executable(lse_tests
    test_main.cpp
    test_tokenizer.cpp
    test_stemmer.cpp
    test_in_memory_index.cpp
    test_txt_parser.cpp
    test_fb2_parser.cpp
    test_docx_parser.cpp
    test_chunker.cpp
    test_memory_mapped_file.cpp
    test_index_persistence.cpp
    test_filename_parser.cpp
    test_query_parser.cpp
    test_search_engine.cpp
)

target_link_libraries(lse_tests PRIVATE
    lse_core
    Catch2::Catch2WithMain
)

target_compile_features(lse_tests PRIVATE cxx_std_23)

# MSVC: отключаем C4834 (nodiscard) для тестовых проверок
if(MSVC)
    target_compile_options(lse_tests PRIVATE /wd4834)
endif()

# Sanitizers for debug (GCC/Clang only)
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(lse_tests PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(lse_tests PRIVATE
            -fsanitize=address,undefined
        )
    elseif(MSVC)
        target_compile_options(lse_tests PRIVATE /fsanitize=address)
    endif()
endif()

include(Catch)
catch_discover_tests(lse_tests)