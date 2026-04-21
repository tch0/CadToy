collect_header_files("inc" tch_database_header_dirs tch_database_headers)
collect_sources_files("src" tch_database_sources)

add_library(tchDatabase STATIC ${tch_database_sources} ${tch_database_headers})

target_include_directories(tchDatabase
    PUBLIC
        ${tch_database_header_dirs}
        ${3rdparty_include_dir}
)
target_link_directories(tchDatabase
    PUBLIC
        ${3rdparty_lib_dir}
)
target_link_libraries(tchDatabase
    PUBLIC
        tchGeometry
        tchInterface
        general_cxx_compiler_flags
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${tch_database_sources} ${tch_database_headers})