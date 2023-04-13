cmake_minimum_required(VERSION 3.15)

project(3rdpartyProj)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/../3rdparty-install)

# add glfw target to CMakeLists.txt, not here.
add_subdirectory(glad)
add_subdirectory(SOIL2)

# headers only libraries: install manually
# install glm headers manually
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glm/glm DESTINATION include)
# install rapidjson headers manually
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include/rapidjson DESTINATION include)
# install magic_enum headers manually
file(GLOB magic_enum_headers ${CMAKE_CURRENT_SOURCE_DIR}/magic_enum/include/*.hpp)
install(FILES ${magic_enum_headers} DESTINATION include/magic_enum)

# install GLFW headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include/GLFW DESTINATION include)
# install format_bridge
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/format_bridge DESTINATION include)
# install fmt headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt DESTINATION include)