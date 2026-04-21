collect_header_files("inc" tch_interface_header_dirs tch_interface_headers)
# 预留源文件收集相关配置
collect_sources_files("src" tch_interface_sources)

# 目前没有任何源文件，所以定义为INTERFACE库，并不会生成库文件，后续如果添加源文件需要修改为STATIC
add_library(tchInterface INTERFACE ${tch_interface_sources} ${tch_interface_headers})

target_include_directories(tchInterface
    INTERFACE
        ${tch_interface_header_dirs}
        ${3rdparty_include_dir}
)
target_link_libraries(tchInterface
    INTERFACE
        general_cxx_compiler_flags
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${tch_interface_sources} ${tch_interface_headers})