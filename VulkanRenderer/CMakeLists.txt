cmake_minimum_required(VERSION 3.14)

add_executable(
    VulkanRenderer

    # PCH
    ${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp

    # Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Entrypoint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/UUID.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Window.cpp

    # Debug
    ${CMAKE_CURRENT_SOURCE_DIR}/Debug/Logger.cpp

    # Graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/Buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/Device.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/Pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/Renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/MaterialSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/Mesh.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/Shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/Texture.cpp

    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils/CVar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Stringifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Timer.cpp
)

set_target_properties(
    VulkanRenderer PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    VulkanRenderer
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/imgui/
    PRIVATE ${CMAKE_SOURCE_DIR}/AssetParser/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/VulkanMemoryAllocator/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/spirv-reflect/
)

target_link_libraries(
    VulkanRenderer 
    PRIVATE imgui
    PRIVATE AssetParser
    PRIVATE spirv-reflect-static
    PRIVATE ${CONAN_LIBS}
)

target_precompile_headers(
    VulkanRenderer 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp
)

if(UNIX)
    target_compile_definitions(
        VulkanRenderer
        PRIVATE VULKANRENDERER_PLATFORM=LINUX
    )
elseif(WIN32)
    target_compile_definitions(
        VulkanRenderer
        PRIVATE VULKANRENDERER_PLATFORM=WINDOWS
    )
endif()
