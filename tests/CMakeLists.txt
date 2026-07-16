add_executable(lse_tests
    test_main.cpp
)

target_link_libraries(lse_tests PRIVATE
    lse_core
    Catch2::Catch2WithMain
)

target_compile_features(lse_tests PRIVATE cxx_std_23)

# Регистрируем тест в CTest
add_test(NAME lse_tests COMMAND lse_tests)