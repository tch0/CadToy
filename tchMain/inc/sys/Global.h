#pragma once
#include <string>
#include <filesystem>
#include <string>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <CommandLineWindow.h>

// =========================================================================================================
// ------------------------------------- global variables
// =========================================================================================================

// cwd: current working directory
inline std::filesystem::path g_PathCwd;

// imgui window config file path
inline std::filesystem::path g_WindowConfigFile;
inline std::string g_WindowConfigFileStr;
// imgui log file path
inline std::filesystem::path g_ImguiLogFile;
inline std::string g_ImguiLogFileStr;

// window size
inline glm::ivec2 g_WindowSize {1920, 1080};
inline bool g_bFullScreen = false;

// the global GLFW window
inline GLFWwindow* g_pWindow = nullptr;

// window ID of command line/status bar/properties side bar
inline const char* g_CommandLineWindowTitle = "Command Line";
inline const char* g_StatusBarTitle = "Status Bar";
inline const char* g_PropertiesSideBarTitle = "Properties";

// window layout properties: command line/status bar/properties side bar
inline ImVec2 g_CommandLineWindowPos;
inline ImVec2 g_CommandLineWindowSize;
inline ImVec2 g_StatusBarPos;
inline ImVec2 g_StatusBarSize;
inline ImVec2 g_PropertiesSideBarPos;
inline ImVec2 g_PropertiesSideBarSize;
inline float g_CommandLineWindowHeight = 250;
inline float g_StatusBarHeight = 35;
inline float g_PropertiesSideBarWidth = 430;

// properties window open or close
inline bool g_bPropertiesSideBarOpen = true;

// global window objects
// command line window
inline CommandLineWindow g_CmdWindow;

// =========================================================================================================
// ------------------------------------- global functions
// =========================================================================================================

// check which OS current is
void checkOS();

// build current working directory from exe path
void buildCwd(const char* exePath);

// create important resource paths
void checkAndCreateImportantDirs();
