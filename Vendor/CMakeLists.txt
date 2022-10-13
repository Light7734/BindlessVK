# Dependencies
add_subdirectory(./entt)
add_subdirectory(./glfw)
add_subdirectory(./glm)
add_subdirectory(./lz4/build/cmake/)
add_subdirectory(./nlohmann/)
add_subdirectory(./spirv-reflect/)
add_subdirectory(./spdlog)
add_subdirectory(./tinygltf)

# Configuration files
configure_file(${CMAKE_SOURCE_DIR}/Vendor/configs/imconfig.h ${CMAKE_SOURCE_DIR}/Vendor/configs/imconfig.h)

# Imgui does not provide a CMakeLists.txt so we do it ourselves here
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

