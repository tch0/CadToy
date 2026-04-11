file(GLOB ImFileDialog_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB ImFileDialog_headers ${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h)

add_library(ImFileDialog STATIC
    ${ImFileDialog_sources}
    ${ImFileDialog_headers}
)

target_include_directories(ImFileDialog
    PUBLIC
        ${3rdparty_include_dir}
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(ImFileDialog
    PUBLIC
        imgui
        stb
        ${essential_libs}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${ImFileDialog_headers})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${ImFileDialog_sources})