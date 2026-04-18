// 对应头文件
#include "CanvasRenderer.h"

// C++ 标准库
#include <cmath>

// 第三方库

// 项目头文件
#include "CommonTypes.h"
#include "Logger.h"
#include "DocManager.h"
#include "InputContext.h"
#include "InputHandler.h"
#include "Renderer.h"

namespace tch {

// Canvas顶点着色器
static const char* CANVAS_VERTEX_SHADER = R"(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aTexCoord;

uniform mat4 uProjection;

out vec4 vColor;
out float vTexCoord;

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
    vTexCoord = aTexCoord;
}
)";

// Canvas片段着色器
static const char* CANVAS_FRAGMENT_SHADER = R"(
#version 330 core

uniform sampler1D uDashTexture;
uniform int uIsDashed;
uniform float uDashScale;

in vec4 vColor;
in float vTexCoord;
out vec4 FragColor;

void main()
{
    if (uIsDashed == 0) {
        FragColor = vColor;
    } else {
        float texPos = vTexCoord * uDashScale;
        float alpha = texture(uDashTexture, texPos).r;
        if (alpha < 0.5) discard;
        FragColor = vColor;
    }
}
)";

// 栅格颜色常量
static glm::vec4 s_mainGridColor(54.0f/255.0f, 61.0f/255.0f, 78.0f/255.0f, 1.0f);   // 主栅格颜色 RGB: 54,61,78
static glm::vec4 s_subGridColor(38.0f/255.0f, 45.0f/255.0f, 55.0f/255.0f, 1.0f);    // 子栅格颜色 RGB: 38,45,55

// 坐标轴颜色常量
static glm::vec4 s_xAxisColor(97.0f/255.0f, 37.0f/255.0f, 39.0f/255.0f, 1.0f);      // X轴颜色 RGB: 97,37,39
static glm::vec4 s_yAxisColor(34.0f/255.0f, 89.0f/255.0f, 41.0f/255.0f, 1.0f);      // Y轴颜色 RGB: 34,89,41

// 选择区域填充颜色常量
static glm::vec4 s_windowSelectionColor(20.0f/255.0f, 90.0f/255.0f, 223.0f/255.0f, 0.1f);   // 窗口选择填充色 RGB: 20,90,223
static glm::vec4 s_crossingSelectionColor(0.0f, 1.0f, 0.0f, 0.1f);                          // 交叉选择填充色（绿色）

// StencilFillGuard实现
StencilFillGuard::StencilFillGuard() {
    // 保存状态
    m_stencilEnabled = glIsEnabled(GL_STENCIL_TEST);
    glGetBooleanv(GL_COLOR_WRITEMASK, m_colorMask);
    glGetIntegerv(GL_STENCIL_WRITEMASK, &m_stencilWriteMask);
    glGetIntegerv(GL_STENCIL_FUNC, &m_stencilFunc);
    glGetIntegerv(GL_STENCIL_REF, &m_stencilRef);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, &m_stencilValueMask);
    glGetIntegerv(GL_STENCIL_FAIL, &m_stencilFail);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &m_stencilZFail);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &m_stencilZPass);
    m_depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &m_depthMask);
    m_blendEnabled = glIsEnabled(GL_BLEND);
    
    // 设置模板状态
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
    glStencilMask(0xFF);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    
    if (m_depthTestEnabled) {
        glDepthMask(GL_FALSE);
    }
}

StencilFillGuard::~StencilFillGuard() {
    // 恢复状态
    glColorMask(m_colorMask[0], m_colorMask[1], m_colorMask[2], m_colorMask[3]);
    if (m_depthTestEnabled) {
        glDepthMask(m_depthMask);
    }
    glStencilMask(m_stencilWriteMask);
    glStencilFunc(m_stencilFunc, m_stencilRef, m_stencilValueMask);
    glStencilOp(m_stencilFail, m_stencilZFail, m_stencilZPass);
    
    if (!m_stencilEnabled) { 
        glDisable(GL_STENCIL_TEST);
    }
    if (!m_blendEnabled) {
        glDisable(GL_BLEND);
    }
}

void StencilFillGuard::setupFill() {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (m_depthTestEnabled) {
        glDepthMask(GL_TRUE);
    }
    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);  // 模板值非零时绘制（奇数次穿越）
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0x00);
}

// AAGuard实现
AAGuard::AAGuard(bool enable) {
    m_lineSmoothEnabled = glIsEnabled(GL_LINE_SMOOTH);
    m_polygonSmoothEnabled = glIsEnabled(GL_POLYGON_SMOOTH);
    m_multisampleEnabled = glIsEnabled(GL_MULTISAMPLE);
    
    if (enable) {
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POLYGON_SMOOTH);
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_POLYGON_SMOOTH);
        glDisable(GL_MULTISAMPLE);
    }
}

AAGuard::~AAGuard() {
    if (m_lineSmoothEnabled) {
        glEnable(GL_LINE_SMOOTH);
    } else {
        glDisable(GL_LINE_SMOOTH);
    }
    if (m_polygonSmoothEnabled) {
        glEnable(GL_POLYGON_SMOOTH);
    } else {
        glDisable(GL_POLYGON_SMOOTH);
    }
    if (m_multisampleEnabled) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }
}

// 构造函数
CanvasRenderer::CanvasRenderer() 
    : m_vao(0)
    , m_vbo(0)
    , m_dashTexture(0)
    , m_mvpLocation(-1)
    , m_isDashedLocation(-1)
    , m_dashScaleLocation(-1)
{
}

// 析构函数
CanvasRenderer::~CanvasRenderer() {
    cleanup();
}

// 初始化虚线纹理
void CanvasRenderer::initDashTexture() {
    // 4白4透明虚线模式
    unsigned char pattern[8] = {255, 255, 255, 255, 0, 0, 0, 0};
    
    glGenTextures(1, &m_dashTexture);
    glBindTexture(GL_TEXTURE_1D, m_dashTexture);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RED, 8, 0, GL_RED, GL_UNSIGNED_BYTE, pattern);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    LOG_INFO("Created dash texture: {}", m_dashTexture);
}

