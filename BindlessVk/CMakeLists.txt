cmake_minimum_required(VERSION 3.14)

add_library(
    BindlessVk

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MaterialSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ModelLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ModelLoaders/GltfLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RenderGraph.cpp

)

set_target_properties(
    BindlessVk PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    BindlessVk
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/

    PRIVATE ${CMAKE_SOURCE_DIR}/AssetParser/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/VulkanMemoryAllocator/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/spirv-reflect/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/tinygltf/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/fmt/include
)

target_link_libraries(
    BindlessVk 
    PRIVATE AssetParser
    PRIVATE vulkan
    PRIVATE ktx
    PRIVATE tinygltf
    PRIVATE spirv-reflect-static
    PRIVATE spdlog
    PRIVATE fmt
)

target_precompile_headers(
    BindlessVk 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp
)
