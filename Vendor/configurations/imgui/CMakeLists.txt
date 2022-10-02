cmake_minimum_required(VERSION 3.14)

add_library(
    imgui SHARED 

    # Imgui
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_widgets.cpp

    # Backends
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/backends/imgui_impl_glfw.cpp

    # Misc
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/misc/cpp/imgui_stdlib.cpp

)

target_include_directories(
    imgui
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/glm/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/imgui
)

target_link_libraries(
    imgui
    PRIVATE volk
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h ${CMAKE_SOURCE_DIR}/Vendor/imgui/imconfig.h)
