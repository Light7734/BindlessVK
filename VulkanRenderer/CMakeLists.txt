cmake_minimum_required(VERSION 3.14)

if(MSVC)
    add_compile_options(/MP)
    add_compile_options(/W0)
endif()

if(UNIX)
    add_definitions(-DVULKANRENDERER_PLATFORM_LINUX)
elseif(WIN32)
    add_definitions(-DVULKANRENDERER_PLATFORM_WINDOWS)
endif()

file(GLOB_RECURSE RENDERER_FILES true ABSOLUTE ${CMAKE_BINARY_DIR}/../VulkanRenderer/*)
list(REMOVE_ITEM RENDERER_FILES ${CMAKE_BINARY_DIR}/../VulkanRenderer/res/viking_room.obj)


include_directories(
    ${CMAKE_SOURCE_DIR}/VulkanRenderer/src/
    
    ${CMAKE_SOURCE_DIR}/Dependencies/glfw/include
    ${CMAKE_SOURCE_DIR}/Dependencies/spdlog/include
    ${CMAKE_SOURCE_DIR}/Dependencies/volk/
    ${CMAKE_SOURCE_DIR}/Dependencies/glm/
    ${CMAKE_SOURCE_DIR}/Dependencies/tinyobjloader/

    ${CMAKE_SOURCE_DIR}/Dependencies/stb_image/

    ${CMAKE_BINARY_DIR}/../Dependencies/shaderc/libshaderc/include
    ${CMAKE_}/../Dependencies/shaderc/libshaderc/include
    
    $ENV{VULKAN_SDK}/Include/    
)


source_group(TREE ${CMAKE_BINARY_DIR}/../VulkanRenderer/ FILES ${RENDERER_FILES})
add_executable(VulkanRenderer ${RENDERER_FILES})

target_precompile_headers(VulkanRenderer PRIVATE ${CMAKE_BINARY_DIR}/../VulkanRenderer/src/pch.h)
