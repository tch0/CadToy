collect_header_files("inc" tch_geometry_header_dirs tch_geometry_headers)
collect_sources_files("src" tch_general_sources)

add_library(tchGeometry STATIC ${tch_general_sources} ${tch_geometry_headers})

target_include_directories(tchGeometry
    PUBLIC
        ${tch_geometry_header_dirs}
        ${3rdparty_include_dir}
        ${sysconfig_dir}
)
target_link_directories(tchGeometry
    PUBLIC
        ${3rdparty_lib_dir}
)
# 只依赖glm，glm是header only，不需要链接
target_link_libraries(tchGeometry
    PUBLIC
        general_cxx_compiler_flags
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${tch_general_sources} ${tch_geometry_headers})