// 初始化渲染器
bool CanvasRenderer::initialize() {
    // 使用嵌入的着色器源码
    m_canvasShader.setShaderSource(CANVAS_VERTEX_SHADER, CANVAS_FRAGMENT_SHADER);
    
    GLuint shaderId = m_canvasShader.getShaderId();
    
    LOG_INFO("Canvas shader program ID: {}", shaderId);
    
    if (shaderId == 0) {
        LOG_ERROR("Failed to create canvas shader program");
        return false;
    }
    
    // 获取uniform位置
    m_mvpLocation = glGetUniformLocation(shaderId, "uProjection");
    if (m_mvpLocation == -1) {
        LOG_WARNING("Failed to get uProjection uniform location");
    } else {
        LOG_INFO("Canvas shader uProjection location: {}", m_mvpLocation);
    }
    
    m_isDashedLocation = glGetUniformLocation(shaderId, "uIsDashed");
    if (m_isDashedLocation == -1) {
        LOG_WARNING("Failed to get uIsDashed uniform location");
    } else {
        LOG_INFO("Canvas shader uIsDashed location: {}", m_isDashedLocation);
    }
    
    m_dashScaleLocation = glGetUniformLocation(shaderId, "uDashScale");
    if (m_dashScaleLocation == -1) {
        LOG_WARNING("Failed to get uDashScale uniform location");
    } else {
        LOG_INFO("Canvas shader uDashScale location: {}", m_dashScaleLocation);
    }
    
    // 创建VAO和VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    
    LOG_INFO("Created VAO: {}, VBO: {}", m_vao, m_vbo);
    
    // 配置顶点属性
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    // 分配动态缓冲区（10000个顶点）
    glBufferData(GL_ARRAY_BUFFER, 10000 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
    
    // 位置属性（location = 0）
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // 颜色属性（location = 1）
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));
    
    // 纹理坐标属性（location = 2）
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 6));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // 初始化虚线纹理
    initDashTexture();
    
    // 预分配顶点缓冲区
    m_vertices.reserve(10000);
    
    LOG_INFO("CanvasRenderer initialized successfully");
    return true;
}

// 清理资源
void CanvasRenderer::cleanup() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    
    if (m_dashTexture) {
        glDeleteTextures(1, &m_dashTexture);
        m_dashTexture = 0;
    }
}

// 设置投影矩阵
void CanvasRenderer::setProjection(const glm::mat4& projection) {
    m_projection = projection;
}

// 设置视图矩阵，CanvasRender根据屏幕坐标绘制，视图矩阵式单位矩阵，不需要用到
void CanvasRenderer::setView(const glm::mat4& view) {
    m_view = view;
}

// 清空顶点缓冲
void CanvasRenderer::clearVertices() {
    m_vertices.clear();
}

// 添加顶点到缓冲区（texCoord=0）
void CanvasRenderer::addVertex(const glm::vec2& pos, const glm::vec4& color) {
    m_vertices.push_back({pos, color, 0.0f});
}

// 添加顶点到缓冲区（带纹理坐标）
void CanvasRenderer::addVertex(const glm::vec2& pos, const glm::vec4& color, float texCoord) {
    m_vertices.push_back({pos, color, texCoord});
}

// 上传顶点数据到GPU
void CanvasRenderer::flushVertices() {
    if (m_vertices.empty()) {
        return;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data());
}

// 设置虚线模式
void CanvasRenderer::setDashedMode(bool isDashed, float period) {
    glUniform1i(m_isDashedLocation, isDashed ? 1 : 0);
    glUniform1f(m_dashScaleLocation, 1.0f / period);
}

// 多边形填充（使用模板缓冲支持凹多边形）
void CanvasRenderer::drawPolygonFill(const std::vector<glm::vec2>& points, const glm::vec4& fillColor) {
    if (points.size() < 3) {
        return;
    }
    
    StencilFillGuard guard;
    
    // 绘制多边形边界（三角形扇）
    clearVertices();
    for (size_t i = 1; i < points.size() - 1; i++) {
        addVertex(points[0], fillColor);
        addVertex(points[i], fillColor);
        addVertex(points[i + 1], fillColor);
    }
    flushVertices();
    setDashedMode(false);
    
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
    
    // 切换到填充模式
    guard.setupFill();
    
    // 计算包围盒（扩大边界确保覆盖所有模板区域）
    float minX = points[0].x, maxX = points[0].x;
    float minY = points[0].y, maxY = points[0].y;
    for (const auto& p : points) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    
    // 扩大边界10像素
    minX -= 10.0f;
    maxX += 10.0f;
    minY -= 10.0f;
    maxY += 10.0f;
    
    // 绘制填充矩形
    clearVertices();
    addVertex(glm::vec2(minX, minY), fillColor);
    addVertex(glm::vec2(maxX, minY), fillColor);
    addVertex(glm::vec2(maxX, maxY), fillColor);
    addVertex(glm::vec2(minX, maxY), fillColor);
    flushVertices();
    
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

// 绘制栅格
void CanvasRenderer::drawGrid() {
    AAGuard aaGuard(false);
    
    // 检查是否显示栅格
    auto& doc = DocManager::getCurrentDocument();
    if (!doc.isShowGrid()) {
        return;
    }
    
    clearVertices();
    
    // 获取视口范围
    int viewportLeft, viewportTop, viewportRight, viewportBottom;
    doc.getTransformManager().getViewport().getViewport(viewportLeft, viewportTop, viewportRight, viewportBottom);
    
    int viewportWidth = viewportRight - viewportLeft;
    int viewportHeight = viewportBottom - viewportTop;
    
    // 将视口边界转换为世界坐标
    glm::dvec3 worldMin = doc.getTransformManager().screenToWorld(glm::vec2(viewportLeft, viewportBottom));
    glm::dvec3 worldMax = doc.getTransformManager().screenToWorld(glm::vec2(viewportRight, viewportTop));
    
    double worldWidth = worldMax.x - worldMin.x;
    double worldHeight = worldMax.y - worldMin.y;
    
    // 基础栅格大小为10个世界单位
    const double baseGridSize = 10.0;
    double mainGridSize, subGridSize;
    
    // 计算栅格在屏幕上的像素大小
    double gridScreenSizeX = (baseGridSize / worldWidth) * viewportWidth;
    double gridScreenSizeY = (baseGridSize / worldHeight) * viewportHeight;
    double gridScreenSize = std::min(gridScreenSizeX, gridScreenSizeY);
    
    // 根据屏幕像素大小动态调整栅格级别
    // 目标：保持栅格线在屏幕上的间距在50-250像素之间
    if (gridScreenSize < 50.0) {
        // 当前栅格太小，需要增加栅格级别（放大栅格间距）
        double testSize = baseGridSize;
        double testScreenSize = gridScreenSize;
        while (testScreenSize < 50.0) {
            testSize *= 5.0;
            testScreenSize *= 5.0;
        }
        mainGridSize = testSize;
        subGridSize = mainGridSize / 5.0;
    }
    else if (gridScreenSize > 250.0) {
        // 当前栅格太大，需要减少栅格级别（缩小栅格间距）
        double testSize = baseGridSize;
        double testScreenSize = gridScreenSize;
        while (testScreenSize > 250.0) {
            testSize /= 5.0;
            testScreenSize /= 5.0;
        }
        mainGridSize = testSize;
        subGridSize = mainGridSize / 5.0;
    }
    else {
        // 栅格大小在合理范围内
        mainGridSize = baseGridSize;
        subGridSize = mainGridSize / 5.0;
    }
    
    // 计算起始位置，确保栅格线与原点对齐
    double startX = floor(worldMin.x / subGridSize) * subGridSize;
    double startY = floor(worldMin.y / subGridSize) * subGridSize;
    
    // 绘制子栅格垂直线
    for (double x = startX; x <= worldMax.x; x += subGridSize) {
        glm::vec2 screenStart = doc.getTransformManager().worldToScreen(glm::dvec3(x, worldMin.y, 0.0));
        glm::vec2 screenEnd = doc.getTransformManager().worldToScreen(glm::dvec3(x, worldMax.y, 0.0));
        addVertex(screenStart, s_subGridColor);
        addVertex(screenEnd, s_subGridColor);
    }
    
    // 绘制子栅格水平线
    for (double y = startY; y <= worldMax.y; y += subGridSize) {
        glm::vec2 screenStart = doc.getTransformManager().worldToScreen(glm::dvec3(worldMin.x, y, 0.0));
        glm::vec2 screenEnd = doc.getTransformManager().worldToScreen(glm::dvec3(worldMax.x, y, 0.0));
        addVertex(screenStart, s_subGridColor);
        addVertex(screenEnd, s_subGridColor);
    }
    
    // 计算主栅格起始位置
    double mainStartX = floor(worldMin.x / mainGridSize) * mainGridSize;
    double mainStartY = floor(worldMin.y / mainGridSize) * mainGridSize;
    
    // 绘制主栅格垂直线
    for (double x = mainStartX; x <= worldMax.x; x += mainGridSize) {
        glm::vec2 screenStart = doc.getTransformManager().worldToScreen(glm::dvec3(x, worldMin.y, 0.0));
        glm::vec2 screenEnd = doc.getTransformManager().worldToScreen(glm::dvec3(x, worldMax.y, 0.0));
        addVertex(screenStart, s_mainGridColor);
        addVertex(screenEnd, s_mainGridColor);
    }
    
    // 绘制主栅格水平线
    for (double y = mainStartY; y <= worldMax.y; y += mainGridSize) {
        glm::vec2 screenStart = doc.getTransformManager().worldToScreen(glm::dvec3(worldMin.x, y, 0.0));
        glm::vec2 screenEnd = doc.getTransformManager().worldToScreen(glm::dvec3(worldMax.x, y, 0.0));
        addVertex(screenStart, s_mainGridColor);
        addVertex(screenEnd, s_mainGridColor);
    }
    
    flushVertices();
    
    // 禁用深度测试，启用混合
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 使用shader绘制
    m_canvasShader.use();
    glUniformMatrix4fv(m_mvpLocation, 1, GL_FALSE, &m_projection[0][0]);
    setDashedMode(false);
    
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);
    
    // 恢复深度测试
    glEnable(GL_DEPTH_TEST);
}

