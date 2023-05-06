#pragma once
#include <string>
#include <filesystem>
#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <utility>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <CommandLineWindow.h>
#include <MainMenuBar.h>
#include <Command.h>
#include <Canvas.h>
#include <OptionsModal.h>
#include <DocManager.h>
#include <FileTabBar.h>
#include <CloseModal.h>

// =========================================================================================================
// ----------------------------------- global variables: windows/path
// =========================================================================================================

// cwd: current working directory
inline std::filesystem::path g_PathCwd;

// system endian, true for big endian (like arm), false for little endian (like x86)
inline bool g_BigEndian {true};

// imgui window config file path
inline std::filesystem::path g_ImguiConfigFile;
inline std::string g_ImguiConfigFileStr;
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
inline const char* g_CommandLineWindowTitle = "Command-Line-Window";
inline const char* g_StatusBarTitle = "Status-Bar";
inline const char* g_PropertiesSideBarTitle = "Properties";
inline const char* g_FileTabBarTitle = "File-Tab-Bar";

// window width and height
inline int g_WindowWidth;
inline int g_WindowHeight;

// window layout properties: command line/status bar/properties side bar: in imgui screen coordinate (top-left as origin)
inline ImVec2 g_CommandLineWindowPos;
inline ImVec2 g_CommandLineWindowSize;
inline ImVec2 g_StatusBarPos;
inline ImVec2 g_StatusBarSize;
inline ImVec2 g_PropertiesSideBarPos;
inline ImVec2 g_PropertiesSideBarSize;
inline float g_CommandLineWindowHeight = 250;
inline float g_StatusBarHeight = 35;
inline float g_PropertiesSideBarWidth = 430;
inline float g_MainMenuBarHeight = 26;
inline float g_FileTabBarHeight = 26;

// properties window open or close
inline bool g_bPropertiesSideBarOpen = true;

// global window objects
// command line window
inline CommandLineWindow g_CmdWindow;
// menu bar
inline MainMenuBar g_MainMenuBar;
// file tabs
inline FileTabBar g_FileTabBar;


// =========================================================================================================
// ----------------------------------- global variables: modals/dialogs
// =========================================================================================================
// modals management, save the bool pointer of all variables that indicate whether a modal shows or not, name is dialog name.
inline std::map<std::string, bool*> g_ModalsMap;

// options modal
inline bool g_OptionsModalOpen {false};
inline OptionsModal g_OptionsModal;

// close modal
inline bool g_CloseModalOpen {false};
inline CloseModal g_CloseModal;


// =========================================================================================================
// ------------------------------------- global variables: commands related
// =========================================================================================================

// the global map that saves all commands
std::map<std::string, std::pair<Command*, int>>& getCommandsMap();


// =========================================================================================================
// ------------------------------------- global variables: canvas related
// =========================================================================================================

// canvas background color, same for all UI styles
inline glm::vec4 g_CanvasBackgroundColor {0.129f, 0.156f, 0.188f, 1.0f}; // RGB: 33,40,48

// OpenGL screen coordinate of canvas (left bottom as origin), changes along with the UI resizing (command line window/properties window)
inline int g_CanvasLeftBottomX;
inline int g_CanvasLeftBottomY;
inline int g_CanvasWidth;
inline int g_CanvasHeight;

// the global canvas
inline Canvas g_Canvas;

// cursor color
inline glm::vec4 g_CursorColor {1.0f, 1.0f, 1.0f, 1.0f}; // pure white

// grid color
inline glm::vec4 g_MainGridColor {0.211f, 0.238f, 0.305f, 1.0f}; // RGB: 54,61,78
inline glm::vec4 g_SubGridColor {0.148f, 0.176f, 0.215f, 1.0f}; // RGB: 38,45,55

// axises color
inline glm::vec4 g_XAxisColor { 97.0f / 256.0f, 37.0f / 256.0f, 39.0f / 256.0f, 1.0f}; // RGB: 97,37,39
inline glm::vec4 g_YAxisColor { 34.0f / 256.0f, 89.0f / 256.0f, 41.0f / 256.0f, 1.0f}; // RGB: 34,89,41

// =========================================================================================================
// ------------------------------------- global variables: inputs related
// =========================================================================================================

// cursor position: in OpenGL screen coordinate (left botton as origin)
inline int g_LastCursorPosX;
inline int g_LastCursorPosY;
inline int g_CursorPosX;
inline int g_CursorPosY;

// left/right mouse button
inline bool g_bLeftMouseButtonHold {false};
inline bool g_bRightMouseButtonHold {false};
inline bool g_bMiddleMouesButtonHold {false};

// scroll input: in OpenGL screen coordinate
inline int g_ScrollXOffset;
inline int g_ScrollYOffset;

// combination keyboard shortcuts map:
// - these keyboard shortcuts requires some control modes (Ctrl/Shift/Alt/Super, one or more) and one any key.
// - eg. Ctrl+Shift+S is {true, true, false, false, ImGuiKey_S}
inline std::map<std::tuple<bool, bool, bool, bool, ImGuiKey>, std::string> g_CombinationKeyboardShortcutsMap;

// independent keyboard shortcuts map:
// - only one key, usually used on Insert/Delete/Home/..., should not conflict with combination keyboard shortcuts.
// - if conflict occurs, independent keys always precede combination keys.
// - eg. if Ctrl+Delete and Delete are all registered, then Ctrl+Delete will never be triggered, Ctrl+Delete/Delete/Shift+Delete/xxx+Delete will all trigger the Delete shortcut.
inline std::map<ImGuiKey, std::string> g_IndependentKeyboardShortcutsMap;

// last keyboard time, if two keyboard shortcuts separated by 200ms are the same, then repeat to execute this command and update this time point.
//      if not the same, update this time too, if same shortcuts within 200ms, keep this time point unchanged.
inline double g_LastKeyboardShortcutTimePoint {0.0};

// last keybaord shortcut: 
inline std::tuple<bool, bool, bool, bool, int> g_LastKeyBoardShortcut {false, false, false, false, ImGuiKey_None};

// =========================================================================================================
// ------------------------------------- global variables: document related
// =========================================================================================================
// recent opened files
inline std::vector<std::string> g_RecentFiles;

// document manager
inline DocManager g_DocManager;

// is document changed last frame, for something need to be updated when document changes (eg. grid/axises/cursor)
inline bool g_bIsDocumentChanged {false};


// =========================================================================================================
// ------------------------------------- global functions
// =========================================================================================================

// check which OS current is
void checkOS();

// check the endian of the system
void checkSystemEndian();

// build current working directory from exe path
void buildCwd(const char* exePath);

// create important resource paths
void checkAndCreateImportantDirs();
