cmake_minimum_required(VERSION 3.14)

project(VulkanRenderer VERSION 1.0.0 LANGUAGES CXX)

add_subdirectory(Dependencies/volk)
add_subdirectory(Dependencies/spdlog)
add_subdirectory(Dependencies/glfw)
add_subdirectory(Dependencies/glm)
add_subdirectory(Dependencies/stb_image)

set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_EXAMPLES ON)
set(SHADERC_ENABLE_WGSL_OUTPUT OFF)

add_subdirectory(Dependencies/shaderc)

add_subdirectory(VulkanRenderer/)
set_property(TARGET VulkanRenderer PROPERTY CXX_STANDARD 20)

target_link_libraries(VulkanRenderer glfw)
target_link_libraries(VulkanRenderer spdlog)
target_link_libraries(VulkanRenderer volk)
target_link_libraries(VulkanRenderer shaderc)
target_link_libraries(VulkanRenderer stb_image)

configure_file(${CMAKE_SOURCE_DIR}/DependenciesConfig/spdlog/tweakme.h.in
               ${CMAKE_SOURCE_DIR}/Dependencies/spdlog/include/spdlog/tweakme.h)

set_target_properties(VulkanRenderer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/VulkanRenderer)