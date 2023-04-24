#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <imgui.h>

#include <Canvas.h>
#include <GLFuncs.h>
#include <Global.h>

// basic shader
static const std::string basicPureColorVertexShader = R"glsl(
#version 430
layout (location = 0) in vec3 position; // vertex array buffer
uniform mat4 mvMatrix;
uniform mat4 projMatrix;
uniform vec4 inputColor;
void main(void)
{
    gl_Position = projMatrix * mvMatrix * vec4(position, 1.0);
}
)glsl";

static const std::string basicPureColorFragmentShader = R"glsl(
#version 430
layout (location = 0) in vec3 position; // vertex array buffer
uniform mat4 mvMatrix;
uniform mat4 projMatrix;
uniform vec4 inputColor;
out vec4 color;
void main(void)
{
    color = inputColor;
}
)glsl";

Canvas::Canvas()
{
}

static GLuint g_vao = 0;
static GLuint g_vbo = 0;
static std::vector<glm::vec3> vertices;

// compile opengl shader, register callbacks
void Canvas::init()
{
    // compile shader
    m_BasicPureColorShader.setShaderSource(basicPureColorVertexShader, basicPureColorFragmentShader);

    // initialize the grid
    static bool bInitialized = false;
    if (!bInitialized)
    {
        bInitialized = true;
        // grid: from (0, 0) to (10, 10)
        for (int i = -10; i <= 10; i++)
        {
            vertices.emplace_back(-10.0f, i * 1.0f, 0.0f);
            vertices.emplace_back( 10.0f, i * 1.0f, 0.0f);
            vertices.emplace_back(i * 1.0f, -10.0f, 0.0f);
            vertices.emplace_back(i * 1.0f,  10.0f, 0.0f);
        }
        glGenVertexArrays(1, &g_vao);
        glGenBuffers(1, &g_vbo);
        glBindVertexArray(g_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        checkOpenGLError();
    }

    // set callbacks
    auto cursorPosCallback = [](GLFWwindow* pWindow, double xpos, double ypos) -> void {
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse)
        {
            // xpos and ypos are in screen coordinate which treats left-top as origin, convert to OpenGL screen(viewport) coordinate (left-bottom as origin)
            g_CursorPosX = int(xpos);
            g_CursorPosY = g_WindowHeight - int(ypos);
            g_bHideCursor = true;
        }
        else
        {
            g_bHideCursor = false;
        }
    };
    glfwSetCursorPosCallback(g_pWindow, cursorPosCallback);

    auto mouseButtonCallback = [](GLFWwindow* pWindow, int button, int action, int mods) -> void {
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse)
        {
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
            {
                g_bLeftMouseButtonHold = true;
            }
            else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
            {
                g_bLeftMouseButtonHold = false;
            }
            else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
            {
                g_bRightMouseButtonHold = true;
            }
            else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
            {
                g_bRightMouseButtonHold = false;
            }
            else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
            {
                g_bMiddleMouesButtonHold = true;
            }
            else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
            {
                g_bMiddleMouesButtonHold = false;
            }
        }
    };
    glfwSetMouseButtonCallback(g_pWindow, mouseButtonCallback);

    auto scrollCallback = [](GLFWwindow* pWindow, double xoffset, double yoffset) -> void {
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse)
        {
            g_ScrollXOffset = int(xoffset);
            g_ScrollYOffset = int(yoffset);
        }
    };
    glfwSetScrollCallback(g_pWindow, scrollCallback);

    // todo: keyboard callback
}

// update cursor hover point / grid / matrices, etc
void Canvas::update()
{
    // calculate delta Cursor X/Y for like selection: todo

    // update last cursor position
    g_LastCursorPosX = g_CursorPosX;
    g_LastCursorPosY = g_CursorPosY;

    // update canvas OpenGL 3D coordinates by update scale factor (chagned through mouse wheel)
    // and yoffset is treated just like xoffset(when scrool the mouse wheel and press Shift at the same time)
    if (g_ScrollXOffset == 0)
    {
        g_ScrollXOffset = g_ScrollYOffset;
    }
    g_ScrollYOffset = 0;
    if (g_ScrollXOffset != 0)
    {
        while (g_ScrollXOffset >= 1)
        {
            g_CanvasScaleFactor *= 0.8f;
            g_ScrollXOffset -= 1;
        }
        while (g_ScrollXOffset <= -1)
        {
            g_CanvasScaleFactor *= 1.25f;
            g_ScrollXOffset += 1;
        }
        g_ScrollXOffset = 0;
    }

    // calculate canvas OpenGL 3D coordinates
    g_CanvasTop = g_CanvasCenterY + g_CanvasScaleFactor * 0.5f * g_CanvasHeight;
    g_CanvasBottom = g_CanvasCenterY - g_CanvasScaleFactor * 0.5f * g_CanvasHeight;
    g_CanvasLeft = g_CanvasCenterX - g_CanvasScaleFactor * 0.5f * g_CanvasWidth;
    g_CanvasRight = g_CanvasCenterX + g_CanvasScaleFactor * 0.5f * g_CanvasWidth;

    // calculate hover point OpenGL 3D coordinates, update it only when it's inside canvas.
    if (g_CursorPosX > 0 && g_CursorPosX < g_CanvasLeftBottomX + g_CanvasWidth &&
        g_CursorPosY > 0 && g_CursorPosY < g_CanvasLeftBottomY + g_CanvasHeight)
    {
        g_CurrentHoverPoint.x = g_CanvasLeft + g_CanvasScaleFactor * (g_CursorPosX - g_CanvasLeftBottomX);
        g_CurrentHoverPoint.y = g_CanvasBottom + g_CanvasScaleFactor * (g_CursorPosY - g_CanvasLeftBottomY);
        g_CurrentHoverPoint.z = 0.0f;
    }

    // hide cursor and draw cursor by myself if cursor is inside the canvas (and it's not captures by any floating windows).
    if (g_bHideCursor)
    {
        //glfwSetInputMode(g_pWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    // calculate matrices
    m_ModelMatrix = glm::mat4(1.0f);
    m_ViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_ProjMatrix = glm::ortho(g_CanvasLeft, g_CanvasRight, g_CanvasBottom, g_CanvasTop, 0.1f, 1000.0f);
    //m_ProjMatrix = glm::perspective(glm::pi<float>() / 2.0f, g_CanvasWidth * 1.0f / g_CanvasHeight, 0.1f, 1000.0f);
}

// draw entities: cursor, grid, 
void Canvas::draw()
{
    
    m_BasicPureColorShader.use();
    m_BasicPureColorShader.setMat4("mvMatrix", m_ModelMatrix * m_ViewMatrix);
    m_BasicPureColorShader.setMat4("projMatrix", m_ProjMatrix);
    m_BasicPureColorShader.setVec4("inputColor", 1.0f, 1.0f, 1.0f, 1.0f);
    glBindVertexArray(g_vao);
    glDrawArrays(GL_LINES, 0, vertices.size() * 3);
    glBindVertexArray(0);
}
