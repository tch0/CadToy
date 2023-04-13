collect_header_dirs("inc" tch_main_header_dirs)
collect_sources_files("src" tch_main_sources)

add_executable(tchMain ${tch_main_sources})

target_include_directories(tchMain
    PUBLIC
        ${tch_main_header_dirs}
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