// 绘制XY坐标轴
void CanvasRenderer::drawAxes() {
    AAGuard aaGuard(false);
    
    // 检查是否显示坐标轴
    auto& doc = DocManager::getCurrentDocument();
    if (!doc.isShowAxes()) {
        return;
    }
    
    clearVertices();
    
    // 获取视口范围
    int viewportLeft, viewportTop, viewportRight, viewportBottom;
    doc.getTransformManager().getViewport().getViewport(viewportLeft, viewportTop, viewportRight, viewportBottom);
    
    // 计算世界原点在屏幕上的位置
    glm::vec2 originScreen = doc.getTransformManager().worldToScreen(glm::dvec3(0.0, 0.0, 0.0));
    
    // 只绘制在视口范围内的部分
    if (originScreen.x >= viewportLeft && originScreen.x <= viewportRight &&
        originScreen.y >= viewportTop && originScreen.y <= viewportBottom) {
        // 绘制X轴（正半轴，向右）
        addVertex(glm::vec2(originScreen.x, originScreen.y), s_xAxisColor);
        addVertex(glm::vec2(static_cast<float>(viewportRight), originScreen.y), s_xAxisColor);
        
        // 绘制Y轴（正半轴，向上，由于Y轴朝下，所以是viewportTop）
        addVertex(glm::vec2(originScreen.x, originScreen.y), s_yAxisColor);
        addVertex(glm::vec2(originScreen.x, static_cast<float>(viewportTop)), s_yAxisColor);
    }
    
    flushVertices();
    
    // 禁用深度测试，启用混合
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 使用shader绘制
    m_canvasShader.use();
    glUniformMatrix4fv(m_mvpLocation, 1, GL_FALSE, &m_projection[0][0]);
    setDashedMode(false);
    
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);
    
    // 恢复深度测试
    glEnable(GL_DEPTH_TEST);
}

