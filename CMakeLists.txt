cmake_minimum_required(VERSION 3.14)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMNADS ON)

project(VulkanRenderer VERSION 0.0.1 LANGUAGES CXX)


find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(glfw3 REQUIRED FATAL_ERROR)
find_package(spdlog REQUIRED FATAL_ERROR)
find_package(glm REQUIRED FATAL_ERROR)
find_package(volk REQUIRED FATAL_ERROR)
find_package(stb REQUIRED FATAL_ERROR)
find_package(shaderc REQUIRED FATAL_ERROR)

include(${CMAKE_SOURCE_DIR}/cmake/conanbuildinfo.cmake)
conan_basic_setup()

add_subdirectory(VulkanRenderer/)
add_subdirectory(Dependencies/)

# Link dependencies to VulkanRenderer
target_link_libraries(
    VulkanRenderer 
    PRIVATE ${CONAN_LIBS}
    PRIVATE imgui)

set(CMAKE_BUILD_TYPE debug)
