cmake_minimum_required(VERSION 3.14)

# ___ IMGUI ___
file(GLOB IMGUI_FILES true ABSOLUTE
${CMAKE_CURRENT_SOURCE_DIR} imgui/*.cpp imgui/*.h)

set(BACKENDS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui)

file(GLOB IMGUI_BACKEND_FILES true ABSOLUTE 
${BACKENDS_DIR}imgui_impl_vulkan.cpp ${BACKENDS_DIR}imgui_impl_vulkan.h
${BACKENDS_DIR}imgui_impl_glfw.cpp ${BACKENDS_DIR}imgui_impl_glfw.h)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/volk/
)

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)
include_directories(${DEPENDENCIES_DIR}GLAD/include)
include_directories(${DEPENDENCIES_DIR}GLFW/include)

add_library(imgui STATIC ${IMGUI_FILES} ${IMGUI_BACKEND_FILES})
