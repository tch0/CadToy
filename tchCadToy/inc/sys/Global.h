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
#include <Canvas.h>


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

// window width and height
inline int g_WindowWidth;
inline int g_WindowHeight;

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
// ------------------------------------- global variables: commands related
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

// OpenGL screen coordinate of canvas (left bottom as origin), changes along with the UI resizing (command line window/properties window)
inline int g_CanvasLeftBottomX;
inline int g_CanvasLeftBottomY;
inline int g_CanvasWidth;
inline int g_CanvasHeight;

// the global canvas
inline Canvas g_Canvas;

// current cursor hover point in canvas, calculated from current screen cursor position
inline glm::vec3 g_CurrentHoverPoint;

// draw cursor by myself and hide glfw cursor at the same time (when hover drawing area) or show glfw cursor (hover in some window)
inline bool g_bHideCursor {false};

// the canvas center point, in OpenGL 3D coordinate, but only X-Y coordinate, z is fixed (always 0) for now (aka the elevation).
inline float g_CanvasCenterX {0.0f};
inline float g_CanvasCenterY {0.0f};

// the canvas scale factor, changed through mouse wheel, determines canvas OpenGL 3d Coordinates together with the canvas center point and screen width/height of canvas.
// specifically, this factor is equal to the length of one pixel in 3D OpenGL coordinate.
inline float g_CanvasScaleFactor {1.0f};

// OpenGL 3D coordinate of canvas, calculated by canvas center point & canvas scale factor & canvas screen coordinates
inline float g_CanvasLeft   {0.0f};
inline float g_CanvasRight  {0.0f};
inline float g_CanvasTop    {0.0f};
inline float g_CanvasBottom {0.0f};


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

// =========================================================================================================
// ------------------------------------- global functions
// =========================================================================================================

// check which OS current is
void checkOS();

// build current working directory from exe path
void buildCwd(const char* exePath);

// create important resource paths
void checkAndCreateImportantDirs();