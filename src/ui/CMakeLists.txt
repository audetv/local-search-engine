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

# --- АВТОМАТИЧЕСКОЕ КОПИРОВАНИЕ QT ПЛАГИНОВ (ТОЛЬКО ДЛЯ WINDOWS) ---
if(WIN32)
    # Находим папку с плагинами Qt
    set(QT_PLUGINS_SRC "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/Qt6/plugins")
    if(NOT EXISTS "${QT_PLUGINS_SRC}")
        set(QT_PLUGINS_SRC "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/plugins")
    endif()
    
    if(EXISTS "${QT_PLUGINS_SRC}/platforms")
        add_custom_command(TARGET local-search-engine-gui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt plugins for local-search-engine-gui..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${QT_PLUGINS_SRC}/platforms"
                "$<TARGET_FILE_DIR:local-search-engine-gui>/platforms"
            COMMENT "Copying Qt platforms plugin to output directory"
        )
        message(STATUS "Qt plugins will be deployed to local-search-engine-gui")
    else()
        message(WARNING "Qt plugins directory not found: ${QT_PLUGINS_SRC}")
    endif()
endif()