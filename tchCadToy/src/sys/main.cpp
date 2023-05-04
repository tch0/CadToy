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
#include <Canvas.h>
#include <Keyboard.h>


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

    // canvas init: compile shader
    g_Canvas.init();

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

    // register keyboard shortcuts
    registerAllKeyboardShortcuts();

    //====================================================================================//
    //         main render loop
    //------------------------------------------------------------------------------------//
    while (!glfwWindowShouldClose(g_pWindow))
    {
        // check events, swap buffers
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // detect keyboard shortcuts
        detectKeyboardShortcutAndExecute();

        // menu bar
        {
            g_MainMenuBar.draw();
        }
        // status bar
        {
            glm::vec3 hoverPoint = g_CurrentHoverPoint;
            ImGui::Begin(g_StatusBarTitle, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Text("%.4f, %.4f, %.4f", hoverPoint.x, hoverPoint.y, hoverPoint.z);
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

        // calculate layout and set to windows, for Command line window/status bar/properties side bar/main menu bar
        {
            calculateLayout();
            setWindowLayout();
        }

        // show demo window as an example
        ImGui::ShowDemoWindow();

        // other modals/dialogs
        {
            g_OptionsModal.draw();
            maintainCommandExecutionState();
        }

        // first draw canvas
        {
            g_Canvas.update();
            g_Canvas.draw();
        }

        // then draw UI
        {
            ImGui::Render();
            int displayWidth, displayHeight;
            glfwGetFramebufferSize(g_pWindow, &displayWidth, &displayHeight);
            glViewport(0, 0, displayWidth, displayHeight);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

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
