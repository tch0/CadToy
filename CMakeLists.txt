#-------------------------------------------------------------------------------------#
#                    CMake version
#-------------------------------------------------------------------------------------#
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
message("## cmake: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")


#-------------------------------------------------------------------------------------#
#                    OS
#-------------------------------------------------------------------------------------#
if (APPLE)
    option(TCH_OS_MACOSX "MacOSX" ON)
    option(TCH_OS_UNIX "Unix" ON)
    message("## system: apple")

elseif(UNIX AND NOT APPLE)
    option(TCH_OS_LINUX "Liunx" ON)
    option(TCH_OS_UNIX "Unix" ON)
    message("## system: Linux")

elseif(WIN32)
    option(TCH_OS_WIN32 "Win32" ON)
    message("## system: windows")
endif()


#-------------------------------------------------------------------------------------#
#                    paths
#-------------------------------------------------------------------------------------#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/publish)


#-------------------------------------------------------------------------------------#
#                    Project
#-------------------------------------------------------------------------------------#
project(CadToy
    DESCRIPTION "A cross-platform CAD-Like toy program based on OpenGL."
    VERSION "0.1"
    LANGUAGES CXX
)

# some directories of project
message("## proejct: ${PROJECT_NAME} ${PROJECT_VERSION}")
message("## CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message("## CMAKE_ARCHIVE_OUTPUT_DIRECTORY: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
message("## CMAKE_LIBRARY_OUTPUT_DIRECTORY: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message("## CMAKE_RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

#-------------------------------------------------------------------------------------#
#                    System Config header
#-------------------------------------------------------------------------------------#
configure_file(
    ${CMAKE_SOURCE_DIR}/sysconfig/SysConfig.h.in
    ${CMAKE_BINARY_DIR}/sysconfig/SysConfig.h
)
set(sysconfig_dir ${CMAKE_BINARY_DIR}/sysconfig)


#-------------------------------------------------------------------------------------#
#             distinguish different compilers
#-------------------------------------------------------------------------------------#
set(CXX_COMPILER_IS_GCC OFF)            # gcc
set(CXX_COMPILER_IS_CLANG OFF)          # clang
set(CXX_COMPILER_IS_MSVC OFF)           # msvc
set(CXX_COMPILER_IS_CLANG_CL OFF)       # clang-cl
set(CXX_COMPILER_IS_GNU_LIKE OFF)       # gcc, clang, clang-cl
set(CXX_COMPILER_IS_GCC_CLANG OFF)      # gcc, clang
set(CXX_COMPILER_IS_CLANG_ALL OFF)      # clang, clang-cl
set(CXX_COMPILER_IS_MSVC_LIKE OFF)      # msvc, clang-cl
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CXX_COMPILER_IS_GCC ON)
    set(CXX_COMPILER_IS_GNU_LIKE ON)
    set(CXX_COMPILER_IS_GCC_CLANG ON)
    message("## Compiler: gcc")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (CMAKE_CXX_SIMULATE_ID MATCHES "MSVC" AND CMAKE_CL_64)
        set(CXX_COMPILER_IS_CLANG_CL ON)
        set(CXX_COMPILER_IS_GNU_LIKE ON)
        set(CXX_COMPILER_IS_CLANG_ALL ON)
        set(CXX_COMPILER_IS_MSVC_LIKE ON)
        message("## Compiler: clang-cl")
    else ()
        set(CXX_COMPILER_IS_CLANG ON)
        set(CXX_COMPILER_IS_GNU_LIKE ON)
        set(CXX_COMPILER_IS_GCC_CLANG ON)
        set(CXX_COMPILER_IS_CLANG_ALL ON)
        message("## Compiler: clang")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CXX_COMPILER_IS_MSVC ON)
    set(CXX_COMPILER_IS_MSVC_LIKE ON)
    message("## Compiler: msvc")
else ()
    message(FATAL_ERROR "Unsupported Compiler!")
endif ()