// 绘制光标
void CanvasRenderer::drawCursor() {
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    
    clearVertices();
    
    // 获取光标屏幕位置
    glm::vec2 cursorScreenPos = InputHandler::getCursorPosition();
    glm::vec4 cursorColor(1.0f, 1.0f, 1.0f, 1.0f);  // 白色
    
    // 获取光标尺寸配置
    float crossCursorSize = Renderer::getCrossCursorSize();
    float pickBoxSize = Renderer::getPickBoxSize();
    
    // 手掌光标需要抗锯齿，其他光标禁用抗锯齿
    bool needAA = (interactionData.cursorMode == CursorMode::kPanning);
    AAGuard aaGuard(needAA);
    
    switch (interactionData.cursorMode) {
        case CursorMode::kDefault:
        case CursorMode::kCrosshair: {
            // 默认模式：十字光标 + 拾取框
            // 十字光标模式：仅十字光标，无拾取框
            float effectivePickBoxSize = (interactionData.cursorMode == CursorMode::kCrosshair) ? 0.0f : pickBoxSize;
            
            // 绘制拾取框（正方形框）
            if (effectivePickBoxSize > 0) {
                float halfSize = effectivePickBoxSize;
                // 拾取框四条边
                addVertex(glm::vec2(cursorScreenPos.x - halfSize + 0.5f, cursorScreenPos.y - halfSize + 0.5f), cursorColor);
                addVertex(glm::vec2(cursorScreenPos.x + halfSize + 0.5f, cursorScreenPos.y - halfSize + 0.5f), cursorColor);
                addVertex(glm::vec2(cursorScreenPos.x + halfSize + 0.5f, cursorScreenPos.y - halfSize + 0.5f), cursorColor);
                addVertex(glm::vec2(cursorScreenPos.x + halfSize + 0.5f, cursorScreenPos.y + halfSize + 0.5f), cursorColor);
                addVertex(glm::vec2(cursorScreenPos.x + halfSize + 0.5f, cursorScreenPos.y + halfSize + 0.5f), cursorColor);
                addVertex(glm::vec2(cursorScreenPos.x - halfSize + 0.5f, cursorScreenPos.y + halfSize + 0.5f), cursorColor);
                addVertex(glm::vec2(cursorScreenPos.x - halfSize + 0.5f, cursorScreenPos.y + halfSize + 0.5f), cursorColor);
                addVertex(glm::vec2(cursorScreenPos.x - halfSize + 0.5f, cursorScreenPos.y - halfSize + 0.5f), cursorColor);
            }
            
            // 绘制十字光标线
            if (crossCursorSize > 0) {
                float lineLength = crossCursorSize;
                float startOffset = effectivePickBoxSize;  // 从拾取框外开始
                float outerLength = lineLength - startOffset;
                
                if (outerLength > 0) {
                    // 计算各方向起止位置
                    float topStart = cursorScreenPos.y - startOffset + 0.5f;
                    float topEnd = cursorScreenPos.y - lineLength + 0.5f;
                    float bottomStart = cursorScreenPos.y + startOffset + 0.5f;
                    float bottomEnd = cursorScreenPos.y + lineLength + 0.5f;
                    float leftStart = cursorScreenPos.x - startOffset + 0.5f;
                    float leftEnd = cursorScreenPos.x - lineLength + 0.5f;
                    float rightStart = cursorScreenPos.x + startOffset + 0.5f;
                    float rightEnd = cursorScreenPos.x + lineLength + 0.5f;
                    
                    // 上、下、左、右四条线段
                    addVertex(glm::vec2(cursorScreenPos.x + 0.5f, topStart), cursorColor);
                    addVertex(glm::vec2(cursorScreenPos.x + 0.5f, topEnd), cursorColor);
                    addVertex(glm::vec2(cursorScreenPos.x + 0.5f, bottomStart), cursorColor);
                    addVertex(glm::vec2(cursorScreenPos.x + 0.5f, bottomEnd), cursorColor);
                    addVertex(glm::vec2(leftStart, cursorScreenPos.y + 0.5f), cursorColor);
                    addVertex(glm::vec2(leftEnd, cursorScreenPos.y + 0.5f), cursorColor);
                    addVertex(glm::vec2(rightStart, cursorScreenPos.y + 0.5f), cursorColor);
                    addVertex(glm::vec2(rightEnd, cursorScreenPos.y + 0.5f), cursorColor);
                }
            }
            break;
        }
        case CursorMode::kPickbox: {
            // 拾取框模式：仅绘制拾取框
            float halfSize = pickBoxSize;
            addVertex(glm::vec2(cursorScreenPos.x - halfSize + 0.5f, cursorScreenPos.y - halfSize + 0.5f), cursorColor);
            addVertex(glm::vec2(cursorScreenPos.x + halfSize + 0.5f, cursorScreenPos.y - halfSize + 0.5f), cursorColor);
            addVertex(glm::vec2(cursorScreenPos.x + halfSize + 0.5f, cursorScreenPos.y - halfSize + 0.5f), cursorColor);
            addVertex(glm::vec2(cursorScreenPos.x + halfSize + 0.5f, cursorScreenPos.y + halfSize + 0.5f), cursorColor);
            addVertex(glm::vec2(cursorScreenPos.x + halfSize + 0.5f, cursorScreenPos.y + halfSize + 0.5f), cursorColor);
            addVertex(glm::vec2(cursorScreenPos.x - halfSize + 0.5f, cursorScreenPos.y + halfSize + 0.5f), cursorColor);
            addVertex(glm::vec2(cursorScreenPos.x - halfSize + 0.5f, cursorScreenPos.y + halfSize + 0.5f), cursorColor);
            addVertex(glm::vec2(cursorScreenPos.x - halfSize + 0.5f, cursorScreenPos.y - halfSize + 0.5f), cursorColor);
            break;
        }
        case CursorMode::kPanning: {
            // 平移模式：绘制手掌光标（启用抗锯齿）
            float handSize = 25.0f;
            float x = cursorScreenPos.x + 0.5f;
            float y = cursorScreenPos.y + 0.5f;
            
            // 手掌轮廓（LINE_LOOP 转换为 LINES：每相邻两点连线，最后闭合）
            // 点序列：底部左 -> 底部右 -> 右侧 -> 顶部右 -> 顶部中 -> 顶部左 -> 左侧 -> 底部左(闭合)
            glm::vec2 p0(x - handSize * 0.4f, y + handSize * 0.2f);  // 底部左
            glm::vec2 p1(x + handSize * 0.4f, y + handSize * 0.2f);  // 底部右
            glm::vec2 p2(x + handSize * 0.3f, y - handSize * 0.3f);  // 右侧
            glm::vec2 p3(x + handSize * 0.1f, y - handSize * 0.4f);  // 顶部右
            glm::vec2 p4(x, y - handSize * 0.45f);                   // 顶部中
            glm::vec2 p5(x - handSize * 0.1f, y - handSize * 0.4f);  // 顶部左
            glm::vec2 p6(x - handSize * 0.3f, y - handSize * 0.3f);  // 左侧
            
            // LINE_LOOP: p0-p1, p1-p2, p2-p3, p3-p4, p4-p5, p5-p6, p6-p0
            addVertex(p0, cursorColor); addVertex(p1, cursorColor);
            addVertex(p1, cursorColor); addVertex(p2, cursorColor);
            addVertex(p2, cursorColor); addVertex(p3, cursorColor);
            addVertex(p3, cursorColor); addVertex(p4, cursorColor);
            addVertex(p4, cursorColor); addVertex(p5, cursorColor);
            addVertex(p5, cursorColor); addVertex(p6, cursorColor);
            addVertex(p6, cursorColor); addVertex(p0, cursorColor);  // 闭合
            
            // 手指（GL_LINES）
            // 拇指
            addVertex(glm::vec2(x - handSize * 0.25f, y + handSize * 0.1f), cursorColor);
            addVertex(glm::vec2(x - handSize * 0.35f, y + handSize * 0.15f), cursorColor);
            // 食指
            addVertex(glm::vec2(x + handSize * 0.25f, y - handSize * 0.1f), cursorColor);
            addVertex(glm::vec2(x + handSize * 0.3f, y - handSize * 0.35f), cursorColor);
            // 中指
            addVertex(glm::vec2(x + handSize * 0.08f, y - handSize * 0.15f), cursorColor);
            addVertex(glm::vec2(x + handSize * 0.12f, y - handSize * 0.4f), cursorColor);
            // 无名指
            addVertex(glm::vec2(x - handSize * 0.08f, y - handSize * 0.15f), cursorColor);
            addVertex(glm::vec2(x - handSize * 0.04f, y - handSize * 0.4f), cursorColor);
            // 小指
            addVertex(glm::vec2(x - handSize * 0.25f, y - handSize * 0.1f), cursorColor);
            addVertex(glm::vec2(x - handSize * 0.2f, y - handSize * 0.35f), cursorColor);
            break;
        }
    }
    
    flushVertices();
    
    // 禁用深度测试，启用混合
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 使用shader绘制
    m_canvasShader.use();
    glUniformMatrix4fv(m_mvpLocation, 1, GL_FALSE, &m_projection[0][0]);
    setDashedMode(false);
    
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);
    
    // 恢复深度测试
    glEnable(GL_DEPTH_TEST);
}

