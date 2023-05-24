cmake_minimum_required(VERSION 3.14)

add_library(
    BindlessVk

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Allocators/Descriptors/DescriptorAllocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Allocators/Descriptors/DescriptorPool.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Allocators/LayoutAllocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Allocators/MemoryAllocator.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Buffers/Buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Buffers/FragmentedBuffer.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Context/DebugUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Context/Device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Context/Gpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Context/Instance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Context/Queues.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Context/Surface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Context/Swapchain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Context/VkContext.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Material/MaterialSystem.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Model/Model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Model/ModelLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Model/Loaders/GltfLoader.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/RenderNode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Rendergraph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/RenderResources.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Shader/Shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Shader/DescriptorSet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Shader/ShaderLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Shader/Loaders/SpvLoader.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Texture/Image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Texture/Texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Texture/TextureLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Texture/Loaders/KtxLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Texture/Loaders/BinaryLoader.cpp
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
