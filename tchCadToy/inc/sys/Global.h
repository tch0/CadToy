#pragma once
#include <string>
#include <filesystem>
#include <string>
#include <map>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <CommandLineWindow.h>
#include <Command.h>

// =========================================================================================================
// ----------------------------------- global variables: windows/path
// =========================================================================================================

// cwd: current working directory
inline std::filesystem::path g_PathCwd;

// imgui window config file path
inline std::filesystem::path g_WindowConfigFile;
inline std::string g_WindowConfigFileStr;
// imgui log file path
inline std::filesystem::path g_ImguiLogFile;
inline std::string g_ImguiLogFileStr;

// consolas font path, use consolas for now
inline std::filesystem::path g_ConsolasFontPath;
inline std::string g_ConsolasFontPathStr;
inline float g_ConsolasFontSize;

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
// ------------------------------------- global variables: commands
// =========================================================================================================

// the global map that saves all commands
std::map<std::string, std::pair<Command*, int>>& getCommandsMap();

// the unprocessed input string that will be either pass to command (if the command is executing now) as inputs 
//      or be parsed as next command (if pervious command do not need inputs, when pervious command finish).
inline std::string g_UnprocessedInput;

// if we are current in the execution of a command
// specifically:
//  - a command is excuting and waiting for text/point/... input.
//  - the command pops up a modal dialog, and is waiting to process/close.
//  - ... (other kind, to be added here)
// How to maintain this value?
inline bool g_bInCommandExecution {false};


// =========================================================================================================
// ------------------------------------- global variables: canvas related
// =========================================================================================================

// canvas background color, same for all UI styles
inline glm::vec4 g_CanvasBackgroundColor {0.129f, 0.156f, 0.188f, 1.0f}; // RGB: 33,40,48

// OpenGL screen coordinate of canvas (left bottom as origin), changes as the UI (command line window/properties window)
inline int g_CanvasLeftBottomX;
inline int g_CanvasLeftBottomY;
inline int g_CanvasWidth;
inline int g_CanvasHeight;


// =========================================================================================================
// ------------------------------------- global functions
// =========================================================================================================

// check which OS current is
void checkOS();

// build current working directory from exe path
void buildCwd(const char* exePath);

// create important resource paths
void checkAndCreateImportantDirs();
