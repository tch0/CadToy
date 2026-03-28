collect_header_files("inc" tch_main_header_dirs tch_main_headers)
collect_sources_files("src" tch_main_sources)

add_executable(tchCadToy ${tch_main_sources} ${tch_main_headers})

target_include_directories(tchCadToy
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${3rdparty_include_dir}
        ${sysconfig_dir}
)
target_link_directories(tchCadToy
    PUBLIC
        ${3rdparty_lib_dir}
)
target_link_libraries(tchCadToy
    PUBLIC
        tchGeneral
        tchPlatform
        ${3rdparty_libs}
        ${essential_libs}
        general_cxx_compiler_flags
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${tch_main_sources} ${tch_main_headers})

# For Windows: compile to Win32 GUI application for Release config, console application for Debug config
if (WIN32)
    # for MinGW/cl/clang-cl, WIN32_EXECUTABLE is true means GUI application, false means console application
    set_target_properties(tchCadToy PROPERTIES
        WIN32_EXECUTABLE $<BOOL:$<CONFIG:Release>>
    )
    # for cl/clang-cl, need specify mainCRTStartup as entry additionally
    if (${CXX_COMPILER_IS_MSVC_LIKE})
        target_link_options(tchCadToy PRIVATE
            $<$<BOOL:$<CONFIG:Release>>:$<BUILD_INTERFACE:
                /ENTRY:mainCRTStartup
            >>
        )
    endif()
endif()
# UNIX/Linux/MacOS do not need such configuration

#-------------------------------------------------------------------------------------#
#            copy resources to build directory
#-------------------------------------------------------------------------------------#
copy_dir_to_target_file_dir(tchCadToy fonts ${CMAKE_SOURCE_DIR}/fonts)
copy_dir_to_target_file_dir(tchCadToy res/lang ${CMAKE_SOURCE_DIR}/res/lang)