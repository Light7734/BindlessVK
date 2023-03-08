cmake_minimum_required(VERSION 3.14)

# Dependencies
add_subdirectory(./entt)
add_subdirectory(./glfw)
add_subdirectory(./glm)
add_subdirectory(./lz4/build/cmake/)
add_subdirectory(./nlohmann/)
add_subdirectory(./spirv-reflect/)
add_subdirectory(./spdlog)
add_subdirectory(./tinygltf)
add_subdirectory(./fmt)

# Configuration files
configure_file(${CMAKE_SOURCE_DIR}/Vendor/configs/imconfig.h ${CMAKE_SOURCE_DIR}/Vendor/configs/imconfig.h)

# KTX (Khronos texture)
add_library(
    ktx SHARED

    ${CMAKE_SOURCE_DIR}/Vendor/ktx/lib/texture.c
    ${CMAKE_SOURCE_DIR}/Vendor/ktx/lib/hashlist.c
    ${CMAKE_SOURCE_DIR}/Vendor/ktx/lib/checkheader.c
    ${CMAKE_SOURCE_DIR}/Vendor/ktx/lib/swap.c
    ${CMAKE_SOURCE_DIR}/Vendor/ktx/lib/memstream.c
    ${CMAKE_SOURCE_DIR}/Vendor/ktx/lib/filestream.c
)

# Imgui does not provide a CMakeLists.txt so we do it ourselves here
add_library(
    imgui SHARED 

    ${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/Vendor/imgui/imgui_widgets.cpp

    ${CMAKE_SOURCE_DIR}/Vendor/imgui/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(
    imgui
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/glm/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/imgui
)
