#pragma once

#include <string>
#include <source_location>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

// init glad/glfw, create glfw window
void openglInit();

// error handling
// for GLSL compile error
void printShaderLog(GLuint shader, const std::source_location& loc = std::source_location::current());
// for GLSL link error
void printProgramLog(GLuint program, const std::source_location& loc = std::source_location::current());
// check for general OPenGL error
bool checkOpenGLError(const std::source_location& loc = std::source_location::current());

// read shader source from file
std::string readShaderSource(const std::string& filePath, const std::source_location& loc = std::source_location::current());

// create a program from vertex, fragment and geometry shader files
GLuint createShaderProgram(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader = "",
    const std::source_location& loc = std::source_location::current());
// create a program from vertex, tessellation, fragment and geometry shader files
GLuint createShaderProgram(const std::string& vertexShader, const std::string& tessellationCtrlShader, const std::string& tessellationEvalShader,
    const std::string& fragmentShader, const std::string& geometryShader = "",
    const std::source_location& loc = std::source_location::current());

// create and compile a shader of a specific type
GLuint createAndCompileShader(const std::string& shaderSource, GLenum shaderType, const std::string& promptStr,
    const std::source_location& loc = std::source_location::current());
// create shader program from vertex, fragment, geometry shader sources
GLuint createShaderProgramFromSource(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader = "",
    const std::source_location& loc = std::source_location::current());
// create shader program from vertex, tessellation, fragment, geometry shader sources
GLuint createShaderProgramFromSource(const std::string& vertexShader, const std::string& tessellationCtrlShader, const std::string& tessellationEvalShader,
    const std::string& fragmentShader, const std::string& geometryShader = "",
    const std::source_location& loc = std::source_location::current());

// load texture to OpenGL texture object
GLuint loadTexture(const std::string& textureImagePath, const std::source_location& loc = std::source_location::current());

// load cube map texture to OpenGL texture object
GLuint loadCubeMap(const std::string& rightImage, const std::string& leftImage,
    const std::string& topImage, const std::string& bottomImage,
    const std::string& frontImage, const std::string& backImage,
    const std::source_location& loc = std::source_location::current());