// 绘制光标标记
void CanvasRenderer::drawCursorMarker() {
    AAGuard aaGuard(false);
    
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    
    // 检查是否有标记需要绘制
    if (interactionData.cursorMarker == CursorMarker::kNone) {
        return;
    }
    
    clearVertices();
    
    // 获取光标位置和拾取框大小
    glm::vec2 cursorScreenPos = InputHandler::getCursorPosition();
    float pickBoxSize = Renderer::getPickBoxSize();
    
    // 标记位置与光标位置相同
    glm::vec2 markerPos = cursorScreenPos;
    markerPos.x += pickBoxSize + 10.0f;
    markerPos.y -= pickBoxSize + 10.0f;
    
    glm::vec4 markerColor(1.0f, 1.0f, 1.0f, 1.0f);
    
    m_canvasShader.use();
    glUniformMatrix4fv(m_mvpLocation, 1, GL_FALSE, &m_projection[0][0]);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 绑定虚线纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, m_dashTexture);
    
    switch (interactionData.cursorMarker) {
        case CursorMarker::kLocked: {
            // 锁标记：由下方长方形和上方锁柱组成
            float rectWidth = 10.0f;   // 长方形宽度
            float rectHeight = 6.0f;   // 长方形高度
            float rectX = markerPos.x - rectWidth * 0.5f;  // 长方形左边界
            float rectY = markerPos.y - rectHeight * 0.5f; // 长方形上边界
            float lockPostHeight = 5.0f;  // 锁柱高度
            float leftPostX = rectX + 2.0f;   // 左侧锁柱位置
            float rightPostX = rectX + rectWidth - 1.0f;  // 右侧锁柱位置
            
            // 绘制下方长方形（实心）
            addVertex(glm::vec2(rectX, rectY), markerColor);                      // 左上角
            addVertex(glm::vec2(rectX + rectWidth, rectY), markerColor);          // 右上角
            addVertex(glm::vec2(rectX + rectWidth, rectY + rectHeight), markerColor);  // 右下角
            addVertex(glm::vec2(rectX, rectY + rectHeight), markerColor);         // 左下角
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            
            clearVertices();
            
            // 绘制锁柱（三根线段：左侧、顶部、右侧）
            addVertex(glm::vec2(leftPostX, rectY), markerColor);                  // 左侧起点
            addVertex(glm::vec2(leftPostX, rectY - lockPostHeight), markerColor); // 左侧终点（顶部）
            addVertex(glm::vec2(rightPostX, rectY - lockPostHeight), markerColor);// 右侧终点（顶部）
            addVertex(glm::vec2(rightPostX, rectY), markerColor);                 // 右侧起点
            
            flushVertices();
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINE_STRIP, 0, 4);
            glBindVertexArray(0);
            break;
        }
        case CursorMarker::kOrthogonal: {
            // 正交标记：由一条垂直线和一条水平线组成，呈倒T字形
            float lineLength = 8.0f;         // 水平线段长度
            float verticalLineLength = 6.0f; // 垂直线段长度
            
            // 绘制垂直线（从中心向下）
            addVertex(glm::vec2(markerPos.x, markerPos.y), markerColor);
            addVertex(glm::vec2(markerPos.x, markerPos.y + verticalLineLength), markerColor);
            // 垂直线向右偏移1像素（模拟2像素宽度）
            addVertex(glm::vec2(markerPos.x + 1.0f, markerPos.y), markerColor);
            addVertex(glm::vec2(markerPos.x + 1.0f, markerPos.y + verticalLineLength), markerColor);
            
            // 绘制水平线（在垂直线下端）
            addVertex(glm::vec2(markerPos.x - lineLength * 0.5f, markerPos.y + verticalLineLength), markerColor);
            addVertex(glm::vec2(markerPos.x + lineLength * 0.5f, markerPos.y + verticalLineLength), markerColor);
            // 水平线向上偏移1像素（模拟2像素宽度）
            addVertex(glm::vec2(markerPos.x - lineLength * 0.5f, markerPos.y + verticalLineLength - 1.0f), markerColor);
            addVertex(glm::vec2(markerPos.x + lineLength * 0.5f, markerPos.y + verticalLineLength - 1.0f), markerColor);
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
            glBindVertexArray(0);
            break;
        }
        case CursorMarker::kErase: {
            // 删除标记：红色X形，由两条对角线组成
            float lineLength = 10.0f;  // 线段长度
            glm::vec4 eraseColor(1.0f, 0.0f, 0.0f, 1.0f);  // 红色
            
            // 绘制第一条对角线（左上到右下）
            addVertex(glm::vec2(markerPos.x - lineLength * 0.5f, markerPos.y - lineLength * 0.5f), eraseColor);
            addVertex(glm::vec2(markerPos.x + lineLength * 0.5f, markerPos.y + lineLength * 0.5f), eraseColor);
            // 绘制第二条对角线（右上到左下）
            addVertex(glm::vec2(markerPos.x + lineLength * 0.5f, markerPos.y - lineLength * 0.5f), eraseColor);
            addVertex(glm::vec2(markerPos.x - lineLength * 0.5f, markerPos.y + lineLength * 0.5f), eraseColor);
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
            glBindVertexArray(0);
            break;
        }
        case CursorMarker::kCopy: {
            // 复制标记：由两个重叠的矩形组成，表示复制
            float rectSize = 8.0f;  // 矩形大小
            float offset = 2.0f;    // 偏移量
            
            // 绘制第一个矩形（实心，位于下方）
            addVertex(glm::vec2(markerPos.x - rectSize * 0.5f + 0.5f, markerPos.y - rectSize * 0.5f + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + rectSize * 0.5f + 0.5f, markerPos.y - rectSize * 0.5f + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + rectSize * 0.5f + 0.5f, markerPos.y + rectSize * 0.5f + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x - rectSize * 0.5f + 0.5f, markerPos.y + rectSize * 0.5f + 0.5f), markerColor);
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            
            clearVertices();
            
            // 绘制第二个矩形的替代线段（位于右上方，表示复制的副本）
            // 水平线段
            addVertex(glm::vec2(markerPos.x + rectSize * 0.5f + offset + 0.5f, markerPos.y + rectSize * 0.5f + offset + 0.5f + 1.0f), markerColor);
            addVertex(glm::vec2(markerPos.x + rectSize * 0.5f + offset - rectSize + 0.5f, markerPos.y + rectSize * 0.5f + offset + 0.5f + 1.0f), markerColor);
            // 垂直线段
            addVertex(glm::vec2(markerPos.x + rectSize * 0.5f + offset + 0.5f, markerPos.y + rectSize * 0.5f + offset + 0.5f + 1.0f), markerColor);
            addVertex(glm::vec2(markerPos.x + rectSize * 0.5f + offset + 0.5f, markerPos.y + rectSize * 0.5f + offset - rectSize + 0.5f + 2.0f), markerColor);
            
            flushVertices();
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINES, 0, 4);
            glBindVertexArray(0);
            break;
        }
        case CursorMarker::kMove: {
            // 移动标记：四向箭头，表示可以向四个方向移动
            float lineLength = 8.0f;   // 线段长度
            float arrowLength = 3.0f;  // 箭头长度
            
            // 绘制上部分线段
            addVertex(glm::vec2(markerPos.x + 0.5f, markerPos.y - lineLength + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + 0.5f, markerPos.y - arrowLength + 0.5f), markerColor);
            // 绘制下部分线段
            addVertex(glm::vec2(markerPos.x + 0.5f, markerPos.y + arrowLength + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + 0.5f, markerPos.y + lineLength + 1.0f + 0.5f), markerColor);
            // 绘制左部分线段
            addVertex(glm::vec2(markerPos.x - lineLength + 0.5f, markerPos.y + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x - arrowLength + 0.5f, markerPos.y + 0.5f), markerColor);
            // 绘制右部分线段
            addVertex(glm::vec2(markerPos.x + arrowLength + 0.5f, markerPos.y + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + lineLength + 1.0f + 0.5f, markerPos.y + 0.5f), markerColor);
            
            // 绘制箭头（上）
            addVertex(glm::vec2(markerPos.x + 0.5f, markerPos.y - arrowLength + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x - arrowLength + 0.5f, markerPos.y - arrowLength + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + 0.5f, markerPos.y - arrowLength + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + arrowLength + 0.5f, markerPos.y - arrowLength + 0.5f), markerColor);
            // 绘制箭头（下）
            addVertex(glm::vec2(markerPos.x + 0.5f, markerPos.y + arrowLength + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x - arrowLength + 0.5f, markerPos.y + arrowLength + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + 0.5f, markerPos.y + arrowLength + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + arrowLength + 0.5f, markerPos.y + arrowLength + 0.5f), markerColor);
            // 绘制箭头（左）
            addVertex(glm::vec2(markerPos.x - arrowLength + 0.5f, markerPos.y + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x - arrowLength + 0.5f, markerPos.y - arrowLength + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x - arrowLength + 0.5f, markerPos.y + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x - arrowLength + 0.5f, markerPos.y + arrowLength + 0.5f), markerColor);
            // 绘制箭头（右）
            addVertex(glm::vec2(markerPos.x + arrowLength + 0.5f, markerPos.y + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + arrowLength + 0.5f, markerPos.y - arrowLength + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + arrowLength + 0.5f, markerPos.y + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x + arrowLength + 0.5f, markerPos.y + arrowLength + 0.5f), markerColor);
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
            glBindVertexArray(0);
            break;
        }
        case CursorMarker::kRotate: {
            // 旋转标记：由矩形框和箭头组成，表示旋转操作
            float boxSize = 10.0f;   // 矩形框大小
            float arrowSize = 6.0f;  // 箭头大小
            
            // 绘制矩形框
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f + 0.5f, markerPos.y - boxSize * 0.5f + 0.5f), markerColor);  // 左上角
            addVertex(glm::vec2(markerPos.x + boxSize * 0.5f + 0.5f, markerPos.y - boxSize * 0.5f + 0.5f), markerColor);  // 右上角
            addVertex(glm::vec2(markerPos.x + boxSize * 0.5f + 0.5f, markerPos.y + boxSize * 0.5f + 0.5f), markerColor);  // 右下角
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f + 0.5f, markerPos.y + boxSize * 0.5f + 0.5f), markerColor);  // 左下角
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINE_STRIP, 0, 4);
            
            clearVertices();
            
            // 绘制箭头（实心三角形，位于矩形框左上角，朝左）
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f + 0.5f, markerPos.y - boxSize * 0.5f + 0.5f), markerColor);  // 左边顶点
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f + arrowSize + 0.5f, markerPos.y - boxSize * 0.5f - arrowSize / 2.0f + 0.5f), markerColor);  // 右边上方点
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f + arrowSize + 0.5f, markerPos.y - boxSize * 0.5f + arrowSize / 2.0f + 0.5f), markerColor);  // 右边下方点
            
            flushVertices();
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
            break;
        }
        case CursorMarker::kScale: {
            // 缩放标记：由大框和小实心正方形组成，表示缩放操作
            float largeBoxSize = 12.0f;  // 大框大小
            float smallBoxSize = 6.0f;   // 小框大小
            
            // 绘制大框
            addVertex(glm::vec2(markerPos.x - largeBoxSize * 0.5f + 0.5f, markerPos.y - largeBoxSize * 0.5f + 0.5f), markerColor);  // 左上角
            addVertex(glm::vec2(markerPos.x + largeBoxSize * 0.5f + 0.5f, markerPos.y - largeBoxSize * 0.5f + 0.5f), markerColor);  // 右上角
            addVertex(glm::vec2(markerPos.x + largeBoxSize * 0.5f + 0.5f, markerPos.y + largeBoxSize * 0.5f + 0.5f), markerColor);  // 右下角
            addVertex(glm::vec2(markerPos.x - largeBoxSize * 0.5f + 0.5f, markerPos.y + largeBoxSize * 0.5f + 0.5f), markerColor);  // 左下角
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            
            clearVertices();
            
            // 绘制实心小正方形（位于大框左下角）
            addVertex(glm::vec2(markerPos.x - largeBoxSize * 0.5f + 0.5f, markerPos.y + largeBoxSize * 0.5f - smallBoxSize + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x - largeBoxSize * 0.5f + smallBoxSize + 0.5f, markerPos.y + largeBoxSize * 0.5f - smallBoxSize + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x - largeBoxSize * 0.5f + smallBoxSize + 0.5f, markerPos.y + largeBoxSize * 0.5f + 0.5f), markerColor);
            addVertex(glm::vec2(markerPos.x - largeBoxSize * 0.5f + 0.5f, markerPos.y + largeBoxSize * 0.5f + 0.5f), markerColor);
            
            flushVertices();
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);
            break;
        }
        case CursorMarker::kAddSelect: {
            // 加选标记：绿色加号，表示添加到选择集
            float lineLength = 8.0f;  // 线段长度
            glm::vec4 addColor(0.0f, 1.0f, 0.0f, 1.0f);  // 绿色
            
            // 绘制水平线
            addVertex(glm::vec2(markerPos.x - lineLength * 0.5f, markerPos.y), addColor);
            addVertex(glm::vec2(markerPos.x + lineLength * 0.5f, markerPos.y), addColor);
            // 水平线向上偏移1像素（模拟2像素宽度）
            addVertex(glm::vec2(markerPos.x - lineLength * 0.5f, markerPos.y - 1.0f), addColor);
            addVertex(glm::vec2(markerPos.x + lineLength * 0.5f, markerPos.y - 1.0f), addColor);
            
            // 绘制垂直线
            addVertex(glm::vec2(markerPos.x, markerPos.y - lineLength * 0.5f), addColor);
            addVertex(glm::vec2(markerPos.x, markerPos.y + lineLength * 0.5f), addColor);
            // 垂直线向右偏移1像素（模拟2像素宽度）
            addVertex(glm::vec2(markerPos.x + 1.0f, markerPos.y - lineLength * 0.5f), addColor);
            addVertex(glm::vec2(markerPos.x + 1.0f, markerPos.y + lineLength * 0.5f), addColor);
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
            glBindVertexArray(0);
            break;
        }
        case CursorMarker::kRemoveSelect: {
            // 减选标记：红色减号，表示从选择集移除
            float lineLength = 8.0f;  // 线段长度
            glm::vec4 removeColor(1.0f, 0.0f, 0.0f, 1.0f);  // 红色
            
            // 绘制水平线
            addVertex(glm::vec2(markerPos.x - lineLength * 0.5f, markerPos.y), removeColor);
            addVertex(glm::vec2(markerPos.x + lineLength * 0.5f, markerPos.y), removeColor);
            // 水平线向下偏移1像素（模拟2像素宽度）
            addVertex(glm::vec2(markerPos.x - lineLength * 0.5f, markerPos.y + 1.0f), removeColor);
            addVertex(glm::vec2(markerPos.x + lineLength * 0.5f, markerPos.y + 1.0f), removeColor);
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
            glBindVertexArray(0);
            break;
        }
        case CursorMarker::kCrossingSelect: {
            // 交叉选择标记：由虚线框和实心小正方形组成
            float boxSize = 10.0f;   // 框大小
            float squareSize = 5.0f; // 小正方形大小
            glm::vec4 squareColor(91.0f/255.0f, 201.0f/255.0f, 189.0f/255.0f, 1.0f);  // 绿色 rgb(91,201,189)
            
            // 绘制虚线框（使用纹理坐标）
            glm::vec2 p1(markerPos.x - boxSize * 0.5f + 0.5f, markerPos.y - boxSize * 0.5f + 0.5f);  // 左上角
            glm::vec2 p2(markerPos.x + boxSize * 0.5f + 0.5f, markerPos.y - boxSize * 0.5f + 0.5f);  // 右上角
            glm::vec2 p3(markerPos.x + boxSize * 0.5f + 0.5f, markerPos.y + boxSize * 0.5f + 0.5f);  // 右下角
            glm::vec2 p4(markerPos.x - boxSize * 0.5f + 0.5f, markerPos.y + boxSize * 0.5f + 0.5f);  // 左下角
            
            // 计算累积纹理坐标
            addVertex(p1, markerColor, 0.0f);
            addVertex(p2, markerColor, boxSize);
            addVertex(p2, markerColor, boxSize);
            addVertex(p3, markerColor, boxSize * 2);
            addVertex(p3, markerColor, boxSize * 2);
            addVertex(p4, markerColor, boxSize * 3);
            addVertex(p4, markerColor, boxSize * 3);
            addVertex(p1, markerColor, boxSize * 4);
            
            flushVertices();
            setDashedMode(true, 2.0f);  // 周期2像素
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINES, 0, 8);
            
            clearVertices();
            
            // 绘制实心小正方形（位于框左侧边上）
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f - squareSize + 3.0f + 0.5f, markerPos.y - squareSize * 0.5f + 0.5f), squareColor);
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f + 3.0f + 0.5f, markerPos.y - squareSize * 0.5f + 0.5f), squareColor);
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f + 3.0f + 0.5f, markerPos.y + squareSize * 0.5f + 0.5f), squareColor);
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f - squareSize + 3.0f + 0.5f, markerPos.y + squareSize * 0.5f + 0.5f), squareColor);
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);
            break;
        }
        case CursorMarker::kWindowSelect: {
            // 窗口选择标记：由实线框和中间实心正方形组成
            float boxSize = 10.0f;        // 框大小
            float innerSquareSize = 5.0f; // 中间正方形大小
            glm::vec4 squareColor(56.0f/255.0f, 171.0f/255.0f, 223.0f/255.0f, 1.0f);  // 蓝色 rgb(56,171,223)
            
            // 绘制实线框
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f + 0.5f, markerPos.y - boxSize * 0.5f + 0.5f), markerColor);  // 左上角
            addVertex(glm::vec2(markerPos.x + boxSize * 0.5f + 0.5f, markerPos.y - boxSize * 0.5f + 0.5f), markerColor);  // 右上角
            addVertex(glm::vec2(markerPos.x + boxSize * 0.5f + 0.5f, markerPos.y + boxSize * 0.5f + 0.5f), markerColor);  // 右下角
            addVertex(glm::vec2(markerPos.x - boxSize * 0.5f + 0.5f, markerPos.y + boxSize * 0.5f + 0.5f), markerColor);  // 左下角
            
            flushVertices();
            setDashedMode(false);
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            
            clearVertices();
            
            // 绘制实心小正方形（位于框中间）
            addVertex(glm::vec2(markerPos.x - innerSquareSize * 0.5f + 0.5f, markerPos.y - innerSquareSize * 0.5f + 0.5f), squareColor);
            addVertex(glm::vec2(markerPos.x + innerSquareSize * 0.5f + 0.5f, markerPos.y - innerSquareSize * 0.5f + 0.5f), squareColor);
            addVertex(glm::vec2(markerPos.x + innerSquareSize * 0.5f + 0.5f, markerPos.y + innerSquareSize * 0.5f + 0.5f), squareColor);
            addVertex(glm::vec2(markerPos.x - innerSquareSize * 0.5f + 0.5f, markerPos.y + innerSquareSize * 0.5f + 0.5f), squareColor);
            
            flushVertices();
            
            glBindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);
            break;
        }
        default:
            break;
    }
    
    glEnable(GL_DEPTH_TEST);
}

