#include <cstdlib>
#include <fstream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <soil2/SOIL2.h>

#include "SysConfig.h"
#include "gl/GLFuncs.h"
#include "sys/Global.h"
#include "debug/Logger.h"

using namespace std::string_literals;

void openglInit()
{
    // init glfw
    int res = glfwInit();
    if (res != GLFW_TRUE)
    {
        globalLogger().fatal("GLFW init failed!");
        glfwTerminate();
        std::exit(-1);
    }
    // glfw window hint
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // prevent resizing window
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    g_pWindow = glfwCreateWindow(g_WindowSize.x, g_WindowSize.y, "tchCadToy", nullptr, nullptr);
    if (!g_pWindow)
    {
        globalLogger().fatal("GLFW create window failed!");
        glfwTerminate();
        std::exit(-1);
    }
    glfwMakeContextCurrent(g_pWindow);

    // init glad
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0)
    {
        globalLogger().fatal("GLAD init failed!");
        glfwTerminate();
        std::exit(-1);
    }
    globalLogger().info(std::format("OpenGL Version: {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version)));

    // some glfw settting
    glfwSetInputMode(g_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSwapInterval(1); // waiting frames before swap buffer: 1
}


// error handling
// for GLSL compile error
void printShaderLog(GLuint shader, const std::source_location& loc)
{
    GLint len = 0;
    GLint chWritten = 0;
    char* log = nullptr;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if (len > 0)
    {
        log = new char[len];
        glGetShaderInfoLog(shader, len, &chWritten, log);
        globalLogger().info(std::format("Shader Info Log: \n{}", log), loc);
        delete[] log;
    }
}
// for GLSL link error
void printProgramLog(GLuint program, const std::source_location& loc)
{
    GLint len = 0;
    GLint chWritten = 0;
    char* log = nullptr;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
    if (len > 0)
    {
        log = new char[len];
        glGetProgramInfoLog(program, len, &chWritten, log);
        globalLogger().info(std::format("Shader Info Log: \n{}", log), loc);
        delete[] log;
    }
}

static std::string glErrorToString(GLenum glError)
{
    switch (glError)
    {
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM"s;
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE"s;
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION"s;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION"s;
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY"s;
    case GL_STACK_UNDERFLOW:
        return "GL_STACK_UNDERFLOW"s;
    case GL_STACK_OVERFLOW:
        return "GL_STACK_OVERFLOW"s;
    default:
        return std::to_string(glError);
    }
}

// check for general OPenGL error
bool checkOpenGLError(const std::source_location& loc)
{
    bool foundError = false;
    GLenum glErr = glGetError();
    while (glErr != GL_NO_ERROR)
    {
        globalLogger().warning(std::format("OpenGL error: {}", glErrorToString(glErr)), loc);
        foundError = true;
        glErr = glGetError();
    }
    return foundError;
}

// read shader source from file
std::string readShaderSource(const std::string& filePath, const std::source_location& loc)
{
    std::string content;
    std::ifstream fin(filePath);
    if (!fin.is_open())
    {
        globalLogger().warning(std::format("Shader file {} does not exist!", filePath), loc);
        return content;
    }
    std::string line;
    while (!fin.eof())
    {
        std::getline(fin, line);
        content += line + "\n";
    }
    fin.close();
    return content;
}

// create a program from vertex, fragment and geometry shader files
GLuint createShaderProgram(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const std::source_location& loc)
{
    std::string vertexShaderStr = readShaderSource(vertexShader, loc);
    std::string fragmentShaderStr = readShaderSource(fragmentShader, loc);
    std::string geometryShaderStr;
    if (!geometryShader.empty())
    {
        geometryShaderStr = readShaderSource(geometryShader, loc);
    }
    return createShaderProgramFromSource(vertexShaderStr, fragmentShaderStr, geometryShaderStr, loc);
}

// create a program from vertex, tessellation, fragment and geometry shader files
GLuint createShaderProgram(const std::string& vertexShader, const std::string& tessellationCtrlShader, const std::string& tessellationEvalShader,
    const std::string& fragmentShader, const std::string& geometryShader,
    const std::source_location& loc)
{
    std::string vertexShaderStr = readShaderSource(vertexShader, loc);
    std::string tessellationCtrlShaderStr = readShaderSource(tessellationCtrlShader, loc);
    std::string tessellationEvalShaderStr = readShaderSource(tessellationEvalShader, loc);
    std::string fragmentShaderStr = readShaderSource(fragmentShader, loc);
    std::string geometryShaderStr;
    if (!geometryShader.empty())
    {
        geometryShaderStr = readShaderSource(geometryShader, loc);
    }
    return createShaderProgramFromSource(vertexShaderStr, tessellationCtrlShaderStr, tessellationEvalShaderStr, fragmentShaderStr, geometryShaderStr, loc);
}

// create and compile a shader of a specific type
GLuint createAndCompileShader(const std::string& shaderSource, GLenum shaderType, const std::string& promptStr,
    const std::source_location& loc)
{
    GLuint shader = glCreateShader(shaderType);
    const char* shaderSourceStr = shaderSource.c_str();
    glShaderSource(shader, 1, &shaderSourceStr, NULL);
    // compile shader
    glCompileShader(shader);
    checkOpenGLError();
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE)
    {
        globalLogger().warning(std::format("{} shader compilation failed!", promptStr), loc);
        printShaderLog(shader, loc);
    }
    return shader;
}

// create shader program from vertex, fragment, geometry shader sources
GLuint createShaderProgramFromSource(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader,
    const std::source_location& loc)
{
    return createShaderProgramFromSource(vertexShader, "", "", fragmentShader, geometryShader, loc);
}

// create shader program from vertex, tessellation, fragment, geometry shader sources
GLuint createShaderProgramFromSource(const std::string& vertexShader, const std::string& tessellationCtrlShader, const std::string& tessellationEvalShader,
    const std::string& fragmentShader, const std::string& geometryShader,
    const std::source_location& loc)
{
    // vertex and fragment shader
    GLuint vShader = createAndCompileShader(vertexShader, GL_VERTEX_SHADER, "Vertex", loc);
    GLuint fShader = createAndCompileShader(fragmentShader, GL_FRAGMENT_SHADER, "Fragment", loc);

    // create program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vShader);
    glAttachShader(shaderProgram, fShader);

    // tessellation control shader
    if (!tessellationCtrlShader.empty())
    {
        GLuint tCShader = createAndCompileShader(tessellationCtrlShader, GL_TESS_CONTROL_SHADER, "Tessellation control", loc);
        glAttachShader(shaderProgram, tCShader);
    }
    // tessellation evaluation shader
    if (!tessellationEvalShader.empty())
    {
        GLuint tEShader = createAndCompileShader(tessellationEvalShader, GL_TESS_EVALUATION_SHADER, "Tessellation evaluation", loc);
        glAttachShader(shaderProgram, tEShader);
    }
    // geometry shader
    if (!geometryShader.empty())
    {
        GLuint gShader = createAndCompileShader(geometryShader, GL_GEOMETRY_SHADER, "Geometry", loc);
        glAttachShader(shaderProgram, gShader);
    }

    // link shader program
    glLinkProgram(shaderProgram);
    checkOpenGLError();
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE)
    {
        globalLogger().warning("Shader program linking failed!", loc);
        printProgramLog(shaderProgram, loc);
    }
    return shaderProgram;
}

// load texture to OpenGL texture object
GLuint loadTexture(const std::string& textureImagePath, const std::source_location& loc)
{
    GLuint textureId;
    textureId = SOIL_load_OGL_texture(textureImagePath.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
    if (textureId == 0)
    {
        globalLogger().warning(std::format("Could not find texture file {}!", textureImagePath), loc);
    }
    return textureId;
}

// load cube map texture to OpenGL texture object
GLuint loadCubeMap(const std::string& rightImage, const std::string& leftImage,
    const std::string& topImage, const std::string& bottomImage,
    const std::string& frontImage, const std::string& backImage,
    const std::source_location& loc)
{
    GLuint textureId;
    textureId = SOIL_load_OGL_cubemap(rightImage.c_str(), leftImage.c_str(),
        topImage.c_str(), bottomImage.c_str(),
        frontImage.c_str(), backImage.c_str(),
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);
    if (textureId == 0)
    {
        globalLogger().warning(std::format("Could not load cube map from {}/{}/{}/{}/{}/{}, please check it out!",
            rightImage, leftImage, topImage, bottomImage, frontImage, backImage), loc);
    }
    return textureId;
}
