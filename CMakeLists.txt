cmake_minimum_required(VERSION 3.14)
project(VulkanRenderer LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/build/conanbuildinfo.cmake)
conan_basic_setup()

add_subdirectory(VulkanRenderer/)
add_subdirectory(Vendor/configurations/imgui)