// 绘制选择区域
void CanvasRenderer::drawSelection() {
    AAGuard aaGuard(false);
    
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    
    if (!interactionData.isSelectionActive) {
        return;
    }
    
    auto& transformManager = DocManager::getCurrentDocument().getTransformManager();
    
    glm::vec4 fillColor;
    glm::vec4 lineColor(1.0f, 1.0f, 1.0f, 1.0f);
    
    // 交叉选择模式使用虚线，窗口选择模式使用实线
    bool useDashedLine = (interactionData.selectionMode == SelectionMode::kCrossing ||
                        interactionData.selectionMode == SelectionMode::kCrossingLasso ||
                        interactionData.selectionMode == SelectionMode::kCrossingPolygon);
    
    // 禁用深度测试，启用混合
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 使用shader绘制
    m_canvasShader.use();
    glUniformMatrix4fv(m_mvpLocation, 1, GL_FALSE, &m_projection[0][0]);
    
    // 绑定虚线纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, m_dashTexture);
    
    // 将世界坐标转换为屏幕坐标（框选模式除外，框选模式在switch内单独处理）
    std::vector<glm::vec2> screenPoints;
    if (interactionData.selectionMode != SelectionMode::kWindow &&
        interactionData.selectionMode != SelectionMode::kCrossing) {
        for (const auto& point : interactionData.selectionPointsWorld) {
            screenPoints.push_back(transformManager.worldToScreen(point));
        }
        // 添加预览点（鼠标当前位置）
        screenPoints.push_back(transformManager.worldToScreen(interactionData.selectionPreviewPointWorld));
        // 多边形选择下只有起始点和预览点2个点时无法绘制填充，所以此时需要临时添加一个点以支持绘制预览线段
        if (screenPoints.size() == 2) {
            screenPoints.push_back(screenPoints.back());
        }
    }
    
    switch (interactionData.selectionMode) {
        case SelectionMode::kWindow:
        case SelectionMode::kCrossing: {
            // 框选模式：由两个对角点定义矩形
            std::vector<glm::vec2> boxPoints;
            boxPoints.push_back(transformManager.worldToScreen(interactionData.selectionInitialPointWorld));
            boxPoints.push_back(transformManager.worldToScreen(interactionData.selectionPreviewPointWorld));
            
            fillColor = (interactionData.selectionMode == SelectionMode::kCrossing) 
                ? s_crossingSelectionColor
                : s_windowSelectionColor;
            drawWindowSelectionVertices(boxPoints, fillColor, lineColor, useDashedLine);
            break;
        }
        case SelectionMode::kWindowLasso:
        case SelectionMode::kCrossingLasso: {
            // 套索选择模式：自由绘制的多边形
            fillColor = (interactionData.selectionMode == SelectionMode::kCrossingLasso)
                ? s_crossingSelectionColor
                : s_windowSelectionColor;
            drawLassoSelectionVertices(screenPoints, fillColor, lineColor, useDashedLine);
            break;
        }
        case SelectionMode::kWindowPolygon:
        case SelectionMode::kCrossingPolygon: {
            // 多边形选择模式：点击绘制的多边形
            fillColor = (interactionData.selectionMode == SelectionMode::kCrossingPolygon)
                ? s_crossingSelectionColor
                : s_windowSelectionColor;
            drawPolygonSelectionVertices(screenPoints, fillColor, lineColor, useDashedLine);
            break;
        }
        case SelectionMode::kFence: {
            // 栏选模式：折线（无填充）
            drawFenceSelectionVertices(screenPoints, lineColor, true);
            break;
        }
        default:
            break;
    }
    
    // 恢复深度测试
    glEnable(GL_DEPTH_TEST);
}

