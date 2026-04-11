file(GLOB stb_headers ${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h)

add_library(stb STATIC
    ${stb_headers}
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_impl.cpp
)

target_include_directories(stb
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# 需要时考虑这个宏的行为：STBI_WINDOWS_UTF8
# if(WIN32)
#     target_compile_definitions(stb PUBLIC STBI_WINDOWS_UTF8)
# endif()