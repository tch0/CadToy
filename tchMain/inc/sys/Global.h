#pragma once
#include <string>
#include <filesystem>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// global variables

// cwd: current working directory
inline std::filesystem::path g_pathCwd;

// viewing box size
inline glm::ivec2 g_WindowSz {1920, 1080};
inline glm::ivec2 g_GameSz {1920, 1080};
inline bool g_bFullScreen = false;

// the global GLFW window
inline GLFWwindow* g_pWindow = nullptr;


// check which OS current is
void checkOS();

// build current working directory from exe path
void buildCwd(const char* exePath);

// create important resource paths
void checkAndCreateImportantDirs();