// 绘制窗口选择顶点（矩形框选）
void CanvasRenderer::drawWindowSelectionVertices(const std::vector<glm::vec2>& points, 
                                           const glm::vec4& fillColor, 
                                           const glm::vec4& lineColor,
                                           bool useDashedLine) {
    if (points.size() < 2) {
        return;
    }
    
    // 计算矩形边界
    float left = std::min(points[0].x, points[1].x);
    float right = std::max(points[0].x, points[1].x);
    float top = std::min(points[0].y, points[1].y);
    float bottom = std::max(points[0].y, points[1].y);
    
    clearVertices();
    
    // 绘制填充（两个三角形组成矩形）
    addVertex(glm::vec2(left + 0.5f, top + 0.5f), fillColor);
    addVertex(glm::vec2(right + 0.5f, top + 0.5f), fillColor);
    addVertex(glm::vec2(right + 0.5f, bottom + 0.5f), fillColor);
    
    addVertex(glm::vec2(left + 0.5f, top + 0.5f), fillColor);
    addVertex(glm::vec2(right + 0.5f, bottom + 0.5f), fillColor);
    addVertex(glm::vec2(left + 0.5f, bottom + 0.5f), fillColor);
    
    flushVertices();
    setDashedMode(false);
    
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
    
    clearVertices();
    
    // 绘制边框（使用累积纹理坐标实现连续虚线）
    float width = right - left;
    float height = bottom - top;
    float perimeter = 2 * (width + height);  // 周长
    
    // 顶边：纹理坐标从0到width
    addVertex(glm::vec2(left + 0.5f, top + 0.5f), lineColor, 0.0f);
    addVertex(glm::vec2(right + 0.5f, top + 0.5f), lineColor, width);
    
    // 右边：纹理坐标从width到width+height
    addVertex(glm::vec2(right + 0.5f, top + 0.5f), lineColor, width);
    addVertex(glm::vec2(right + 0.5f, bottom + 0.5f), lineColor, width + height);
    
    // 底边：纹理坐标从width+height到2*width+height
    addVertex(glm::vec2(right + 0.5f, bottom + 0.5f), lineColor, width + height);
    addVertex(glm::vec2(left + 0.5f, bottom + 0.5f), lineColor, 2 * width + height);
    
    // 左边：纹理坐标从2*width+height到perimeter
    addVertex(glm::vec2(left + 0.5f, bottom + 0.5f), lineColor, 2 * width + height);
    addVertex(glm::vec2(left + 0.5f, top + 0.5f), lineColor, perimeter);
    
    flushVertices();
    setDashedMode(useDashedLine, 8.0f);  // 虚线周期8像素
    
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);
}

