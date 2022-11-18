cmake_minimum_required(VERSION 3.14)

add_library(
    Framework

    # Core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Window.cpp

    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/CVar.cpp
)

set_target_properties(
    Framework PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    Framework
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include

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
