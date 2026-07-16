find_package(Catch2 REQUIRED)

add_executable(lse_tests
    test_main.cpp
    test_tokenizer.cpp
    test_stemmer.cpp
)

target_link_libraries(lse_tests PRIVATE
    lse_core
    Catch2::Catch2WithMain
)

target_compile_features(lse_tests PRIVATE cxx_std_23)

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