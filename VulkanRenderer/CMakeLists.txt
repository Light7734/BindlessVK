cmake_minimum_required(VERSION 3.14)

if(MSVC)
    add_compile_options(/MP)
    add_compile_options(/W0)
endif()

file(GLOB_RECURSE RENDERER_FILES true ABSOLUTE ${CMAKE_BINARY_DIR}/../VulkanRenderer/src/*)

include_directories(
    ${CMAKE_BINARY_DIR}/../VulkanRenderer/src/
    
    ${CMAKE_BINARY_DIR}/../Dependencies/glfw/include
    ${CMAKE_BINARY_DIR}/../Dependencies/spdlog/include
    ${CMAKE_BINARY_DIR}/../Dependencies/volk/
    
    $ENV{VULKAN_SDK}/Include/

)

source_group(TREE ${CMAKE_BINARY_DIR}/../VulkanRenderer/ FILES ${RENDERER_FILES})
add_executable(VulkanRenderer ${RENDERER_FILES})

target_precompile_headers(VulkanRenderer PRIVATE ${CMAKE_BINARY_DIR}/../VulkanRenderer/src/pch.h)