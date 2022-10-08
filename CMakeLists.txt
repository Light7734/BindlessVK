cmake_minimum_required(VERSION 3.14)
project(VulkanRenderer LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/build/conanbuildinfo.cmake)
conan_basic_setup()

set(SPIRV_REFLECT_STATIC_LIB ON)
set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_EXAMPLES OFF)
OPTION(SPIRV_REFLECT_STATIC_LIB ON)
OPTION(SPIRV_REFLECT_EXECUTABLE OFF)
OPTION(SPIRV_REFLECT_EXAMPLES OFF)

add_subdirectory(Vendor/configurations/imgui/)
add_subdirectory(Vendor/lz4/build/cmake/)
add_subdirectory(Vendor/spirv-reflect/)
add_subdirectory(Vendor/nlohmann/)

add_subdirectory(AssetBaker/)
add_subdirectory(AssetParser/)
add_subdirectory(VulkanRenderer/)


