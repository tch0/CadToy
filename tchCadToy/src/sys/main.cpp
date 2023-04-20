#include <iostream>
#include <format>
#include <filesystem>
#include <string>

#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <SysConfig.h>
#include <Global.h>
#include <GLFuncs.h>
#include <Logger.h>
#include <Layout.h>
#include <CommandLineWindow.h>

// for test temporarily
glm::vec3 currentHoverPoint()
{
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

int main(int argc, char const *argv[])
{
    //====================================================================================//
    //                 logger
    //------------------------------------------------------------------------------------//
    globalLogger().setLowestOutputLevel(Logger::Trace); // the most detailed informations

    //====================================================================================//
    //                 prepare
    //------------------------------------------------------------------------------------//
    checkOS();
    buildCwd(argv[0]);
    checkAndCreateImportantDirs();

    //====================================================================================//
    //         OpenGL init: glfw/glad
    //------------------------------------------------------------------------------------//
    openglInit();

    //====================================================================================//
    //         imgui context setup
    //------------------------------------------------------------------------------------//
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    // ini file
    g_WindowConfigFile = g_PathCwd / "imgui.ini";
    g_WindowConfigFileStr = g_WindowConfigFile.string();
    io.IniFilename = g_WindowConfigFileStr.c_str();
    globalLogger().info(std::format("Window config file: {}", g_WindowConfigFileStr));

    // default log file
    g_ImguiLogFile = g_PathCwd / "imgui_log.txt";
    g_ImguiLogFileStr = g_ImguiLogFile.string();
    io.LogFilename = g_ImguiLogFileStr.c_str();
    globalLogger().info(std::format("Imgui log file: {}", g_ImguiLogFileStr));

    // load font
    g_ConsolasFontPath = g_PathCwd / "fonts/consolas.ttf";
    g_ConsolasFontPathStr = g_ConsolasFontPath.string();
    if (std::filesystem::exists(g_ConsolasFontPath))
    {
        // load consolas and set to default
        g_ConsolasFontSize = 16.0f;
        ImFont* consolasFont = io.Fonts->AddFontFromFileTTF(g_ConsolasFontPathStr.c_str(), g_ConsolasFontSize);
        io.FontDefault = consolasFont;
        consolasFont->Scale = 1.0;
    }
    else
    {
        globalLogger().warning(std::format("Font file {} load failed, use default font!", g_ConsolasFontPathStr));
    }

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(g_pWindow, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    //====================================================================================//
    //         main render loop
    //------------------------------------------------------------------------------------//
    while (!glfwWindowShouldClose(g_pWindow))
    {
        // main logic here
        
        // check events, swap buffers
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // status bar
        {
            glm::vec3 currentP = currentHoverPoint();
            ImGui::Begin(g_StatusBarTitle, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Text("%.4f, %.4f, %.4f", currentP.x, currentP.y, currentP.z);
            // todo: other icons
            ImGui::End();
        }
        // properties side bar
        {
            if (g_bPropertiesSideBarOpen)
            {
                ImGui::Begin(g_PropertiesSideBarTitle, &g_bPropertiesSideBarOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
                ImGui::Text("properties test");
                g_PropertiesSideBarWidth = ImGui::GetWindowSize().x;
                ImGui::End();
            }
        }
        // command line
        {
            g_CmdWindow.draw(g_CommandLineWindowTitle, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove, g_CommandLineWindowHeight);
        }

        // calculate layout and set to windows for Command line window/status bar/properties side bar
        {
            calculateLayout();
            setWindowLayout();
        }

        // show demo window as an example
        ImGui::ShowDemoWindow();

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

    // Imgui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    //====================================================================================//
    //         glfw terminate
    //------------------------------------------------------------------------------------//
    glfwTerminate();
    globalLogger().info("Program terminated properly!");
    return 0;
}
