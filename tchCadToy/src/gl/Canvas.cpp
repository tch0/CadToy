#include <vector>
#include <cmath>

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

// =========================================================================================================
// ------------------------------------- CustomCursor
// =========================================================================================================
Canvas::CustomCursor::CustomCursor()
{
    m_Vertices.reserve(16);
}

// range: 0 ~ 50
void Canvas::CustomCursor::setPickBoxSize(int size)
{
    if (size <= 0)
    {
        size = 0;
    }
    else if (size > 50)
    {
        size = 50;
    }
    m_PickBoxSize = size;
    updateVertices();
}

// range: 10 ~ 200
void Canvas::CustomCursor::setCursorSize(int size)
{
    if (size <= 10)
    {
        size = 10;
    }
    else if (size > 200)
    {
        size = 200;
    }
    m_CursorSize = size;
    updateVertices();
}

// update cursor datas
void Canvas::CustomCursor::updateVertices()
{
    m_Vertices.resize(0);
    float pickBoxHalfWidth = m_PickBoxSize * g_CanvasScaleFactor * 0.5f;
    float cursorHalfWidth = m_CursorSize * g_CanvasScaleFactor * 0.5f;
    glm::vec3 left = g_CurrentHoverPoint - glm::vec3(cursorHalfWidth, 0.0f, 0.0f);
    glm::vec3 right = g_CurrentHoverPoint + glm::vec3(cursorHalfWidth, 0.0f, 0.0f);
    glm::vec3 top = g_CurrentHoverPoint + glm::vec3(0.0f, cursorHalfWidth, 0.0f);
    glm::vec3 bottom = g_CurrentHoverPoint + glm::vec3(0.0f, -cursorHalfWidth, 0.0f);
    if (m_PickBoxSize == 0)
    {
        m_Vertices.push_back(left);
        m_Vertices.push_back(right);
        m_Vertices.push_back(top);
        m_Vertices.push_back(bottom);
    }
    else
    {
        glm::vec3 pickBoxLeftTop = g_CurrentHoverPoint + glm::vec3(-pickBoxHalfWidth, pickBoxHalfWidth, 0.0f);
        glm::vec3 pickBoxLeftBottom = g_CurrentHoverPoint + glm::vec3(-pickBoxHalfWidth, -pickBoxHalfWidth, 0.0f);
        glm::vec3 pickBoxRightTop = g_CurrentHoverPoint + glm::vec3(pickBoxHalfWidth, pickBoxHalfWidth, 0.0f);
        glm::vec3 pickBoxRightBottom = g_CurrentHoverPoint + glm::vec3(pickBoxHalfWidth, -pickBoxHalfWidth, 0.0f);
        glm::vec3 leftMiddle = g_CurrentHoverPoint + glm::vec3(-pickBoxHalfWidth, 0.0f, 0.0f);
        glm::vec3 rightMiddle = g_CurrentHoverPoint + glm::vec3(pickBoxHalfWidth, 0.0f, 0.0f);
        glm::vec3 topMiddle = g_CurrentHoverPoint + glm::vec3(0.0f, pickBoxHalfWidth, 0.0f);
        glm::vec3 bottomMiddle = g_CurrentHoverPoint + glm::vec3(0.0f, -pickBoxHalfWidth, 0.0f);
        if (m_PickBoxSize < m_CursorSize)
        {
            m_Vertices.push_back(left);
            m_Vertices.push_back(leftMiddle);
            m_Vertices.push_back(right);
            m_Vertices.push_back(rightMiddle);
            m_Vertices.push_back(top);
            m_Vertices.push_back(topMiddle);
            m_Vertices.push_back(bottom);
            m_Vertices.push_back(bottomMiddle);
        }
        m_Vertices.push_back(pickBoxLeftTop);
        m_Vertices.push_back(pickBoxRightTop);
        m_Vertices.push_back(pickBoxRightTop);
        m_Vertices.push_back(pickBoxRightBottom);
        m_Vertices.push_back(pickBoxRightBottom);
        m_Vertices.push_back(pickBoxLeftBottom);
        m_Vertices.push_back(pickBoxLeftBottom);
        m_Vertices.push_back(pickBoxLeftTop);
    }
}

const std::vector<glm::vec3>& Canvas::CustomCursor::vertices()
{
    return m_Vertices;
}


