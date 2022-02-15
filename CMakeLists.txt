cmake_minimum_required(VERSION 3.14)

project(VulkanRenderer VERSION 0.0.1 LANGUAGES CXX)

add_subdirectory(VulkanRenderer/)
add_subdirectory(Dependencies/)

set(CMAKE_EXPORT_COMPILE_COMMNADS ON)

# Link dependencies to VulkanRenderer
target_link_libraries(
    VulkanRenderer 
    PRIVATE spdlog
    PRIVATE glfw
    PRIVATE volk
)

set(CMAKE_BUILD_TYPE debug)
