cmake_minimum_required(VERSION 3.14)

add_library(
    imgui SHARED 

    # Imgui
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_widgets.cpp

    # Backends
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_glfw.cpp
)

target_include_directories(
    imgui
    PRIVATE ${CMAKE_SOURCE_DIR}/Dependencies/glm/
    PRIVATE ${CMAKE_SOURCE_DIR}/Dependencies/imgui
)

target_link_libraries(
    imgui
    PRIVATE volk
)
