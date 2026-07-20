find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(local-search-engine-gui
    main.cpp
    main_window.hpp
    main_window.cpp
    search_widget.hpp
    search_widget.cpp
    results_widget.hpp
    results_widget.cpp
)

target_link_libraries(local-search-engine-gui PRIVATE
    lse_core
    Qt6::Widgets
)

set_target_properties(local-search-engine-gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src"
)