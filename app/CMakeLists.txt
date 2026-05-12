# Dear ImGui library (no CMakeLists provided, so we build it ourselves)
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

add_executable(mathapp
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
)

target_include_directories(mathapp PRIVATE ${CMAKE_SOURCE_DIR}/third_party)

target_link_libraries(mathapp PRIVATE
    mathengine
    imgui
    OpenGL::GL
)

if(APPLE)
    target_link_libraries(mathapp PRIVATE "-framework Cocoa" "-framework IOKit")
endif()
