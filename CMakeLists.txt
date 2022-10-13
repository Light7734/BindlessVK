cmake_minimum_required(VERSION 3.14)
project(VulkanRenderer LANGUAGES CXX)

set(SPIRV_REFLECT_STATIC_LIB ON)
set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_EXAMPLES OFF)
OPTION(SPIRV_REFLECT_STATIC_LIB ON)
OPTION(SPIRV_REFLECT_EXECUTABLE OFF)
OPTION(SPIRV_REFLECT_EXAMPLES OFF)

add_subdirectory(Vendor/)

add_subdirectory(AssetBaker/)
add_subdirectory(AssetParser/)
add_subdirectory(VulkanRenderer/)

set(CMAKE_BUILD_TYPE DEBUG)