// =========================================================================================================
// ------------------------------------- Grid
// =========================================================================================================
Canvas::Grid::Grid()
{
    m_MainGridVertices.reserve(100);
    m_SubGridVertices.reserve(400);
}

void Canvas::Grid::updateVertices()
{
    m_MainGridVertices.resize(0);
    m_SubGridVertices.resize(0);
    float gridLeft = std::floor(g_CanvasLeft / g_GridScaleFactor) * g_GridScaleFactor;
    float gridRight = std::ceil(g_CanvasRight / g_GridScaleFactor) * g_GridScaleFactor;
    float gridBottom = std::floor(g_CanvasBottom / g_GridScaleFactor) * g_GridScaleFactor;
    float gridTop = std::ceil(g_CanvasTop / g_GridScaleFactor) * g_GridScaleFactor;
    for (float x = gridLeft; x <= gridRight; x += g_GridScaleFactor)
    {
        m_MainGridVertices.emplace_back(x, gridBottom, 0.0f);
        m_MainGridVertices.emplace_back(x, gridTop, 0.0f);
        for (int i = 1; i <= 4; i++)
        {
            m_SubGridVertices.emplace_back(x + i / 5.0f * g_GridScaleFactor, gridBottom, 0.0f);
            m_SubGridVertices.emplace_back(x + i / 5.0f * g_GridScaleFactor, gridTop, 0.0f);
        }
    }
    for (float y = gridBottom; y <= gridTop; y += g_GridScaleFactor)
    {
        m_MainGridVertices.emplace_back(gridLeft, y, 0.0f);
        m_MainGridVertices.emplace_back(gridRight, y, 0.0f);
        for (int i = 1; i <= 4; i++)
        {
            m_SubGridVertices.emplace_back(gridLeft, y + i / 5.0f * g_GridScaleFactor, 0.0f);
            m_SubGridVertices.emplace_back(gridRight, y + i / 5.0f * g_GridScaleFactor, 0.0f);
        }
    }
}

const std::vector<glm::vec3>& Canvas::Grid::mainGridVertices()
{
    return m_MainGridVertices;
}

const std::vector<glm::vec3>& Canvas::Grid::subGridVertices()
{
    return m_SubGridVertices;
}


// =========================================================================================================
// ------------------------------------- Axises
// =========================================================================================================
Canvas::Axises::Axises()
{
    m_XAxisVertices.reserve(2);
    m_YAxisVertices.reserve(2);
}

void Canvas::Axises::updateVertices()
{
    float width = g_CanvasRight - g_CanvasLeft;
    float height = g_CanvasTop - g_CanvasBottom;
    // update only when necessary
    if (width > m_Width || height > m_Height)
    {
        m_Width = width;
        m_Height = height;
        m_XAxisVertices.resize(0);
        m_YAxisVertices.resize(0);
        m_XAxisVertices.emplace_back(0.0f, 0.0f, 0.0f);
        m_XAxisVertices.emplace_back(m_Width, 0.0f, 0.0f);
        m_YAxisVertices.emplace_back(0.0f, 0.0f, 0.0f);
        m_YAxisVertices.emplace_back(0.0f, m_Height, 0.0f);
    }
}

const std::vector<glm::vec3>& Canvas::Axises::xVertices()
{
    return m_XAxisVertices;
}

const std::vector<glm::vec3>& Canvas::Axises::yVertices()
{
    return m_YAxisVertices;
}


// =========================================================================================================
// ------------------------------------- Canvas
// =========================================================================================================
Canvas::Canvas()
{
}

// compile opengl shader, register callbacks
void Canvas::init()
{
    // compile shader
    m_BasicPureColorShader.setShaderSource(basicPureColorVertexShader, basicPureColorFragmentShader);

    // init cursor vao, vbo
    //m_Cursor.setPickBoxSize(0);
    generateVaoVbo(m_CursorVao, m_CursorVbo, m_Cursor.vertices());

    // grid vao, vbo
    m_Grid.updateVertices();
    generateVaoVbo(m_MainGridVao, m_MainGridVbo, m_Grid.mainGridVertices());
    generateVaoVbo(m_SubGridVao, m_SubGridVbo, m_Grid.subGridVertices());

    // axises vao, vbo
    m_Axises.updateVertices();
    generateVaoVbo(m_XAxisVao, m_XAxisVbo, m_Axises.xVertices());
    generateVaoVbo(m_YAxisVao, m_YAxisVbo, m_Axises.yVertices());

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

    // keyboards are processed with imgui
    // todo: maybe process mouse with imgui too?
}

