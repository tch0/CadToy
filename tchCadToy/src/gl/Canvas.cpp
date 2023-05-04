#include <vector>
#include <cmath>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <imgui.h>

#include <Canvas.h>
#include <GLFuncs.h>
#include <Global.h>
#include <Logger.h>

// basic shader
static const std::string basicPureColorVertexShader = R"glsl(
#version 410
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec4 vertexColor;
uniform mat4 mvMatrix;
uniform mat4 projMatrix;
out vec4 color;
void main(void)
{
    gl_Position = projMatrix * mvMatrix * vec4(vertexPosition, 1.0);
    color = vertexColor;
}
)glsl";

static const std::string basicPureColorFragmentShader = R"glsl(
#version 410
uniform mat4 mvMatrix;
uniform mat4 projMatrix;
in vec4 color;
out vec4 outColor;
void main(void)
{
    outColor = color;
}
)glsl";

// =========================================================================================================
// ------------------------------------- CustomCursor
// =========================================================================================================
Canvas::CustomCursor::CustomCursor()
{
    m_Vertices.reserve(16);
    m_Colors.reserve(16);
}

// range: 0 ~ 50
void Canvas::CustomCursor::setPickBoxSize(int size)
{
    if (size < 0)
    {
        size = 0;
    }
    else if (size > 50)
    {
        size = 50;
    }
    if (size != m_PickBoxSize)
    {
        m_PickBoxSize = size;
        updateVertices();
    }
}

// range: 10 ~ 200
void Canvas::CustomCursor::setCursorSize(int size)
{
    if (size < 10)
    {
        size = 10;
    }
    else if (size > 200)
    {
        size = 200;
    }
    if (size != m_CursorSize)
    {
        m_CursorSize = size;
        updateVertices();
    }
}

// update cursor datas
void Canvas::CustomCursor::updateVertices()
{
    m_Vertices.resize(0);
    m_Colors.resize(0);
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
        m_Colors.resize(4, g_CursorColor);
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
            m_Colors.resize(8, g_CursorColor);
        }
        m_Vertices.push_back(pickBoxLeftTop);
        m_Vertices.push_back(pickBoxRightTop);
        m_Vertices.push_back(pickBoxRightTop);
        m_Vertices.push_back(pickBoxRightBottom);
        m_Vertices.push_back(pickBoxRightBottom);
        m_Vertices.push_back(pickBoxLeftBottom);
        m_Vertices.push_back(pickBoxLeftBottom);
        m_Vertices.push_back(pickBoxLeftTop);
        for (int i = 0; i < 8; ++i)
        {
            m_Colors.push_back(g_CursorColor);
        }
    }
}

const std::vector<glm::vec3>& Canvas::CustomCursor::vertices()
{
    return m_Vertices;
}

const std::vector<glm::vec4>& Canvas::CustomCursor::colors()
{
    return m_Colors;
}

// =========================================================================================================
// ------------------------------------- Grid
// =========================================================================================================
Canvas::Grid::Grid()
{
    m_Vertices.reserve(500);
    m_Colors.reserve(500);
}

void Canvas::Grid::updateVertices()
{
    m_Vertices.resize(0);
    m_Colors.resize(0);
    float gridLeft = std::floor(g_CanvasLeft / g_GridScaleFactor) * g_GridScaleFactor;
    float gridRight = std::ceil(g_CanvasRight / g_GridScaleFactor) * g_GridScaleFactor;
    float gridBottom = std::floor(g_CanvasBottom / g_GridScaleFactor) * g_GridScaleFactor;
    float gridTop = std::ceil(g_CanvasTop / g_GridScaleFactor) * g_GridScaleFactor;
    // vertical
    for (float x = gridLeft; x <= gridRight; x += g_GridScaleFactor)
    {
        m_Vertices.emplace_back(x, gridBottom, 0.0f);
        m_Vertices.emplace_back(x, gridTop, 0.0f);
        m_Colors.push_back(g_MainGridColor);
        m_Colors.push_back(g_MainGridColor);
        for (int i = 1; i <= 4 && x < gridRight; i++)
        {
            m_Vertices.emplace_back(x + i / 5.0f * g_GridScaleFactor, gridBottom, 0.0f);
            m_Vertices.emplace_back(x + i / 5.0f * g_GridScaleFactor, gridTop, 0.0f);
            m_Colors.push_back(g_SubGridColor);
            m_Colors.push_back(g_SubGridColor);
        }
    }
    // horizontal
    for (float y = gridBottom; y <= gridTop; y += g_GridScaleFactor)
    {
        m_Vertices.emplace_back(gridLeft, y, 0.0f);
        m_Vertices.emplace_back(gridRight, y, 0.0f);
        m_Colors.push_back(g_MainGridColor);
        m_Colors.push_back(g_MainGridColor);
        for (int i = 1; i <= 4 && y < gridTop; i++)
        {
            m_Vertices.emplace_back(gridLeft, y + i / 5.0f * g_GridScaleFactor, 0.0f);
            m_Vertices.emplace_back(gridRight, y + i / 5.0f * g_GridScaleFactor, 0.0f);
            m_Colors.push_back(g_SubGridColor);
            m_Colors.push_back(g_SubGridColor);
        }
    }
}

const std::vector<glm::vec3>& Canvas::Grid::vertices()
{
    return m_Vertices;
}

const std::vector<glm::vec4>& Canvas::Grid::colors()
{
    return m_Colors;
}



