add_library(ImFileDialog STATIC
    src/ImFileDialog.cpp
    inc/ImFileDialog.h
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
        general_cxx_compiler_flags
)