cmake_minimum_required(VERSION 3.14)
project(BindlessVk LANGUAGES C CXX)

include_directories(
    ${CMAKE_SOURCE_DIR}/Vendor/ktx/include
    ${CMAKE_SOURCE_DIR}/Vendor/ktx/other_include
)

set(SPIRV_REFLECT_STATIC_LIB ON)
set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_EXAMPLES OFF)
OPTION(SPIRV_REFLECT_STATIC_LIB ON)
OPTION(SPIRV_REFLECT_EXECUTABLE OFF)
OPTION(SPIRV_REFLECT_EXAMPLES OFF)

add_subdirectory(Vendor/)

add_subdirectory(ABC/)
add_subdirectory(AssetBaker/)
add_subdirectory(AssetParser/)
add_subdirectory(BindlessVk/)

target_compile_options(ABC PRIVATE -O3 -DNDEBUG -w)
target_link_options(ABC PRIVATE -O3 -DNDEBUG -w)

target_compile_options(AssetBaker PRIVATE -O3 -DNDEBUG -w)
target_link_options(AssetBaker PRIVATE -O3 -DNDEBUG -w)

target_compile_options(AssetParser PRIVATE -O3 -DNDEBUG -w )
target_link_options(AssetParser PRIVATE -O3 -DNDEBUG -w)

target_compile_options(BindlessVk PRIVATE -g -Og)
target_link_options(BindlessVk PRIVATE -g -Og)

add_subdirectory(Examples/)
