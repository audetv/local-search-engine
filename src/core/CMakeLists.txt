# Core library - no UI dependencies
add_library(lse_core STATIC
    core.cpp
    tokenizer/tokenizer.cpp
)

target_include_directories(lse_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lse_core PUBLIC
    spdlog::spdlog
    unofficial::sqlite3::sqlite3
    pugixml::pugixml
    CURL::libcurl
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
)

target_compile_features(lse_core PUBLIC cxx_std_23)

if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(lse_core PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(lse_core PRIVATE
        -fsanitize=address,undefined
    )
endif()

if(MSVC)
    target_compile_options(lse_core PRIVATE /W4 /WX)
else()
    target_compile_options(lse_core PRIVATE -Wall -Wextra -Werror -pedantic)
endif()