// =========================================================================================================
// ------------------------------------- Axises
// =========================================================================================================
Canvas::Axises::Axises()
{
    m_Vertices.reserve(4);
    m_Colors.reserve(4);
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
        m_Vertices.resize(0);
        m_Colors.resize(0);
        m_Vertices.emplace_back(0.0f, 0.0f, 0.0f);
        m_Vertices.emplace_back(m_Width, 0.0f, 0.0f);
        m_Vertices.emplace_back(0.0f, 0.0f, 0.0f);
        m_Vertices.emplace_back(0.0f, m_Height, 0.0f);
        m_Colors.push_back(g_XAxisColor);
        m_Colors.push_back(g_XAxisColor);
        m_Colors.push_back(g_YAxisColor);
        m_Colors.push_back(g_YAxisColor);
    }
}

const std::vector<glm::vec3>& Canvas::Axises::vertices()
{
    return m_Vertices;
}

const std::vector<glm::vec4>& Canvas::Axises::colors()
{
    return m_Colors;
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
    generateVaoVbo(m_CursorVao, m_CursorPosVbo, m_CursorColorVbo, m_Cursor.vertices(), m_Cursor.colors());

    // grid vao, vbo
    m_Grid.updateVertices();
    generateVaoVbo(m_GridVao, m_GridPosVbo, m_GridColorVbo, m_Grid.vertices(), m_Grid.colors());

    // axises vao, vbo
    m_Axises.updateVertices();
    generateVaoVbo(m_AxisesVao, m_AxisesPosVbo, m_AxisesColorVbo, m_Axises.vertices(), m_Axises.colors());
}

// update cursor hover point / grid / matrices, etc
void Canvas::update()
{
    // todo: calculate delta Cursor X/Y for like selection

    // get cursor pos
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse)
    {
        g_CursorPosX = int(io.MousePos.x);
        g_CursorPosY = g_WindowHeight - int(io.MousePos.y);
    }
    
    // update cursor datas if cursor position changes
    if (g_LastCursorPosX != g_CursorPosX || g_LastCursorPosY != g_CursorPosY)
    {
        m_Cursor.updateVertices();
        updateVertexArrayBufferData(m_CursorVao, m_CursorPosVbo, m_CursorColorVbo, m_Cursor.vertices(), m_Cursor.colors());
    }

    // hide cursor and draw custom cursor if cursor is inside the canvas (and it's not captured by any floating window).
    if (!io.WantCaptureMouse)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    }

    // update last cursor position
    g_LastCursorPosX = g_CursorPosX;
    g_LastCursorPosY = g_CursorPosY;

    // get mouse wheel data
    if (!io.WantCaptureMouse)
    {
        g_ScrollXOffset = int(io.MouseWheel);
        g_ScrollYOffset = int(io.MouseWheelH);
    }

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
        updateVertexArrayBufferData(m_CursorVao, m_CursorPosVbo, m_CursorColorVbo, m_Cursor.vertices(), m_Cursor.colors());
        // grid will be updated after canvas coorindates are updated
        bShouldUpdateGrid = true;

        // set scroll offset to 0
        g_ScrollXOffset = 0;
    }

    // deal with panning with middle mouse
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
    {
        ImVec2 delta = io.MouseDelta;
        float deltaX = delta.x;
        float deltaY = -delta.y;
        if (std::abs(deltaX) > 0.0f || std::abs(deltaY) > 0.0f)
        {
            g_CanvasCenterX -= deltaX * g_CanvasScaleFactor;
            g_CanvasCenterY -= deltaY * g_CanvasScaleFactor;
        }
        bShouldUpdateGrid = true;
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
        updateVertexArrayBufferData(m_GridVao, m_GridPosVbo, m_GridColorVbo, m_Grid.vertices(), m_Grid.colors());
    }

    // update axises data when necessary
    m_Axises.updateVertices();
    updateVertexArrayBufferData(m_AxisesVao, m_AxisesPosVbo, m_AxisesColorVbo, m_Axises.vertices(), m_Axises.colors());

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
    glBindVertexArray(m_GridVao);
    glDrawArrays(GL_LINES, 0, m_Grid.vertices().size() * 3);
    // draw axises
    glBindVertexArray(m_AxisesVao);
    glDrawArrays(GL_LINES, 0, m_Axises.vertices().size() * 3);
    // draw custom cursor
    glBindVertexArray(m_CursorVao);
    glDrawArrays(GL_LINES, 0, m_Cursor.vertices().size() * 3);

    glBindVertexArray(0);
    checkOpenGLError();
}

int Canvas::getCursorSize()
{
    return m_Cursor.m_CursorSize;
}

void Canvas::setCursorSize(int size)
{
    m_Cursor.setCursorSize(size);
}

int Canvas::getPickBoxSize()
{
    return m_Cursor.m_PickBoxSize;
}

void Canvas::setPickBoxSize(int size)
{
    m_Cursor.setPickBoxSize(size);
}

// generate vao, generate vbo, and bind data to vertex buffer
void Canvas::generateVaoVbo(GLuint& vao, GLuint& posVbo, GLuint& colorVbo, const std::vector<glm::vec3>& vertices, const std::vector<glm::vec4>& colors)
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    // vertex position
    glGenBuffers(1, &posVbo);
    glBindBuffer(GL_ARRAY_BUFFER, posVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    // vertex color
    glGenBuffers(1, &colorVbo);
    glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    checkOpenGLError();
}

// bind new data to existing vertex buffer
void Canvas::updateVertexArrayBufferData(GLuint& vao, GLuint& posVbo, GLuint& colorVbo, const std::vector<glm::vec3>& vertices, const std::vector<glm::vec4>& colors)
{
    glBindVertexArray(vao);
    // vertex position
    glBindBuffer(GL_ARRAY_BUFFER, posVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    // vertex color
    glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    checkOpenGLError();
}
