file(GLOB imgui_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# use glfw + opengl3 as backend
add_library(imgui STATIC
    ${imgui_sources}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

target_include_directories(imgui
    PUBLIC
        ${3rdparty_include_dir}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

target_link_libraries(imgui
    PUBLIC
        glfw
        ${essential_libs}
)