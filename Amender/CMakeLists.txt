### Amender is a logger/instrumentor/profiler that helps us amend the messes we made in 
### this project

### She should not leak into the end-user's code because her code lives in the global namespace
### for convenience

add_library(
    Amender
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Profiler.cpp
)

set_target_properties(
    Amender PROPERTIES
    CXX_STANDARD 20
)

target_include_directories(
    Amender
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/imgui/
    PUBLIC ${CMAKE_SOURCE_DIR}/Vendor/spdlog/include/
)

target_link_libraries(
    Amender
    PRIVATE spdlog
    PRIVATE imgui
)
