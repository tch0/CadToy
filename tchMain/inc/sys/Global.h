#pragma once
#include <string>
#include <filesystem>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// global variables

// cwd: current working directory
inline std::filesystem::path g_pathCwd;

// system endian, true for big endian (like arm), false for little endian (like x86)
inline bool g_bBigEndian {true};

// viewing box size
inline glm::ivec2 g_WindowSize {1920, 1080};
inline bool g_bFullScreen = false;

// the global GLFW window
inline GLFWwindow* g_pWindow = nullptr;


// check which OS current is
void checkOS();

// check the endian of the system
void checkSystemEndian();

// build current working directory from exe path
void buildCwd(const char* exePath);

// create important resource paths
void checkAndCreateImportantDirs();
