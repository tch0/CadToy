collect_header_files("inc" tch_main_header_dirs tch_main_headers)
collect_sources_files("src" tch_main_sources)

add_executable(tchMain ${tch_main_sources} ${tch_main_headers})

target_include_directories(tchMain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${3rdparty_include_dir}
        ${sysconfig_dir}
)
target_link_directories(tchMain
    PUBLIC
        ${3rdparty_lib_dir}
)
target_link_libraries(tchMain
    PUBLIC
        tchGeneral
        tchPlatform
        ${3rdparty_libs}
        ${essential_libs}
        general_cxx_compiler_flags
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${tch_main_sources} ${tch_main_headers})