add_library(
    ABC  
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Logger.cpp
)

set_target_properties(
    ABC PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    ABC
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/imgui/
    PRIVATE ${CMAKE_SOURCE_DIR}/Vendor/spdlog/include/
)

target_link_libraries(
    ABC
    PRIVATE spdlog
    PRIVATE imgui
)
