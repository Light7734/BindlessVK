cmake_minimum_required(VERSION 3.14)

####### FRAMEWORK #######
add_library(
    Framework

    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/Core/Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/Core/Window.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/Utils/CVar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/Utils/Logger.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/Pools/ThreadPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/Pools/StagingPool.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/UserInterface/ImguiVulkanBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Framework/src/UserInterface/ImguiGlfwBackend.cpp

)

target_compile_options(Framework PRIVATE -g -Og -w)
target_link_options(Framework PRIVATE -g -Og -w)

set_target_properties(
    Framework PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    Framework
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Framework/include
    PUBLIC ${CMAKE_SOURCE_DIR}/ABC/include/
    PUBLIC ${CMAKE_SOURCE_DIR}/BindlessVk/include
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/entt/single_include/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/glfw/include/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/imgui/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/VulkanMemoryAllocator/include/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/include/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/tracy/public/
)

target_link_libraries(
    Framework
    PRIVATE ABC
    PUBLIC BindlessVk
    PUBLIC vulkan
    PUBLIC glfw
    PUBLIC imgui
    PUBLIC TracyClient
)

target_precompile_headers(
    Framework
    PRIVATE ${CMAKE_SOURCE_DIR}/ABC/pch.hpp
)

####### RENDERGRAPHS #######
add_library(
    Rendergraphs

    ${CMAKE_CURRENT_SOURCE_DIR}/Rendergraphs/Graph.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/Rendergraphs/Passes/Forward.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendergraphs/Passes/UserInterface.cpp
)

target_compile_options(Rendergraphs PRIVATE -g -Og -w)
target_link_options(Rendergraphs PRIVATE -g -Og -w)

set_target_properties(
    Rendergraphs PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    Rendergraphs
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Framework/include
    PUBLIC ${CMAKE_SOURCE_DIR}/BindlessVk/include
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/entt/single_include/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/glfw/include/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/imgui/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/VulkanMemoryAllocator/include/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/include/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/tracy/public/
)

target_link_libraries(
    Rendergraphs
    PRIVATE ABC
    PUBLIC Framework
)

target_precompile_headers(
    Rendergraphs 
    PRIVATE ${CMAKE_SOURCE_DIR}/ABC/pch.hpp
)

####### EXAMPLES #######
add_executable(
    Example_Development

    ${CMAKE_CURRENT_SOURCE_DIR}/Development/Development.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Development/Entrypoint.cpp
)

target_compile_options(Example_Development PRIVATE -g -Og -w)
target_link_options(Example_Development  PRIVATE -g -Og -w)

set_target_properties(
    Example_Development PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    Example_Development

    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Development/
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Framework/include
    PRIVATE ${CMAKE_SOURCE_DIR}/BindlessVk/include
    PRIVATE ${CMAKE_SOURCE_DIR}/AssetParser/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/VulkanMemoryAllocator/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/vma-hpp/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/spirv-reflect/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/tinygltf/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/entt/single_include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/imgui/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/glfw/include/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/tracy/public/
)

target_link_libraries(
    Example_Development

    PRIVATE ABC
    PRIVATE AssetParser
    PRIVATE BindlessVk
    PRIVATE Framework

    PRIVATE vulkan
    PRIVATE ktx
    PRIVATE glfw
    PRIVATE imgui
    PRIVATE tinygltf
    PRIVATE spirv-reflect-static

    PRIVATE Rendergraphs
)

target_precompile_headers(
    Example_Development
    PRIVATE ${CMAKE_SOURCE_DIR}/ABC/pch.hpp
)

####### TESTS #######
