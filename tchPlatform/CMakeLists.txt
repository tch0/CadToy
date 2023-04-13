add_library(tchPlatform STATIC)

collect_header_dirs("inc" tch_platform_header_dirs)
collect_sources_files("src/win32" tch_platform_win32_sources)
collect_sources_files("src/unix" tch_platform_unix_sources)

if (UNIX)
    target_sources(tchPlatform PRIVATE ${tch_platform_unix_sources})
elseif(WIN32)
    target_sources(tchPlatform PRIVATE ${tch_platform_win32_sources})
else()
    message("## unsupported platform for tchPlatform module!")
endif()

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