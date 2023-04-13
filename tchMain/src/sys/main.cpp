#include <iostream>
#include <format>

#include <glm/glm.hpp>

#include <SysConfig.h>
#include <Global.h>
#include <GLFuncs.h>
#include <Logger.h>

int main(int argc, char const *argv[])
{
    //==========================================//
    //                 prepare
    //------------------------------------------//
    globalLogger().setLowestOutputLevel(Logger::Trace); // the most detailed informations

    //==========================================//
    //                 prepare
    //------------------------------------------//
    checkOS();
    buildCwd(argv[0]);
    checkAndCreateImportantDirs();

    //==========================================//
    //         OpenGL init: glfw/glad
    //------------------------------------------//
    openglInit();

    //==========================================//
    //         main render loop
    //------------------------------------------//
    while (!glfwWindowShouldClose(g_pWindow))
    {
        // main logic here

        // check events, swap buffers
        glfwPollEvents();
        glfwSwapBuffers(g_pWindow);
    }


    //==========================================//
    //         glfw terminate
    //------------------------------------------//
    glfwTerminate();
    globalLogger().info("Program terminated properly!");
    return 0;
}
