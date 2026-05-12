add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

find_package(OpenGL REQUIRED)

set(MATHAPP_SOURCES
    main.cpp
    app.cpp
    graph_renderer.cpp
    input_panel.cpp
    result_panel.cpp
    renderer3d.cpp
    console.cpp
    design.cpp
    tools.cpp
    datalab.cpp
    implicit.cpp
    file_picker_win32.cpp
)

if(APPLE)
    # macOS .app bundle
    set(MATHAPP_ICON ${CMAKE_SOURCE_DIR}/packaging/MathEngine.icns)
    set_source_files_properties(${MATHAPP_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    add_executable(mathapp MACOSX_BUNDLE ${MATHAPP_SOURCES} ${MATHAPP_ICON})
    set_target_properties(mathapp PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "MathEngine"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.theomirzakhanian.mathengine"
        MACOSX_BUNDLE_INFO_STRING "MathEngine v1.0 - desktop math engine"
        MACOSX_BUNDLE_COPYRIGHT "© 2026 Theo Mirzakhanian"
        MACOSX_BUNDLE_ICON_FILE "MathEngine.icns"
    )
elseif(WIN32)
    add_executable(mathapp WIN32 ${MATHAPP_SOURCES})
    # Use console subsystem during development; WIN32 hides console
else()
    add_executable(mathapp ${MATHAPP_SOURCES})
endif()

target_include_directories(mathapp PRIVATE ${CMAKE_SOURCE_DIR}/third_party)

target_link_libraries(mathapp PRIVATE
    mathengine
    imgui
    OpenGL::GL
)

if(APPLE)
    target_link_libraries(mathapp PRIVATE "-framework Cocoa" "-framework IOKit")
elseif(WIN32)
    target_link_libraries(mathapp PRIVATE comdlg32 user32 gdi32 shell32)
endif()

install(TARGETS mathapp
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