// update cursor hover point / grid / matrices, etc
void Canvas::update()
{
    // todo: calculate delta Cursor X/Y for like selection

    // update cursor datas if cursor position changes
    if (g_LastCursorPosX != g_CursorPosX || g_LastCursorPosY != g_CursorPosY)
    {
        m_Cursor.updateVertices();
        updateVertexArrayBufferData(m_CursorVao, m_CursorVbo, m_Cursor.vertices());
    }

    // hide cursor and draw custom cursor if cursor is inside the canvas (and it's not captured by any floating window).
    if (g_bHideCursor)
    {
        glfwSetInputMode(g_pWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        ImGui::SetMouseCursor(ImGuiMouseCursor_None); // prevent imgui draw the cursor unproperly in ImGui_ImplGlfw_NewFrame
    }

    // update last cursor position
    g_LastCursorPosX = g_CursorPosX;
    g_LastCursorPosY = g_CursorPosY;

    bool bShouldUpdateGrid = false;
    // update canvas OpenGL 3D coordinates by update scale factor (changed through mouse wheel), and update center point at the same time.
    // yoffset is treated just like xoffset (when scrool the mouse wheel and press Shift at the same time)
    if (g_ScrollXOffset == 0)
    {
        g_ScrollXOffset = g_ScrollYOffset;
    }
    g_ScrollYOffset = 0;
    if (g_ScrollXOffset != 0)
    {
        glm::vec3 leftBottomToHoverVec = glm::vec3(g_CanvasLeft, g_CanvasBottom, 0.0f) - g_CurrentHoverPoint;
        glm::vec3 rightTopToHoverVec = glm::vec3(g_CanvasRight, g_CanvasTop, 0.0f) - g_CurrentHoverPoint;

        while (g_ScrollXOffset >= 1)
        {
            g_CanvasScaleFactor *= 0.8f;
            g_ScrollXOffset -= 1;
            leftBottomToHoverVec *= 0.8f;
            rightTopToHoverVec *= 0.8f;
            g_GridAutoAjustFactor *= 0.8f;
        }
        while (g_ScrollXOffset <= -1)
        {
            g_CanvasScaleFactor *= 1.25f;
            g_ScrollXOffset += 1;
            leftBottomToHoverVec *= 1.25f;
            rightTopToHoverVec *= 1.25f;
            g_GridAutoAjustFactor *= 1.25f;
        }
        // adjust grid scale factor
        if (g_GridAutoAjustFactor >= 5.0f)
        {
            g_GridAutoAjustFactor /= 5.0f;
            g_GridScaleFactor *= 5.0f;
        }
        else if (g_GridAutoAjustFactor <= 1.0f)
        {
            g_GridAutoAjustFactor *= 5.0f;
            g_GridScaleFactor /= 5.0f;
        }
        // calculate new center point
        g_CanvasCenterX = (rightTopToHoverVec.x + g_CurrentHoverPoint.x + leftBottomToHoverVec.x + g_CurrentHoverPoint.x) / 2.0f;
        g_CanvasCenterY = (rightTopToHoverVec.y + g_CurrentHoverPoint.y + leftBottomToHoverVec.y + g_CurrentHoverPoint.y) / 2.0f;
        // update cursor data
        m_Cursor.updateVertices();
        updateVertexArrayBufferData(m_CursorVao, m_CursorVbo, m_Cursor.vertices());
        // grid will be updated after canvas coorindates are updated
        bShouldUpdateGrid = true;

        // set scroll offset to 0
        g_ScrollXOffset = 0;
    }

    // calculate canvas OpenGL 3D coordinates
    g_CanvasTop = g_CanvasCenterY + g_CanvasScaleFactor * 0.5f * g_CanvasHeight;
    g_CanvasBottom = g_CanvasCenterY - g_CanvasScaleFactor * 0.5f * g_CanvasHeight;
    g_CanvasLeft = g_CanvasCenterX - g_CanvasScaleFactor * 0.5f * g_CanvasWidth;
    g_CanvasRight = g_CanvasCenterX + g_CanvasScaleFactor * 0.5f * g_CanvasWidth;

    // update grid
    if (bShouldUpdateGrid || !m_bGridUpdatedFirstTime)
    {
        m_bGridUpdatedFirstTime = true;
        m_Grid.updateVertices();
        updateVertexArrayBufferData(m_MainGridVao, m_MainGridVbo, m_Grid.mainGridVertices());
        updateVertexArrayBufferData(m_SubGridVao, m_SubGridVbo, m_Grid.subGridVertices());
    }

    // update axises
    m_Axises.updateVertices();
    updateVertexArrayBufferData(m_XAxisVao, m_XAxisVbo, m_Axises.xVertices());
    updateVertexArrayBufferData(m_YAxisVao, m_YAxisVbo, m_Axises.yVertices());

    // calculate hover point OpenGL 3D coordinates, update it only when it's inside canvas.
    if (g_CursorPosX > 0 && g_CursorPosX < g_CanvasLeftBottomX + g_CanvasWidth &&
        g_CursorPosY > 0 && g_CursorPosY < g_CanvasLeftBottomY + g_CanvasHeight)
    {
        g_CurrentHoverPoint.x = g_CanvasLeft + g_CanvasScaleFactor * (g_CursorPosX - g_CanvasLeftBottomX);
        g_CurrentHoverPoint.y = g_CanvasBottom + g_CanvasScaleFactor * (g_CursorPosY - g_CanvasLeftBottomY);
        g_CurrentHoverPoint.z = 0.0f;
    }

    // calculate matrices
    m_ModelMatrix = glm::mat4(1.0f);
    m_ViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_ProjMatrix = glm::ortho(g_CanvasLeft, g_CanvasRight, g_CanvasBottom, g_CanvasTop, 0.1f, 1000.0f); // orthogonal projection
}

// draw entities: cursor, grid, 
void Canvas::draw()
{
    // clear the background
    glViewport(g_CanvasLeftBottomX, g_CanvasLeftBottomY, g_CanvasWidth, g_CanvasHeight);
    glClearColor(g_CanvasBackgroundColor.x, g_CanvasBackgroundColor.y, g_CanvasBackgroundColor.z, g_CanvasBackgroundColor.w);
    glClear(GL_COLOR_BUFFER_BIT);

    m_BasicPureColorShader.use();
    m_BasicPureColorShader.setMat4("mvMatrix", m_ModelMatrix * m_ViewMatrix);
    m_BasicPureColorShader.setMat4("projMatrix", m_ProjMatrix);
    m_BasicPureColorShader.setVec4("inputColor", 1.0f, 1.0f, 1.0f, 1.0f);
    
    // draw grid
    m_BasicPureColorShader.setVec4("inputColor", g_MainGridColor);
    glBindVertexArray(m_MainGridVao);
    glDrawArrays(GL_LINES, 0, m_Grid.mainGridVertices().size() * 3);
    m_BasicPureColorShader.setVec4("inputColor", g_SubGridColor);
    glBindVertexArray(m_SubGridVao);
    glDrawArrays(GL_LINES, 0, m_Grid.subGridVertices().size() * 3);
    // draw axises
    glBindVertexArray(m_XAxisVao);
    m_BasicPureColorShader.setVec4("inputColor", g_XAxisColor);
    glDrawArrays(GL_LINES, 0, m_Axises.xVertices().size() * 3);
    glBindVertexArray(m_YAxisVao);
    m_BasicPureColorShader.setVec4("inputColor", g_YAxisColor);
    glDrawArrays(GL_LINES, 0, m_Axises.yVertices().size() * 3);
    // draw custom cursor
    glBindVertexArray(m_CursorVao);
    m_BasicPureColorShader.setVec4("inputColor", 1.0f, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINES, 0, m_Cursor.vertices().size() * 3);

    glBindVertexArray(0);
    checkOpenGLError();
}

// generate vao, generate vbo, and bind data to vertex buffer
void Canvas::generateVaoVbo(GLuint& vao, GLuint& vbo, const std::vector<glm::vec3>& vertices)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    checkOpenGLError();
}

// bind new data to existing vertex buffer
void Canvas::updateVertexArrayBufferData(GLuint& vao, GLuint& vbo, const std::vector<glm::vec3>& vertices)
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    checkOpenGLError();
}
