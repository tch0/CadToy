cmake_minimum_required(VERSION 3.15)

project(3rdpartyProj)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # generate position independent code

# install directories
set(3RDPARTY_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty-install)
set(3RDPARTY_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty-src)

# default install prefix for compiled libraries: gald/SOIL2
set(CMAKE_INSTALL_PREFIX ${3RDPARTY_INSTALL_DIR})

# add glfw target to CMakeLists.txt, not here.
add_subdirectory(glad)
add_subdirectory(SOIL2)

# headers only libraries: install to 3rdparty-install
# install glm headers manually
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glm/glm DESTINATION ${3RDPARTY_INSTALL_DIR}/include)
# install rapidjson headers manually
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include/rapidjson DESTINATION ${3RDPARTY_INSTALL_DIR}/include)
# install magic_enum headers manually
file(GLOB magic_enum_headers ${CMAKE_CURRENT_SOURCE_DIR}/magic_enum/include/magic_enum/*.hpp)
install(FILES ${magic_enum_headers} DESTINATION ${3RDPARTY_INSTALL_DIR}/include/magic_enum)
# install minicoro header
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/minicoro/minicoro.h DESTINATION ${3RDPARTY_INSTALL_DIR}/include/minicoro)

# install GLFW headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include/GLFW DESTINATION ${3RDPARTY_INSTALL_DIR}/include)

# install imgui sources to 3rdparty-src/imgui/
file(GLOB imgui_sources ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)
file(GLOB imgui_headers ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.h)
file(GLOB imgui_backends_sources ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/*.cpp)
file(GLOB imgui_backends_headers ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/*.h)
file(GLOB imgui_misc_cpp_sources ${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/cpp/*.cpp)
file(GLOB imgui_misc_cpp_headers ${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/cpp/*.h)
install(FILES ${imgui_sources} DESTINATION ${3RDPARTY_SRC_DIR}/imgui)
install(FILES ${imgui_headers} DESTINATION ${3RDPARTY_SRC_DIR}/imgui)
install(FILES ${imgui_backends_sources} DESTINATION ${3RDPARTY_SRC_DIR}/imgui/backends)
install(FILES ${imgui_backends_headers} DESTINATION ${3RDPARTY_SRC_DIR}/imgui/backends)
install(FILES ${imgui_misc_cpp_sources} DESTINATION ${3RDPARTY_SRC_DIR}/imgui/misc/cpp)
install(FILES ${imgui_misc_cpp_headers} DESTINATION ${3RDPARTY_SRC_DIR}/imgui/misc/cpp)

# install ImFileDialog header and source file to 3rdparty-src/ImFileDialog/
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ImFileDialog/ImFileDialog.h DESTINATION ${3RDPARTY_SRC_DIR}/ImFileDialog/inc)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ImFileDialog/ImFileDialog.cpp DESTINATION ${3RDPARTY_SRC_DIR}/ImFileDialog/src)

# install stb headers to 3rdparty-src/stb/
# only install stb_image.h for now, add other if need in future
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/stb/stb_image.h DESTINATION ${3RDPARTY_SRC_DIR}/stb/inc)
