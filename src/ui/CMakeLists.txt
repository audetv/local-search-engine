qt_add_executable(local-search-engine-ui
    main.cpp
)

qt_add_qml_module(local-search-engine-ui
    URI "LocalSearchEngine"
    VERSION 1.0
    QML_FILES
        main.qml
        SearchBar.qml
        ResultDelegate.qml
    RESOURCES
        qml.qrc
)

target_link_libraries(local-search-engine-ui PRIVATE
    lse_core
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
)

set_target_properties(local-search-engine-ui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
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