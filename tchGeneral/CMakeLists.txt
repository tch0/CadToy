collect_header_files("inc" tch_general_header_dirs tch_general_headers)
collect_sources_files("src" tch_general_sources)

add_library(tchGeneral STATIC ${tch_general_sources} ${tch_general_headers})

target_include_directories(tchGeneral
    PUBLIC
        ${tch_general_header_dirs}
        ${3rdparty_include_dir}
        ${sysconfig_dir}
)
target_link_directories(tchGeneral
    PUBLIC
        ${3rdparty_lib_dir}
)
target_link_libraries(tchGeneral
    PUBLIC
        ${3rdparty_libs}
        ${essential_libs}
        general_cxx_compiler_flags
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${tch_general_sources} ${tch_general_headers})