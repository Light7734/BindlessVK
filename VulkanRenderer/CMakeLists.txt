cmake_minimum_required(VERSION 3.14)

if(UNIX)
    add_definitions(-DVULKANRENDERER_PLATFORM_LINUX)
elseif(WIN32)
    add_definitions(-DVULKANRENDERER_PLATFORM_WINDOWS)
endif()

set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp
    # Core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Entrypoint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Window.cpp
    # Debug
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Debug/Logger.cpp
    # Graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Device.cpp
    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/Stringifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/Timer.cpp
    )
add_executable(VulkanRenderer ${SRC_FILES})

set_property(TARGET VulkanRenderer PROPERTY CXX_STANDARD 20)
target_include_directories(VulkanRenderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/Dependencies/glfw/include/
    ${CMAKE_SOURCE_DIR}/Dependencies/spdlog/include/
    ${CMAKE_SOURCE_DIR}/Dependencies/volk/)
target_precompile_headers(VulkanRenderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
