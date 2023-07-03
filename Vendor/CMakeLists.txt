cmake_minimum_required(VERSION 3.14)

# Dependencies
add_subdirectory(./entt)

add_subdirectory(./glfw)
target_compile_options(glfw PRIVATE -O3 -DNDEBUG -w)
target_link_options(glfw PRIVATE -O3 -DNDEBUG -w)

add_subdirectory(./glm)
target_compile_options(glm INTERFACE -O3 -DNDEBUG -w)
target_link_options(glm INTERFACE -O3 -DNDEBUG -w)

add_subdirectory(./lz4/build/cmake/)
target_compile_options(lz4_static  PRIVATE -O3 -DNDEBUG -w)
target_link_options(lz4_static  PRIVATE -O3 -DNDEBUG -w)

add_subdirectory(./nlohmann/)
target_compile_options(nlohmann_json INTERFACE -O3 -DNDEBUG -w)
target_link_options(nlohmann_json  INTERFACE  -O3 -DNDEBUG -w)

add_subdirectory(./spirv-reflect/)
target_compile_options(spirv-reflect-static PRIVATE -O3 -DNDEBUG -w)
target_link_options(spirv-reflect-static PRIVATE -O3 -DNDEBUG -w)

add_subdirectory(./spdlog)
target_compile_options(spdlog PRIVATE -O3 -DNDEBUG -w)
target_link_options(spdlog PRIVATE -O3 -DNDEBUG -w)

add_subdirectory(./tinygltf)
target_compile_options(tinygltf PRIVATE -O3 -DNDEBUG -w)
target_link_options(tinygltf PRIVATE -O3 -DNDEBUG -w)

option(TRACY_ENABLE "" ON)
add_subdirectory(./tracy)
target_compile_options(TracyClient  PRIVATE -O3 -DNDEBUG -w)
target_link_options(TracyClient  PRIVATE -O3 -DNDEBUG -w)

add_subdirectory(./meshoptimizer)

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

target_compile_options(ktx PRIVATE -O3 -DNDEBUG -w)
target_link_options(ktx PRIVATE -O3 -DNDEBUG -w)

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

target_compile_options(imgui PRIVATE -O3 -DNDEBUG -w)
target_link_options(imgui PRIVATE -O3 -DNDEBUG -w)

target_include_directories(
    imgui
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/glm/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/imgui
)
