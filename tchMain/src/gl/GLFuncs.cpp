#include <cstdlib>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <SysConfig.h>
#include <GLFuncs.h>
#include <Global.h>
#include <Logger.h>

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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // prevent resizing window
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    g_pWindow = glfwCreateWindow(g_WindowSz.x, g_WindowSz.y, "tchMain", nullptr, nullptr);
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