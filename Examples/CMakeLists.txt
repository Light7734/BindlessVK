cmake_minimum_required(VERSION 3.14)

####### FRAMEWORK #######
add_library(
    Framework

    # Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/Core/Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/Core/Common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/Core/Window.cpp

    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/Utils/CVar.cpp
)

set_target_properties(
    Framework PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    Framework
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Framework/include

    PRIVATE ${CMAKE_SOURCE_DIR}/BindlessVk/include

    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/entt/single_include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/glfw/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/imgui/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/spdlog/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/VulkanMemoryAllocator/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/include/
)

target_link_libraries(
    Framework
    PRIVATE BindlessVk
    PRIVATE vulkan
    PRIVATE glfw
    PRIVATE spdlog
    PRIVATE imgui
)

target_precompile_headers(
    Framework
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp
)

####### EXAMPLES #######
add_executable(
    Example_Development

    ${CMAKE_CURRENT_SOURCE_DIR}/Example_Development/Entrypoint.cpp
)

set_target_properties(
    Example_Development PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    Example_Development

    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Development/
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Framework/include

    PRIVATE ${CMAKE_SOURCE_DIR}/BindlessVk/include

    PRIVATE ${CMAKE_SOURCE_DIR}/AssetParser/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/VulkanMemoryAllocator/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/spirv-reflect/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/tinygltf/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/entt/single_include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/imgui/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/glfw/include/
)

target_link_libraries(
    Example_Development
    PRIVATE AssetParser
    PRIVATE BindlessVk
    PRIVATE Framework

    PRIVATE vulkan
    PRIVATE ktx
    PRIVATE glfw
    PRIVATE imgui
    PRIVATE tinygltf
    PRIVATE spirv-reflect-static
    PRIVATE spdlog
)

target_precompile_headers(
    Example_Development
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp
)


####### TESTS #######
