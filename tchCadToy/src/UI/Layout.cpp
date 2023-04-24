#include <Layout.h>
#include <Global.h>


// calculate window layout, mainly for command line/status bar/properties side bar, 
// call at the first frame and everying time after resizing
void calculateLayout()
{
    // UIs are in imgui screen coordinate (left-top as origin, float type)
    int width, height;
    glfwGetWindowSize(g_pWindow, &width, &height);
    g_WindowWidth = width;
    g_WindowHeight = height;

    float propertiesWidth = g_PropertiesSideBarWidth;
    if (!g_bPropertiesSideBarOpen)
    {
        propertiesWidth = 0.0f;
    }

    // command line window
    g_CommandLineWindowSize.x = width - propertiesWidth; // + 10.0f; // move right some pixels to avoid right side resizing
    g_CommandLineWindowSize.y = g_CommandLineWindowHeight;
    g_CommandLineWindowPos.x = 0.0f;
    g_CommandLineWindowPos.y = height - g_StatusBarHeight - g_CommandLineWindowHeight;

    // status bar
    g_StatusBarSize.x = float(width);
    g_StatusBarSize.y = g_StatusBarHeight;
    g_StatusBarPos.x = 0.0f;
    g_StatusBarPos.y = height - g_StatusBarHeight;

    // properties side bar
    g_PropertiesSideBarSize.x = propertiesWidth;
    g_PropertiesSideBarSize.y = height - g_StatusBarHeight;
    g_PropertiesSideBarPos.x = width - propertiesWidth; // + 10.0f; // move right some pixels to avoid right side resizing
    g_PropertiesSideBarPos.y = 0.0f;

    // canvas is in OpenGL screen coordinate (left bottom as origin, int type)
    g_CanvasLeftBottomX = 0;
    g_CanvasLeftBottomY = int(g_StatusBarHeight + g_CommandLineWindowHeight);
    g_CanvasWidth = int(g_CommandLineWindowSize.x);
    g_CanvasHeight = height - g_CanvasLeftBottomY;
}

// set window layout
void setWindowLayout()
{
    ImGui::SetWindowSize(g_CommandLineWindowTitle, g_CommandLineWindowSize);
    ImGui::SetWindowPos(g_CommandLineWindowTitle, g_CommandLineWindowPos);
    ImGui::SetWindowSize(g_StatusBarTitle, g_StatusBarSize);
    ImGui::SetWindowPos(g_StatusBarTitle, g_StatusBarPos);
    if (g_bPropertiesSideBarOpen)
    {
        ImGui::SetWindowSize(g_PropertiesSideBarTitle, g_PropertiesSideBarSize);
        ImGui::SetWindowPos(g_PropertiesSideBarTitle, g_PropertiesSideBarPos);
    }
}