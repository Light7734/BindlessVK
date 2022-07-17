cmake_minimum_required(VERSION 3.14)

add_executable(
    VulkanRenderer

    # PCH
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp

    # Core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Entrypoint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/UUID.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Window.cpp

    # Debug
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Debug/Logger.cpp

    # Graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Texture.cpp

    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/Stringifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/Timer.cpp
)

set_target_properties(
    VulkanRenderer PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    VulkanRenderer
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/
    PRIVATE ${CMAKE_SOURCE_DIR}/Dependencies/imgui/
)

target_link_libraries(
    VulkanRenderer 
    PRIVATE imgui
    PRIVATE ${CONAN_LIBS}
)

target_precompile_headers(
    VulkanRenderer 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp
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
