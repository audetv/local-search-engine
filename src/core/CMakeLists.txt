file(GLOB STEMMER_C_FILES
    "${libstemmer_SOURCE_DIR}/src_c/*.c"
    "${libstemmer_SOURCE_DIR}/runtime/*.c"
    "${libstemmer_SOURCE_DIR}/libstemmer/*.c"
)

find_package(uchardet CONFIG REQUIRED)
find_package(minizip CONFIG REQUIRED)

add_library(lse_core STATIC
    core.cpp
    tokenizer/tokenizer.cpp
    tokenizer/stemmer.cpp
    index/in_memory_index.cpp
    index/memory_mapped_file.cpp
    index/index_writer.cpp
    index/index_reader.cpp
    parser/txt_parser.cpp
    parser/fb2_parser.cpp
    parser/docx_parser.cpp
    chunker/chunker.cpp
    book_indexer.cpp
    ${STEMMER_C_FILES}
)

target_include_directories(lse_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${libstemmer_SOURCE_DIR}/include
    ${libstemmer_SOURCE_DIR}/runtime
    ${libstemmer_SOURCE_DIR}/src_c
    ${libstemmer_SOURCE_DIR}/libstemmer
)

# C-файлы компилируем с подавлением ворнингов
set_source_files_properties(${STEMMER_C_FILES} PROPERTIES COMPILE_FLAGS "-w")

target_link_libraries(lse_core PUBLIC
    spdlog::spdlog
    unofficial::sqlite3::sqlite3
    pugixml::pugixml
    CURL::libcurl
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
    uchardet::libuchardet
    $<IF:$<TARGET_EXISTS:MINIZIP::minizip>,MINIZIP::minizip,MINIZIP::minizipstatic>
    xxHash::xxhash
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