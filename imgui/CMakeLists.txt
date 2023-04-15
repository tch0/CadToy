file(GLOB imgui_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB imgui_headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# use glfw + opengl3 as backend
list(APPEND imgui_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)
list(APPEND imgui_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl3.h
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_glfw.h
)

add_library(imgui STATIC
    ${imgui_sources}
    ${imgui_headers}
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

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${imgui_headers})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${imgui_sources})