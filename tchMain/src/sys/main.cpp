#include <iostream>
#include <format>

#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <SysConfig.h>
#include <Global.h>
#include <GLFuncs.h>
#include <Logger.h>

int main(int argc, char const *argv[])
{
    //==========================================//
    //                 logger
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
    //         imgui context setup
    //------------------------------------------//
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(g_pWindow, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Our state
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    //==========================================//
    //         main render loop
    //------------------------------------------//
    while (!glfwWindowShouldClose(g_pWindow))
    {
        // main logic here

        // check events, swap buffers
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(g_pWindow, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(g_pWindow);
    }


    //==========================================//
    //         glfw terminate
    //------------------------------------------//
    glfwTerminate();
    globalLogger().info("Program terminated properly!");
    return 0;
}
