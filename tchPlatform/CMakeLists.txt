add_library(tchPlatform STATIC)

collect_header_files("inc" tch_platform_header_dirs tch_platform_headers)
collect_sources_files("src/win32" tch_platform_win32_sources)
collect_sources_files("src/unix" tch_platform_unix_sources)

if (UNIX)
    set(tch_platform_sources ${tch_platform_unix_sources})
elseif(WIN32)
    set(tch_platform_sources ${tch_platform_win32_sources})
else()
    message("## unsupported platform for tchPlatform module!")
endif()

target_sources(tchPlatform PRIVATE ${tch_platform_sources} ${tch_platform_headers})
target_include_directories(tchPlatform
    PUBLIC
        ${tch_platform_header_dirs}
        ${3rdparty_include_dir}
        ${sysconfig_dir}
)
target_link_directories(tchPlatform
    PUBLIC
        ${3rdparty_lib_dir}
)
target_link_libraries(tchPlatform
    PUBLIC
        tchGeneral
        ${3rdparty_libs}
        ${essential_libs}
        general_cxx_compiler_flags
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${tch_platform_sources} ${tch_platform_headers})