// 绘制套索选择顶点（自由绘制的多边形）
void CanvasRenderer::drawLassoSelectionVertices(const std::vector<glm::vec2>& points,
                                           const glm::vec4& fillColor,
                                           const glm::vec4& lineColor,
                                           bool useDashedLine) {
    if (points.size() < 3) {
        return;
    }
    
    // 使用模板缓冲绘制填充（支持凹多边形）
    drawPolygonFill(points, fillColor);
    
    // 绘制边框（使用累积纹理坐标实现连续虚线）
    clearVertices();
    
    float texCoord = 0.0f;
    for (size_t i = 0; i < points.size(); ++i) {
        size_t next = (i + 1) % points.size();
        float len = glm::length(points[next] - points[i]);  // 线段长度
        addVertex(points[i], lineColor, texCoord);
        addVertex(points[next], lineColor, texCoord + len);
        texCoord += len;  // 累积纹理坐标
    }
    
    flushVertices();
    setDashedMode(useDashedLine, 8.0f);
    
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);
}

// 绘制多边形选择顶点（点击绘制的多边形）
void CanvasRenderer::drawPolygonSelectionVertices(const std::vector<glm::vec2>& points,
                                              const glm::vec4& fillColor,
                                              const glm::vec4& lineColor,
                                              bool useDashedLine) {
    if (points.size() < 3) {
        return;
    }
    
    // 使用模板缓冲绘制填充（支持凹多边形）
    drawPolygonFill(points, fillColor);
    
    // 绘制边框（使用累积纹理坐标实现连续虚线）
    clearVertices();
    
    float texCoord = 0.0f;
    for (size_t i = 0; i < points.size(); ++i) {
        size_t next = (i + 1) % points.size();
        float len = glm::length(points[next] - points[i]);  // 线段长度
        addVertex(points[i], lineColor, texCoord);
        addVertex(points[next], lineColor, texCoord + len);
        texCoord += len;  // 累积纹理坐标
    }

    flushVertices();
    setDashedMode(useDashedLine, 8.0f);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);
}

// 绘制栏选顶点（折线，无填充）
void CanvasRenderer::drawFenceSelectionVertices(const std::vector<glm::vec2>& points,
                                          const glm::vec4& lineColor,
                                          bool useDashedLine) {
    if (points.size() < 2) {
        return;
    }
    
    clearVertices();
    
    // 绘制折线（使用累积纹理坐标实现连续虚线）
    float texCoord = 0.0f;
    for (size_t i = 0; i < points.size() - 1; ++i) {
        float len = glm::length(points[i + 1] - points[i]);  // 线段长度
        addVertex(points[i], lineColor, texCoord);
        addVertex(points[i + 1], lineColor, texCoord + len);
        texCoord += len;  // 累积纹理坐标
    }
    
    flushVertices();
    setDashedMode(useDashedLine, 8.0f);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);
}

} // namespace tch
