cmake_minimum_required(VERSION 3.14)
project(VulkanRenderer LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/build/conanbuildinfo.cmake)
conan_basic_setup()

add_subdirectory(Vendor/configurations/imgui)
add_subdirectory(Vendor/lz4/build/cmake)
add_subdirectory(Vendor/nlohmann/)

add_subdirectory(AssetBaker/)
add_subdirectory(AssetParser/)
add_subdirectory(VulkanRenderer/)
