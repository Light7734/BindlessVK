cmake_minimum_required(VERSION 3.14)

project(VulkanRenderer VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(Dependencies/volk)
add_subdirectory(Dependencies/spdlog)
add_subdirectory(Dependencies/glfw)
add_subdirectory(VulkanRenderer/)

target_link_libraries(VulkanRenderer glfw)
target_link_libraries(VulkanRenderer spdlog)
target_link_libraries(VulkanRenderer volk)