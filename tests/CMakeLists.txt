find_package(Catch2 REQUIRED)

add_executable(lse_tests
    test_main.cpp
    test_tokenizer.cpp
)

target_link_libraries(lse_tests PRIVATE
    lse_core
    Catch2::Catch2WithMain
)

target_compile_features(lse_tests PRIVATE cxx_std_23)

# Register tests
include(Catch)
catch_discover_tests(lse_tests)