#-------------------------------------------------------------------------------------#
#            compiler flags
#-------------------------------------------------------------------------------------#
add_library(general_cxx_compiler_flags INTERFACE)
# C++ standard
target_compile_features(general_cxx_compiler_flags INTERFACE cxx_std_20)
# warning options for different toolchain
target_compile_options(general_cxx_compiler_flags
        INTERFACE
        # clang/clang-cl
        $<$<BOOL:${CXX_COMPILER_IS_CLANG_ALL}>:$<BUILD_INTERFACE:
            -Wall
            -Wextra

            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-language-extension-token      #- glad
            -Wno-documentation                 #- glfw3
            -Wno-documentation-unknown-command #- glfw3
            -Wno-nonportable-system-include-path
            -Wno-reserved-id-macro
            -Wno-global-constructors            #++ need ++
            -Wno-exit-time-destructors          #++ need ++
            -Wno-covered-switch-default         #++ need ++
            -Wno-switch-enum                    #++ need ++
            -Wno-unused-parameter               #++ need ++
            -Wno-unused-member-function         #++ need ++
            -Wno-missing-variable-declarations  #++ need ++
            -Wno-missing-prototypes
            -Wno-old-style-cast   #---- tmp ---- 

            -Wno-unused-variable #----- tmp ----
            -Wno-unused-private-field #----- tmp ----
            -Wno-unused-but-set-variable
        >>
        # gcc
        $<$<BOOL:${CXX_COMPILER_IS_GCC}>:$<BUILD_INTERFACE:
            -Wall
            -Wextra

            -Wno-switch-enum                    #++ need ++
            -Wno-unused-parameter               #++ need ++

            -Wno-old-style-cast   #---- tmp ---- 
            -Wno-unused-variable #----- tmp ----

            -Wconversion-null
            -Wno-unused-but-set-variable
        >>
        # msvc/clang-cl
        $<$<BOOL:${CXX_COMPILER_IS_MSVC_LIKE}>:$<BUILD_INTERFACE:
            /W3
            /source-charset:utf-8               #support UTF-8 source file, like Chinese comment
            /execution-charset:utf-8            #strange errors may occur if you write Chinese comment but lack of these 2 flags
        >>
)

# generate Position Independent Code for Linux system
set_target_properties(general_cxx_compiler_flags PROPERTIES 
    INTERFACE_POSITION_INDEPENDENT_CODE ON
)

#-------------------------------------------------------------------------------------#
#            macros that help to collect headers and sources
#-------------------------------------------------------------------------------------#
# collect header directories and header files
macro(collect_header_files dir_name return_dir_list return_header_list)
    set(header_list "")
    file(GLOB_RECURSE header_list
        ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/*.hpp
    )
    set(dir_list "")
    foreach(file_path ${header_list})
        get_filename_component(dir_path ${file_path} PATH)
        list(APPEND dir_list ${dir_path})
    endforeach()
    list(REMOVE_DUPLICATES dir_list)
    set(${return_dir_list} ${dir_list})
    set(${return_header_list} ${header_list})
endmacro()

macro(collect_sources_files dir_name return_list)
    file(GLOB_RECURSE ${return_list}
        ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/*.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/*.cxx
    )
endmacro()


#-------------------------------------------------------------------------------------#
#            Unity building setting
#-------------------------------------------------------------------------------------#
set (CMAKE_UNITY_BUILD_BATCH_SIZE 16)


#-------------------------------------------------------------------------------------#
#            3rd party libraries
#-------------------------------------------------------------------------------------#
# 3rdparty libraries
list(APPEND 3rdparty_libs glad glfw soil2 imgui)
# header-only libraries: glm rapidjson magic_enum

set(3rdparty_include_dir ${CMAKE_SOURCE_DIR}/3rdparty-install/include)
set(3rdparty_lib_dir ${CMAKE_SOURCE_DIR}/3rdparty-install/lib)


#-------------------------------------------------------------------------------------#
#            copy necessary files after every build (only when different)
#-------------------------------------------------------------------------------------#
function(copy_dir_to_target_file_dir target dirname source)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.26)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${source} $<TARGET_FILE_DIR:${target}>/${dirname} # copy_directory_if_different need cmake 3.26
            COMMAND_EXPAND_LISTS
        )
    else()
            add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${source} $<TARGET_FILE_DIR:${target}>/${dirname}
            COMMAND_EXPAND_LISTS
        )
    endif()
endfunction()


#-------------------------------------------------------------------------------------#
#            targets
#-------------------------------------------------------------------------------------#
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/glfw EXCLUDE_FROM_ALL)

if (WIN32)
    list(APPEND essential_libs opengl32)
elseif(UNIX AND NOT APPLE)
    list(APPEND essential_libs GL X11 dl pthread)
elseif(APPLE)
    # todo
endif()

add_subdirectory(3rdparty-src/imgui)
add_subdirectory(3rdparty-src/stb)
add_subdirectory(3rdparty-src/ImFileDialog)

add_subdirectory(tchInterface)      # top-level interface class definition, like Graphics Data Cache/Graphics Engine interfaces
add_subdirectory(tchGeometry)       # basic geometry primitives and algorithms
add_subdirectory(tchDatabase)       # databse and entities
add_subdirectory(tchPlatform)       # necessary platform specific encapsulation
add_subdirectory(tchCadToy)         # App/cross-platform execuatable

set_target_properties(tchCadToy PROPERTIES UNITY_BUILD OFF) # UNITY building
