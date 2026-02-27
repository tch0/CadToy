#include "render/Renderer.h"
#include "file/FileManager.h"
#include "Layer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "sys/Global.h"
#include "debug/Logger.h"
#include "utils/LocalizationManager.h"
#include "command/CommandParser.h"
#include "input/InputHandler.h"
#include <algorithm>
#include <array>

namespace tch {

// 静态成员初始化
bool Renderer::s_initialized = false;
GLFWwindow* Renderer::s_window = nullptr;
float Renderer::s_crossCursorSize = 50.0f;
float Renderer::s_pickBoxSize = 5.0f;      // 拾取框大小，默认值为5

// 栅格和坐标轴相关初始化
bool Renderer::s_showGrid = true;         // 默认显示栅格
bool Renderer::s_showAxes = true;         // 默认显示XY轴
float Renderer::s_mainGridColor[3] = {54.0f/255.0f, 61.0f/255.0f, 78.0f/255.0f}; // 主栅格颜色 RGB: 54,61,78
float Renderer::s_subGridColor[3] = {38.0f/255.0f, 45.0f/255.0f, 55.0f/255.0f};  // 子栅格颜色 RGB: 38,45,55
float Renderer::s_xAxisColor[3] = {97.0f/255.0f, 37.0f/255.0f, 39.0f/255.0f};    // X轴颜色 RGB: 97,37,39
float Renderer::s_yAxisColor[3] = {34.0f/255.0f, 89.0f/255.0f, 41.0f/255.0f};    // Y轴颜色 RGB: 34,89,41
float Renderer::s_originX = 0.0f;          // 坐标原点X位置
float Renderer::s_originY = 0.0f;          // 坐标原点Y位置
glm::dvec3 Renderer::s_cursorPosition = glm::dvec3(0.0, 0.0, 0.0); // 当前光标位置（以窗口中央为原点）

// UI组件高度
float Renderer::s_menuBarHeight = 0.0f;              // 菜单栏高度
float Renderer::s_fileBarHeight = 30.0f;              // 文件栏高度
float Renderer::s_statusBarHeight = 35.0f;            // 状态栏高度

// 命令历史滚动控制
bool Renderer::s_bScrollCommandHistoryToBottom = false; // 是否应该将命令历史滚动到底部
// 命令输入框焦点控制
bool Renderer::s_bShouldFocusOnCommandInput = false; // 是否应该将焦点设置到命令输入框
// 命令输入缓冲区是否被修改，通过非命令输入栏的字符输入或者退格
bool Renderer::s_bCommandBufferModified = false;
// 是否应该执行命令
bool Renderer::s_bShouldExecuteCommand = false;
// 是否应该取消命令执行
bool Renderer::s_bShouldCancelCommand = false;
// 是否需要清除命令输入缓冲区
bool Renderer::s_bNeedClearCommandBuffer = false;

// 命令栏相关
static std::vector<std::string> s_commandHistory; // 命令执行历史
static bool s_commandBarVisible = true; // 命令栏是否可见
static float s_commandBarHeight = 150.0f; // 命令栏高度
static std::array<char, 256> s_cmdBuffer{}; // 命令输入缓冲区


// 选项对话框相关
static bool s_optionsDialogVisible = false; // 选项对话框是否可见

// 属性栏相关
static bool s_propertyBarVisible = true;    // 属性栏是否可见
static float s_propertyBarWidth = 250.0f;   // 属性栏宽度

// 示例与调试窗口相关
static bool s_demoWindowVisible = false;     // Demo窗口是否可见
static bool s_metricsWindowVisible = false;  // Metrics/Debugger窗口是否可见

// 文件栏相关 - 使用FileManager类管理

// 逻辑视口初始化
LogicalViewport Renderer::s_logicalViewport;

// 初始化渲染器
void Renderer::initialize(GLFWwindow* window) {
    s_window = window;
    s_initialized = true;
    
    // 设置默认背景颜色为RGB: 33,40,48
    setBackgroundColor(33.0f/255.0f, 40.0f/255.0f, 48.0f/255.0f, 1.0f);
    
    // 获取窗口尺寸并设置视口
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    setViewport(width, height);
    
    // 初始化逻辑视口
    s_logicalViewport.initialize(width, height);
    
    // 设置坐标原点为窗口中心
    s_originX = width / 2.0f;
    s_originY = height / 2.0f;
    
    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 初始化ImGui
    initializeImGui();
    
    // 初始化本地化管理器
    LocalizationManager::getInstance().initialize();
    
    // 初始化文件管理器
    FileManager::initialize();
}

// 清理渲染器
void Renderer::cleanup() {
    // 清理ImGui
    cleanupImGui();
    
    s_initialized = false;
    s_window = nullptr;
}

// 更新可绘制区域
void Renderer::updateDrawableArea() {
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    
    // 状态栏高度
    float statusBarHeight = s_statusBarHeight;
    
    // 计算可绘制区域的边界
    // 左侧：0
    // 顶部：菜单栏高度 + 文件栏高度
    // 右侧：窗口宽度 - (属性栏宽度 if 可见)
    // 底部：窗口高度 - 状态栏高度 - (命令栏高度 if 可见)
    int left = 0;
    int top = static_cast<int>(s_menuBarHeight + s_fileBarHeight);
    int right = width - (s_propertyBarVisible ? static_cast<int>(s_propertyBarWidth) : 0);
    int bottom = height - static_cast<int>(statusBarHeight) - (s_commandBarVisible ? static_cast<int>(s_commandBarHeight) : 0);
    
    // 确保可绘制区域有效
    // 确保顶部边界小于底部边界
    if (top >= bottom) {
        // 如果顶部边界大于或等于底部边界，调整底部边界为顶部边界 + 1
        bottom = top + 1;
    }
    
    // 确保左右边界有效
    if (left >= right) {
        // 如果左边界大于或等于右边界，调整右边界为左边界 + 1
        right = left + 1;
    }
    
    // 确保边界在窗口范围内
    top = std::max(0, std::min(top, height - 1));
    bottom = std::max(1, std::min(bottom, height));
    left = std::max(0, std::min(left, width - 1));
    right = std::max(1, std::min(right, width));
    
    // 更新逻辑视口的可绘制区域
    s_logicalViewport.setDrawableArea(left, top, right, bottom);
}

// 开始渲染
void Renderer::beginRender() {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 清除颜色缓冲
    glClear(GL_COLOR_BUFFER_BIT);
    
    // 开始ImGui渲染
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // 绘制菜单栏
    drawMenuBar();
    
    // 绘制文件栏
    drawFileBar();
    
    // 更新可绘制区域
    updateDrawableArea();
    
    // 使用ImGui的原生API来控制光标显示
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        // 当ImGui不想要捕获鼠标时，设置鼠标光标为None
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    }
}

// 结束渲染
void Renderer::endRender() {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 绘制选项对话框
    drawOptionsDialog();
    
    // 绘制Demo窗口
    if (s_demoWindowVisible) {
        ImGui::ShowDemoWindow(&s_demoWindowVisible);
    }
    
    // 绘制Metrics窗口
    if (s_metricsWindowVisible) {
        ImGui::ShowMetricsWindow(&s_metricsWindowVisible);
    }
    
    // 渲染ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // 交换缓冲区
    glfwSwapBuffers(s_window);
}

// 设置背景颜色
void Renderer::setBackgroundColor(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
}

// 设置视口
void Renderer::setViewport(int width, int height) {
    glViewport(0, 0, width, height);
}

// 更新视口大小（窗口大小变化时调用）
void Renderer::updateViewport(int width, int height) {
    // 直接调用setViewport方法更新视口大小
    setViewport(width, height);
    
    // 更新逻辑视口的窗口大小
    s_logicalViewport.setWindowSize(width, height);
}

// 绘制所有图形
void Renderer::drawAll() {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 绘制栅格
    if (s_showGrid) {
        drawGrid();
    }
    
    // 绘制XY轴
    if (s_showAxes) {
        drawAxes();
    }
    
    // 绘制所有图层
    LayerManager::getInstance().draw();
}

// 获取渲染器状态
bool Renderer::isInitialized() {
    return s_initialized;
}

// 绘制光标
void Renderer::drawCursor(const glm::vec2& position) {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 检查鼠标位置是否在可绘制区域内
    bool isInDrawableArea = s_logicalViewport.isPointInDrawableArea(position);
    
    // 只有当鼠标在可绘制区域内时，才更新光标位置
    if (isInDrawableArea) {
        // 更新当前光标位置（使用逻辑视口转换）
        glm::dvec3 logicPos = s_logicalViewport.screenToLogic(position);
        s_cursorPosition = logicPos;
    }
    
    // 保存当前矩阵状态
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    // 设置正交投影，Y轴朝下（标准鼠标坐标系）
    // 注意：glOrtho的参数顺序是left, right, bottom, top, near, far
    // 在标准鼠标坐标系中，Y轴向下，所以底部在屏幕顶部，顶部在屏幕底部
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    glOrtho(0, width, height, 0, -1, 1);
    
    // 切换到模型视图矩阵
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // 禁用深度测试
    glDisable(GL_DEPTH_TEST);
    
    // 计算光标在屏幕上的位置
    glm::vec2 cursorScreenPos;
    if (isInDrawableArea) {
        // 如果鼠标在可绘制区域内，使用鼠标位置
        cursorScreenPos = position;
    } else {
        // 如果鼠标不在可绘制区域内，使用逻辑光标位置转换到屏幕坐标
        cursorScreenPos = s_logicalViewport.logicToScreen(s_cursorPosition);
    }
    
    // 绘制拾取框
    glBegin(GL_LINE_LOOP);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
    glVertex2f(cursorScreenPos.x - s_pickBoxSize, cursorScreenPos.y - s_pickBoxSize);
    glVertex2f(cursorScreenPos.x + s_pickBoxSize, cursorScreenPos.y - s_pickBoxSize);
    glVertex2f(cursorScreenPos.x + s_pickBoxSize, cursorScreenPos.y + s_pickBoxSize);
    glVertex2f(cursorScreenPos.x - s_pickBoxSize, cursorScreenPos.y + s_pickBoxSize);
    glEnd();
    
    // 只有当十字光标尺寸大于0且大于选择框尺寸时，才绘制光标的四条线
    if (s_crossCursorSize > 0 && s_crossCursorSize > s_pickBoxSize) {
        // 计算线段长度：十字光标大小减去选择框大小
        float lineLength = s_crossCursorSize - s_pickBoxSize;
        
        // 绘制从正方形四条边中点向外延伸的光标线条
        glBegin(GL_LINES);
        glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
        
        // 上边中点向上延伸
        glVertex2f(cursorScreenPos.x, cursorScreenPos.y - s_pickBoxSize);
        glVertex2f(cursorScreenPos.x, cursorScreenPos.y - s_pickBoxSize - lineLength);
        
        // 下边中点向下延伸
        glVertex2f(cursorScreenPos.x, cursorScreenPos.y + s_pickBoxSize);
        glVertex2f(cursorScreenPos.x, cursorScreenPos.y + s_pickBoxSize + lineLength);
        
        // 左边中点向左延伸
        glVertex2f(cursorScreenPos.x - s_pickBoxSize, cursorScreenPos.y);
        glVertex2f(cursorScreenPos.x - s_pickBoxSize - lineLength, cursorScreenPos.y);
        
        // 右边中点向右延伸
        glVertex2f(cursorScreenPos.x + s_pickBoxSize, cursorScreenPos.y);
        glVertex2f(cursorScreenPos.x + s_pickBoxSize + lineLength, cursorScreenPos.y);
        glEnd();
    }
    
    // 恢复矩阵状态
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // 重新启用深度测试
    glEnable(GL_DEPTH_TEST);
}

// 设置十字光标大小
void Renderer::setCrossCursorSize(float size) {
    s_crossCursorSize = size;
}

// 获取十字光标大小
float Renderer::getCrossCursorSize() {
    return s_crossCursorSize;
}

// 绘制栅格
void Renderer::drawGrid() {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 保存当前矩阵状态
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    // 设置正交投影，Y轴朝下（标准鼠标坐标系）
    // 注意：glOrtho的参数顺序是left, right, bottom, top, near, far
    // 在标准鼠标坐标系中，Y轴向下，所以底部在屏幕顶部，顶部在屏幕底部
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    glOrtho(0, width, height, 0, -1, 1);
    
    // 切换到模型视图矩阵
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // 禁用深度测试
    glDisable(GL_DEPTH_TEST);
    
    // 获取逻辑视口边界
    glm::dvec2 logicMin = s_logicalViewport.getLogicMin();
    glm::dvec2 logicMax = s_logicalViewport.getLogicMax();
    
    // 计算逻辑视口的宽度和高度
    double logicWidth = logicMax.x - logicMin.x;
    double logicHeight = logicMax.y - logicMin.y;
    
    // 获取可绘制区域大小
    glm::ivec2 drawableSize = s_logicalViewport.getDrawableAreaSize();
    int drawableWidth = drawableSize.x;
    int drawableHeight = drawableSize.y;
    
    // 基础栅格间距（逻辑坐标）
    const double baseGridSize = 10.0;
    
    // 计算当前有效的栅格间距（考虑可绘制区域大小）
    double currentEffectiveSize = baseGridSize;
    
    // 确定栅格级别
    // 目标是保持栅格在屏幕上的大小在合理范围内（50-250像素）
    double mainGridSize, subGridSize;
    
    // 计算当前栅格在屏幕上的大小（水平和垂直方向）
    double gridScreenSizeX = (baseGridSize / logicWidth) * drawableWidth;
    double gridScreenSizeY = (baseGridSize / logicHeight) * drawableHeight;
    
    // 取较小的值，确保栅格单元格在屏幕上保持正方形
    double gridScreenSize = std::min(gridScreenSizeX, gridScreenSizeY);
    
    if (gridScreenSize < 50.0) {
        // 当前栅格太小，需要增加栅格级别
        int level = 0;
        double testSize = baseGridSize;
        double testScreenSize = gridScreenSize;
        while (testScreenSize < 50.0) {
            testSize *= 5.0;
            testScreenSize *= 5.0;
            level++;
        }
        
        // 设置新的栅格大小
        mainGridSize = testSize;
        subGridSize = mainGridSize / 5.0;
    } else if (gridScreenSize > 250.0) {
        // 当前栅格太大，需要减少栅格级别
        int level = 0;
        double testSize = baseGridSize;
        double testScreenSize = gridScreenSize;
        while (testScreenSize > 250.0) {
            testSize /= 5.0;
            testScreenSize /= 5.0;
            level++;
        }
        
        // 设置新的栅格大小
        mainGridSize = testSize;
        subGridSize = mainGridSize / 5.0;
    } else {
        // 栅格大小在合理范围内
        mainGridSize = baseGridSize;
        subGridSize = mainGridSize / 5.0;
    }
    
    // 计算逻辑原点
    double originX = 0.0;
    double originY = 0.0;
    
    // 绘制子栅格
    glBegin(GL_LINES);
    glColor3fv(s_subGridColor);
    
    // 计算起始位置，确保栅格线与原点对齐
    double startX = floor(logicMin.x / subGridSize) * subGridSize;
    double startY = floor(logicMin.y / subGridSize) * subGridSize;
    
    // 绘制垂直线
    for (double x = startX; x <= logicMax.x; x += subGridSize) {
        glm::vec2 screenPos = s_logicalViewport.logicToScreen(glm::dvec3(x, logicMin.y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
        screenPos = s_logicalViewport.logicToScreen(glm::dvec3(x, logicMax.y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
    }
    
    // 绘制水平线
    for (double y = startY; y <= logicMax.y; y += subGridSize) {
        glm::vec2 screenPos = s_logicalViewport.logicToScreen(glm::dvec3(logicMin.x, y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
        screenPos = s_logicalViewport.logicToScreen(glm::dvec3(logicMax.x, y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
    }
    glEnd();
    
    // 绘制主栅格
    glBegin(GL_LINES);
    glColor3fv(s_mainGridColor);
    
    // 计算起始位置，确保栅格线与原点对齐
    double mainStartX = floor(logicMin.x / mainGridSize) * mainGridSize;
    double mainStartY = floor(logicMin.y / mainGridSize) * mainGridSize;
    
    // 绘制垂直线
    for (double x = mainStartX; x <= logicMax.x; x += mainGridSize) {
        glm::vec2 screenPos = s_logicalViewport.logicToScreen(glm::dvec3(x, logicMin.y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
        screenPos = s_logicalViewport.logicToScreen(glm::dvec3(x, logicMax.y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
    }
    
    // 绘制水平线
    for (double y = mainStartY; y <= logicMax.y; y += mainGridSize) {
        glm::vec2 screenPos = s_logicalViewport.logicToScreen(glm::dvec3(logicMin.x, y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
        screenPos = s_logicalViewport.logicToScreen(glm::dvec3(logicMax.x, y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
    }
    glEnd();
    
    // 恢复矩阵状态
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // 重新启用深度测试
    glEnable(GL_DEPTH_TEST);
}

// 绘制XY轴
void Renderer::drawAxes() {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 保存当前矩阵状态
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    // 设置正交投影，Y轴朝下（标准鼠标坐标系）
    // 注意：glOrtho的参数顺序是left, right, bottom, top, near, far
    // 在标准鼠标坐标系中，Y轴向下，所以底部在屏幕顶部，顶部在屏幕底部
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    glOrtho(0, width, height, 0, -1, 1);
    
    // 切换到模型视图矩阵
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // 禁用深度测试
    glDisable(GL_DEPTH_TEST);
    
    // 获取可绘制区域边界
    int drawableLeft, drawableTop, drawableRight, drawableBottom;
    s_logicalViewport.getDrawableArea(drawableLeft, drawableTop, drawableRight, drawableBottom);
    
    // 计算逻辑原点在屏幕上的位置
    glm::vec2 originScreenPos = s_logicalViewport.logicToScreen(glm::dvec3(0.0, 0.0, 0.0));
    float originScreenX = originScreenPos.x;
    float originScreenY = originScreenPos.y;
    
    // 绘制X轴（正半轴）
    glBegin(GL_LINES);
    glColor3fv(s_xAxisColor);
    
    // 只绘制在可绘制区域范围内的部分
    if (originScreenX >= drawableLeft && originScreenX <= drawableRight && originScreenY >= drawableTop && originScreenY <= drawableBottom) {
        glVertex2f(originScreenX, originScreenY);
        glVertex2f(drawableRight, originScreenY);
    }
    glEnd();
    
    // 绘制Y轴（正半轴，朝上）
    glBegin(GL_LINES);
    glColor3fv(s_yAxisColor);
    
    // 只绘制在可绘制区域范围内的部分
    if (originScreenX >= drawableLeft && originScreenX <= drawableRight && originScreenY >= drawableTop && originScreenY <= drawableBottom) {
        glVertex2f(originScreenX, originScreenY);
        glVertex2f(originScreenX, drawableTop);
    }
    glEnd();
    
    // 恢复矩阵状态
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // 重新启用深度测试
    glEnable(GL_DEPTH_TEST);
}

// 栅格和坐标轴控制方法
void Renderer::setShowGrid(bool show) {
    s_showGrid = show;
}

void Renderer::setShowAxes(bool show) {
    s_showAxes = show;
}



void Renderer::setOrigin(float x, float y) {
    s_originX = x;
    s_originY = y;
}

glm::vec2 Renderer::getOrigin() {
    return glm::vec2(s_originX, s_originY);
}

void Renderer::zoomIn() {
    // 使用逻辑视口进行缩放
    s_logicalViewport.zoomIn();
}

void Renderer::zoomOut() {
    // 使用逻辑视口进行缩放
    s_logicalViewport.zoomOut();
}

void Renderer::zoomIn(const glm::vec2& mousePos) {
    // 使用逻辑视口进行缩放，以鼠标位置为中心
    s_logicalViewport.zoomIn(mousePos);
}

void Renderer::zoomOut(const glm::vec2& mousePos) {
    // 使用逻辑视口进行缩放，以鼠标位置为中心
    s_logicalViewport.zoomOut(mousePos);
}

// 平移功能
void Renderer::pan(const glm::dvec2& deltaLogic) {
    // 使用逻辑视口进行平移
    s_logicalViewport.pan(deltaLogic);
}

// 初始化ImGui
void Renderer::initializeImGui() {
    // 设置ImGui上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // 配置ImGui
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    
    // 1. 加载Consolas字体用于英文显示
    std::filesystem::path consolasPath = g_pathCwd / "fonts" / "consolas.ttf";
    std::string consolasPathStr = consolasPath.string();
    
    LOG_INFO("Attempting to load English font from: {}", consolasPathStr);
    
    // 尝试加载Consolas字体
    ImFont* consolasFont = io.Fonts->AddFontFromFileTTF(consolasPathStr.c_str(), 18.0f);
    
    if (!consolasFont) {
        LOG_WARNING("Failed to load Consolas font: {}", consolasPathStr);
        LOG_INFO("Using default ImGui font instead");
        io.Fonts->AddFontDefault();
    }
    
    // 2. 配置中文字体加载选项
    ImFontConfig config;
    config.MergeMode = true;
    
    // 3. 加载微软雅黑字体用于中文显示
    std::filesystem::path msyhPath = g_pathCwd / "fonts" / "MSYH.TTC";
    std::string msyhPathStr = msyhPath.string();
    
    LOG_INFO("Attempting to load Chinese font from: {}", msyhPathStr);
    
    // 尝试加载微软雅黑字体，加载完整中文字符集以确保所有汉字都能显示，中文字号略大看起来才和英文匹配
    ImFont* msyhFont = io.Fonts->AddFontFromFileTTF(msyhPathStr.c_str(), 22.0f, &config, io.Fonts->GetGlyphRangesChineseFull());
    
    if (!msyhFont) {
        LOG_WARNING("Failed to load Microsoft YaHei font: {}", msyhPathStr);
        LOG_INFO("Chinese characters may not display correctly");
    }
    
    // 4. 构建字体图集
    io.Fonts->Build();
    
    LOG_INFO("Font loading completed");
    
    // 设置ImGui样式
    ImGui::StyleColorsDark();
    
    // 初始化ImGui GLFW后端
    ImGui_ImplGlfw_InitForOpenGL(s_window, true);
    
    // 初始化ImGui OpenGL3后端
    const char* glsl_version = "#version 330";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

// 清理ImGui
void Renderer::cleanupImGui() {
    // 清理ImGui OpenGL3后端
    ImGui_ImplOpenGL3_Shutdown();
    
    // 清理ImGui GLFW后端
    ImGui_ImplGlfw_Shutdown();
    
    // 销毁ImGui上下文
    ImGui::DestroyContext();
}

// 绘制状态栏
void Renderer::drawStatusBar(const glm::vec2& cursorPos) {
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    
    // 计算状态栏位置和大小
    float statusBarHeight = s_statusBarHeight;
    ImVec2 statusBarPos(0, height - statusBarHeight);
    ImVec2 statusBarSize(width, statusBarHeight);
    
    // 绘制状态栏
    ImGui::SetNextWindowPos(statusBarPos);
    ImGui::SetNextWindowSize(statusBarSize);
    ImGui::SetNextWindowBgAlpha(0.9f);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    if (ImGui::Begin("StatusBar", nullptr, flags)) {
        // 直接使用已保存的光标位置（以窗口中央为原点的坐标系）
        ImGui::Text("%.4f, %.4f, %.4f", s_cursorPosition.x, s_cursorPosition.y, s_cursorPosition.z);
        ImGui::End();
    }
}

// 绘制选项对话框
void Renderer::drawOptionsDialog() {
    if (s_optionsDialogVisible) {
        auto& loc = LocalizationManager::getInstance();
        // 使用BeginPopupModal创建真正的模态对话框
        ImGui::OpenPopup("Options");
        
        // 设置对话框位置为屏幕中央
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        
        // 不强制设置对话框大小，让ImGui从ini文件读取
        // ImGui::SetNextWindowSize(ImVec2(400, 300));
        
        // 使用模态对话框标志，允许调整大小
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
        
        if (ImGui::BeginPopupModal("Options", &s_optionsDialogVisible, flags)) {
            // 对话框标题
            ImGui::Text(loc.get("optionsDialog.title").c_str());
            ImGui::Separator();
            
            // 对话框内容
            static bool showGrid = s_showGrid;
            static bool showAxes = s_showAxes;
            static int crossCursorSize = static_cast<int>(s_crossCursorSize);
            static int pickBoxSizeInt = static_cast<int>(s_pickBoxSize);
            
            // 创建选项卡栏
            if (ImGui::BeginTabBar("OptionsTabs")) {
                // 第一个选项卡：显示
                if (ImGui::BeginTabItem(loc.get("optionsDialog.tab.display").c_str())) {
                    // Grid & Axes 标题
                    ImGui::Text(loc.get("optionsDialog.gridAxes").c_str());
                    ImGui::Spacing();
                    
                    // 选项
                    ImGui::Checkbox(loc.get("optionsDialog.showGrid").c_str(), &showGrid);
                    ImGui::Checkbox(loc.get("optionsDialog.showAxes").c_str(), &showAxes);
                    
                    // 在十字光标大小设置前添加分隔线，与前面的栅格坐标轴设置分开
                    ImGui::Separator();
                    
                    // 十字光标大小
                    ImGui::Spacing();
                    ImGui::Text(loc.get("optionsDialog.crossCursorSize").c_str());
                    ImGui::Spacing();
                    
                    // 滑块控件，范围5-100，使用整数，长度设为500
                    ImGui::PushItemWidth(500); // 设置滑块宽度为500
                    ImGui::SliderInt("##CrossCursorSize", &crossCursorSize, 5, 100, "%d");
                    ImGui::PopItemWidth();
                    
                    ImGui::EndTabItem();
                }
                
                // 第二个选项卡：选择集
                if (ImGui::BeginTabItem(loc.get("optionsDialog.tab.selection").c_str())) {
                    // 选择框大小
                    ImGui::Spacing();
                    ImGui::Text(loc.get("optionsDialog.pickBoxSize").c_str());
                    ImGui::Spacing();
                    
                    // 首先绘制预览框
                    ImGui::BeginGroup();
                    
                    // 创建一个更大的预览区域，确保最大选择框也能完全显示
                    ImVec2 previewSize(120, 120);
                    ImGui::BeginChild("Preview", previewSize, true);
                    
                    // 计算预览框的位置和大小
                    ImVec2 previewPos = ImGui::GetCursorScreenPos();
                    ImVec2 center(previewSize.x / 2 - 8, previewSize.y / 2 - 8);
                    
                    // 绘制预览框（正方形）
                    ImGui::GetWindowDrawList()->AddRect(
                        ImVec2(previewPos.x + center.x - pickBoxSizeInt, previewPos.y + center.y - pickBoxSizeInt),
                        ImVec2(previewPos.x + center.x + pickBoxSizeInt, previewPos.y + center.y + pickBoxSizeInt),
                        IM_COL32(255, 255, 255, 255),
                        0.0f,
                        0,
                        1.0f
                    );
                    
                    ImGui::EndChild();
                    ImGui::EndGroup();
                    
                    // 然后在左侧绘制滑块，对齐到预览框底部
                    ImGui::SameLine(0.0f, 20.0f); // 0.0f表示左对齐，20.0f是间距
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 90); // 调整垂直位置，使滑块与预览框底部对齐
                    
                    ImGui::PushItemWidth(250); // 设置滑块宽度为250
                    ImGui::SliderInt("##PickBoxSize", &pickBoxSizeInt, 0, 50, "%d");
                    ImGui::PopItemWidth();
                    
                    ImGui::EndTabItem();
                }
                
                // 第三个选项卡：语言
                if (ImGui::BeginTabItem(loc.get("optionsDialog.tab.language").c_str())) {
                    // 语言选择
                    ImGui::Spacing();
                    ImGui::Text(loc.get("optionsDialog.language").c_str());
                    ImGui::Spacing();
                    
                    // 获取当前语言
                    std::string currentLanguage = loc.getCurrentLanguage();
                    
                    // 语言选择下拉框，显示固定的语言选项
                    if (ImGui::BeginCombo("##LanguageSelect", (currentLanguage == "en" ? "English" : "中文"))) {
                        bool isEnglishSelected = (currentLanguage == "en");
                        if (ImGui::Selectable("English", isEnglishSelected)) {
                            loc.setLanguage("en");
                        }
                        if (isEnglishSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                        
                        bool isChineseSelected = (currentLanguage == "zh");
                        if (ImGui::Selectable("中文", isChineseSelected)) {
                            loc.setLanguage("zh");
                        }
                        if (isChineseSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
            
            // 垂直填充空间，将按钮推到底部
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();
            
            // 底部分隔线
            ImGui::Separator();
            
            // 右对齐按钮
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 170);
            
            // 确定按钮
            if (ImGui::Button(loc.get("optionsDialog.ok").c_str(), ImVec2(80, 30))) {
                // 应用设置
                s_showGrid = showGrid;
                s_showAxes = showAxes;
                s_crossCursorSize = static_cast<float>(crossCursorSize);
                s_pickBoxSize = static_cast<float>(pickBoxSizeInt);
                ImGui::CloseCurrentPopup();
                s_optionsDialogVisible = false;
            }
            
            // 取消按钮
            ImGui::SameLine();
            if (ImGui::Button(loc.get("optionsDialog.cancel").c_str(), ImVec2(80, 30))) {
                // 不应用设置，直接关闭对话框
                ImGui::CloseCurrentPopup();
                s_optionsDialogVisible = false;
            }
            
            ImGui::EndPopup();
        }
    }
}

// 绘制菜单栏
void Renderer::drawMenuBar() {
    auto& loc = LocalizationManager::getInstance();
    if (ImGui::BeginMainMenuBar()) {
        // File菜单
        if (ImGui::BeginMenu(loc.get("menu.file").c_str())) {
            if (ImGui::MenuItem(loc.get("menu.file.new").c_str(), "Ctrl+N")) {
                CommandParser::executeCommand("new", {});
            }
            if (ImGui::MenuItem(loc.get("menu.file.open").c_str(), "Ctrl+O")) {
                CommandParser::executeCommand("open", {});
            }
            ImGui::MenuItem(loc.get("menu.file.openRecent").c_str());
            if (ImGui::MenuItem(loc.get("menu.file.save").c_str(), "Ctrl+S")) {
                CommandParser::executeCommand("save", {});
            }
            if (ImGui::MenuItem(loc.get("menu.file.saveAs").c_str(), "Ctrl+Shift+S")) {
                CommandParser::executeCommand("saveas", {});
            }
            if (ImGui::MenuItem(loc.get("menu.file.close").c_str(), "Ctrl+W")) {
                CommandParser::executeCommand("close", {});
            }
            ImGui::Separator();
            if (ImGui::MenuItem(loc.get("menu.file.quit").c_str(), "Ctrl+Q")) {
                CommandParser::executeCommand("quit", {});
            }
            ImGui::EndMenu();
        }
        
        // Edit菜单
        if (ImGui::BeginMenu(loc.get("menu.edit").c_str())) {
            if (ImGui::MenuItem(loc.get("menu.edit.undo").c_str(), "Ctrl+Z")) {
                CommandParser::executeCommand("undo", {});
            }
            if (ImGui::MenuItem(loc.get("menu.edit.redo").c_str(), "Ctrl+Y")) {
                CommandParser::executeCommand("redo", {});
            }
            ImGui::Separator();
            if (ImGui::MenuItem(loc.get("menu.edit.cut").c_str(), "Ctrl+X")) {
                CommandParser::executeCommand("cut", {});
            }
            if (ImGui::MenuItem(loc.get("menu.edit.copy").c_str(), "Ctrl+C")) {
                CommandParser::executeCommand("copy", {});
            }
            if (ImGui::MenuItem(loc.get("menu.edit.paste").c_str(), "Ctrl+V")) {
                CommandParser::executeCommand("paste", {});
            }
            if (ImGui::MenuItem(loc.get("menu.edit.selectAll").c_str(), "Ctrl+A")) {
                CommandParser::executeCommand("selectall", {});
            }
            if (ImGui::MenuItem(loc.get("menu.edit.erase").c_str(), "Del")) {
                CommandParser::executeCommand("erase", {});
            }
            ImGui::EndMenu();
        }
        
        // Tools菜单
        if (ImGui::BeginMenu(loc.get("menu.tools").c_str())) {
            if (ImGui::MenuItem(loc.get("menu.tools.options").c_str())) {
                // 执行OPTIONS命令
                Renderer::showOptionsDialog(true);
            }
            ImGui::MenuItem(loc.get("menu.tools.properties").c_str(), nullptr, &s_propertyBarVisible);
            ImGui::Separator();
            ImGui::MenuItem(loc.get("menu.tools.demo").c_str(), nullptr, &s_demoWindowVisible);
            ImGui::MenuItem(loc.get("menu.tools.metrics").c_str(), nullptr, &s_metricsWindowVisible);
            ImGui::EndMenu();
        }
        
        // Language菜单
        if (ImGui::BeginMenu(loc.get("menu.language").c_str())) {
            if (ImGui::MenuItem("English", nullptr, loc.getCurrentLanguage() == "en")) {
                loc.setLanguage("en");
            }
            if (ImGui::MenuItem("中文", nullptr, loc.getCurrentLanguage() == "zh")) {
                loc.setLanguage("zh");
            }
            ImGui::EndMenu();
        }
        
        // 更新菜单栏高度
        s_menuBarHeight = ImGui::GetFrameHeight();
        
        ImGui::EndMainMenuBar();
    }
}

// 绘制属性栏
void Renderer::drawPropertyBar() {
    if (!s_propertyBarVisible) {
        return;
    }
    
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    
    // 计算属性栏位置和大小
    float statusBarHeight = s_statusBarHeight;
    // 计算属性栏高度，从文件栏下方到状态栏上方
    float propertyBarHeight = height - statusBarHeight - s_menuBarHeight - s_fileBarHeight;
    // 计算属性栏位置，确保右侧与窗口对齐，底部与状态栏顶部对齐
    ImVec2 propertyBarPos(width - s_propertyBarWidth, s_menuBarHeight + s_fileBarHeight);
    ImVec2 propertyBarSize(s_propertyBarWidth, propertyBarHeight);
    
    // 绘制属性栏
    ImGui::SetNextWindowPos(propertyBarPos);
    ImGui::SetNextWindowSize(propertyBarSize);
    
    // 使用ImGui的原生窗口功能，支持拖动调整大小和关闭按钮
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    auto& loc = LocalizationManager::getInstance();
    // 使用ImGui的命名机制，##前面的内容显示在界面上，##后面的内容作为内部标识符
    std::string windowName = loc.get("propertyBar.title") + "##PropertyBar";
    if (ImGui::Begin(windowName.c_str(), &s_propertyBarVisible, flags)) {
        // 预留空白区域，等待添加实际属性
        
        // 监听属性栏宽度变化
        ImVec2 currentSize = ImGui::GetWindowSize();
        s_propertyBarWidth = currentSize.x;
        
        ImGui::End();
    }
}

// 绘制文件栏
void Renderer::drawFileBar() {
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    
    // 计算文件栏位置和大小
    ImVec2 fileBarPos(0, s_menuBarHeight);
    ImVec2 fileBarSize(width, s_fileBarHeight);
    
    // 绘制文件栏背景
    ImGui::SetNextWindowPos(fileBarPos);
    ImGui::SetNextWindowSize(fileBarSize);
    ImGui::SetNextWindowBgAlpha(0.9f);
    
    // 使用参考实现中的窗口标志
    ImGuiWindowFlags tabWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
                                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav | 
                                     ImGuiWindowFlags_NoSavedSettings;
    
    if (ImGui::Begin("FileBar", nullptr, tabWindowFlags)) {
        // 使用参考实现中的TabBar标志
        ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs | 
                                      ImGuiTabBarFlags_TabListPopupButton | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | 
                                      ImGuiTabBarFlags_FittingPolicyScroll;
        
        if (ImGui::BeginTabBar("FileTabBar", tabBarFlags)) {
            // 使用TabItemButton实现+号按钮
            if (ImGui::TabItemButton(" + ", ImGuiTabItemFlags_Trailing)) {
                // 创建新文件
                FileManager::createNewFile();
            }
            
            // 遍历文件列表
            std::size_t fileCount = FileManager::getFileCount();
            for (std::size_t i = 0; i < fileCount; i++) {
                // 获取文件名
                std::string tabText = FileManager::getFileName(i);
                
                // 设置标签项标志
                ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_None;
                if (FileManager::isFileModified(i)) {
                    tabItemFlags |= ImGuiTabItemFlags_UnsavedDocument;
                }
                
                bool tabOpen = true;
                if (ImGui::BeginTabItem(tabText.c_str(), &tabOpen, tabItemFlags)) {
                    // 设置当前文件索引
                    if (FileManager::getCurrentFileIndex() != i) {
                        FileManager::setCurrentFileIndex(i);
                    }
                    ImGui::EndTabItem();
                }
                
                // 添加工具提示
                if (ImGui::IsItemHovered()) {
                    const std::string& fullFileName = FileManager::getFullFileName(i);
                    const std::string& filePath = FileManager::getFilePath(i);
                    ImGui::SetTooltip(filePath.empty() ? fullFileName.c_str() : filePath.c_str());
                }
                
                // 处理标签关闭
                if (!tabOpen) {
                    FileManager::closeFile(i);
                }
            }
            
            ImGui::EndTabBar();
        }
        
        // 更新文件栏高度
        s_fileBarHeight = ImGui::GetWindowSize().y;
        
        ImGui::End();
    }
}

// 获取逻辑视口
LogicalViewport& Renderer::getLogicalViewport() {
    return s_logicalViewport;
}

// 绘制命令栏
void Renderer::drawCommandBar() {
    if (!s_commandBarVisible) {
        return;
    }
    
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    
    // 计算命令栏位置（位于状态栏正上方，属性栏左侧）
    float statusBarHeight = s_statusBarHeight;
    // 计算命令栏宽度，考虑属性栏的宽度
    float commandBarWidth = width - (s_propertyBarVisible ? s_propertyBarWidth : 0.0f);
    ImVec2 commandBarPos(0, height - statusBarHeight - s_commandBarHeight);
    ImVec2 commandBarSize(commandBarWidth, s_commandBarHeight);
    
    // 绘制命令栏
    ImGui::SetNextWindowPos(commandBarPos);
    ImGui::SetNextWindowSize(commandBarSize);
    ImGui::SetNextWindowBgAlpha(0.9f);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    auto& loc = LocalizationManager::getInstance();
    if (ImGui::Begin("CommandBar", nullptr, flags)) {
        // 创建区域显示命令历史，添加垂直和水平滚动条，留出空间给命令输入栏
        const float footerReserveHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("CommandHistory", ImVec2(0, -footerReserveHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        // TODO：使用ImGuiListClipper会导致滚动条的行为变得奇怪，无法自动滚动到末尾，暂不使用
        // // 使用静态的ImGuiListClipper来优化渲染，只绘制可见区域，提升历史条目过多时的性能
        // static ImGuiListClipper clipper;
        // float itemHeight = ImGui::GetTextLineHeight();
        // clipper.Begin(s_commandHistory.size(), itemHeight);
        
        // while (clipper.Step()) {
        //     for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
        //         // 绘制每一条命令历史
        //         ImGui::TextUnformatted(s_commandHistory[i].c_str());
        //     }
        // }
        // clipper.End();
        
        for (std::size_t i = 0; i < s_commandHistory.size(); i++)
        {
            ImGui::TextUnformatted(s_commandHistory[i].c_str());
        }
        
        // 根据标志决定是否滚动到最后，在绘制项目之前执行
        if (!s_commandHistory.empty() && s_bScrollCommandHistoryToBottom) {
            ImGui::SetScrollHereY(1.0f);
            s_bScrollCommandHistoryToBottom = false;
        }

        ImGui::EndChild();

        // 命令输入栏部分
        ImGui::Separator();
        
        // 调整布局：Command提示在左边，上下居中，输入框占满剩余空间
        ImGui::AlignTextToFramePadding();
        ImGui::Text(loc.get("commandBar.prompt").c_str());
        ImGui::SameLine();
        
        // 如果需要设置焦点到命令输入框
        if (s_bShouldFocusOnCommandInput) {
            ImGui::SetKeyboardFocusHere(0);
            s_bShouldFocusOnCommandInput = false;
        }
        
        // 使用PushItemWidth使输入框占满剩余空间
        ImGui::PushItemWidth(-1);
        
        // 检查是否需要取消命令
        if (s_bShouldCancelCommand) {
            // 取消命令执行，在命令历史中添加取消标记
            std::string command(s_cmdBuffer.data());
            // 使用localization资源构建取消命令的历史记录
            std::string promptStr = loc.get("commandBar.prompt") + " " + command + loc.get("commandBar.prompt.cancel");
            addContentToCommandHistory(promptStr);
            s_bShouldCancelCommand = false;
            s_bShouldExecuteCommand = false;
            // 清空缓冲区
            std::fill(s_cmdBuffer.begin(), s_cmdBuffer.end(), 0);
            // 设置清除命令输入缓冲区的标记，因为内部ImGui内部会维护InputText的状态，所以回调中还需要再清除一次
            s_bNeedClearCommandBuffer = true;
        }
        
        // 按下Enter/Space时执行命令，手动拦截执行，不再依赖ImGui的控件返回值
        bool bShouldExecute = s_bShouldExecuteCommand;
        // 执行命令
        if (bShouldExecute) {
            s_bShouldExecuteCommand = false;
            // 执行命令
            std::string command(s_cmdBuffer.data());
            std::string promptStr = loc.get("commandBar.prompt") + " " + command;
            addContentToCommandHistory(promptStr);
            if (!command.empty()) {
                CommandParser::parseCommand(command);
            }
            // 清空缓冲区
            std::fill(s_cmdBuffer.begin(), s_cmdBuffer.end(), 0);
            // 设置清除命令输入缓冲区的标记，因为内部ImGui内部会维护InputText的状态，所以回调中还需要再清除一次
            s_bNeedClearCommandBuffer = true;
        }
        
        // 回调函数处理文本选择问题、字符过滤与清除缓冲区
        auto inputTextCallback = [](ImGuiInputTextCallbackData* data) -> int {
            // 处理字符过滤逻辑 (只有输入字符时触发)
            if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
                if (data->EventChar > 127) {
                    return 1; // 丢弃非 ASCII 字符
                }
                // Space执行命令，丢弃，判断是否执行的逻辑则由InputHandler负责处理
                else if (data->EventChar == ' ')
                {
                    return 1;
                }
            }
            if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
                // 命令输入缓冲区被修改，那么就解除选中并移动光标到末尾
                if (s_bCommandBufferModified) {
                    // 直接修改缓冲区后，字符在下一帧可能处于选中状态，所以需要取消其选中状态
                    data->SelectionStart = data->SelectionEnd = data->BufTextLen;
                    // 焦点丢失后，其他位置的输入总是追加到末尾，所以总是移动光标到末尾，不管焦点丢失前光标在什么位置
                    data->CursorPos = data->BufTextLen;
                    // 重置标记
                    s_bCommandBufferModified = false;
                }
                // 需要清除命令输入缓冲区
                if (s_bNeedClearCommandBuffer) {
                    data->DeleteChars(0, data->BufTextLen); // 强制抹除 ImGui 内部的副本
                    data->CursorPos = 0;
                    // 重置标记
                    s_bNeedClearCommandBuffer = false;
                }
            }
            return 0;
        };

        ImGui::InputTextWithHint("##CommandInput", loc.get("commandBar.inputPrompt").c_str(), s_cmdBuffer.data(), s_cmdBuffer.size(), 
            ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_CallbackCharFilter, 
            inputTextCallback, nullptr);
        
        // 回车或者空格执行后强制把焦点拉回来，防止输入框失焦
        if (bShouldExecute) {
            ImGui::SetKeyboardFocusHere(-1); // -1 代表作用于上一个 Item（即当前的 InputText）
        }
        
        // 平衡PushItemWidth调用
        ImGui::PopItemWidth();
        
        ImGui::End();
    }
    
    // 绘制拖动条，允许调整命令栏高度
    static bool isResizing = false;
    static float resizeStartY = 0.0f;
    static float resizeStartHeight = 0.0f;
    
    // 减小拖动条高度，使其更美观
    float resizeBarHeight = 2.0f;
    ImVec2 resizeBarPos(0, height - statusBarHeight - s_commandBarHeight);
    ImVec2 resizeBarSize(commandBarWidth, resizeBarHeight);
    
    // 使用ImDrawList直接绘制拖动条，避免ImGui窗口最小高度限制
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    // 绘制拖动条背景
    drawList->AddRectFilled(resizeBarPos, ImVec2(resizeBarPos.x + resizeBarSize.x, resizeBarPos.y + resizeBarSize.y), ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 0.7f)));
    
    // 检测鼠标是否悬停在拖动条上
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isHovered = mousePos.x >= resizeBarPos.x && mousePos.x <= resizeBarPos.x + resizeBarSize.x && 
                     mousePos.y >= resizeBarPos.y && mousePos.y <= resizeBarPos.y + resizeBarSize.y;
    
    // 设置鼠标光标
    if (isHovered) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
    
    // 开始拖动
    if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        isResizing = true;
        resizeStartY = mousePos.y;
        resizeStartHeight = s_commandBarHeight;
    }
    
    // 正在拖动
    if (isResizing) {
        float currentY = mousePos.y;
        float deltaY = currentY - resizeStartY;
        float newHeight = resizeStartHeight - deltaY;
        
        // 限制高度范围，增大最小高度和最大高度以适应更大的屏幕
        if (newHeight > 150.0f && newHeight < 1080.0f) {
            s_commandBarHeight = newHeight;
        }
        
        // 停止拖动
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            isResizing = false;
        }
    }
    
    // 确保在窗口外释放鼠标时也能停止拖动
    if (isResizing && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        isResizing = false;
    }
}

// 添加内容到命令历史记录
void Renderer::addContentToCommandHistory(const std::string& command) {
    s_commandHistory.push_back(command);
    // 设置滚动标志为true，确保新命令添加后会滚动到最底部
    s_bScrollCommandHistoryToBottom = true;
}

// 设置是否应该将焦点设置到命令输入框
void Renderer::setShouldFocusOnCommandInput(bool shouldFocus) {
    s_bShouldFocusOnCommandInput = shouldFocus;
}

// 设置是否应该执行命令
void Renderer::setShouldExecuteCommand(bool shouldExecute) {
    s_bShouldExecuteCommand = shouldExecute;
}

// 设置是否应该取消命令执行
void Renderer::setShouldCancelCommand(bool shouldCancel) {
    s_bShouldCancelCommand = shouldCancel;
}

// 从命令输入缓冲区中删除最后一个字符
void Renderer::removeLastCharFromCommandInput() {
    size_t currentLen = std::strlen(s_cmdBuffer.data());
    if (currentLen > 0) {
        s_cmdBuffer[currentLen - 1] = '\0';
        // 设置标记，表示命令输入缓冲区被修改
        s_bCommandBufferModified = true;
    }
}

// 添加输入字符到命令输入框
void Renderer::addInputChar(unsigned int codepoint) {
    // 过滤非ASCII字符
    if (codepoint > 127) {
        return; // 丢弃非ASCII字符
    }
    
    size_t currentLen = std::strlen(s_cmdBuffer.data());
    if (currentLen < s_cmdBuffer.size() - 1) {
        s_cmdBuffer[currentLen] = static_cast<char>(codepoint);
        s_cmdBuffer[currentLen + 1] = '\0';
        // 设置标记，表示命令输入缓冲区被修改
        s_bCommandBufferModified = true;
    }
}

// 获取属性栏是否可见
bool Renderer::isPropertyBarVisible() {
    return s_propertyBarVisible;
}

// 设置属性栏是否可见
void Renderer::setPropertyBarVisible(bool visible) {
    s_propertyBarVisible = visible;
}

// 显示或隐藏选项对话框
void Renderer::showOptionsDialog(bool visible) {
    s_optionsDialogVisible = visible;
}



// 检查焦点是否位于指定窗口或其子窗口
bool Renderer::FocusIsOnWindow(const std::string& windowName) {
    if (ImGuiWindow* targetWindow = ImGui::FindWindowByName(windowName.c_str())) {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        if (ctx && ctx->NavWindow && ctx->NavWindow->RootWindow == targetWindow) {
            return true;
        }
    }
    return false;
}

// 检查焦点是否在命令输入框上
bool Renderer::FocusIsOnCommandInput() {
    // 获取当前活跃控件的ID
    ImGuiID activeID = ImGui::GetActiveID();
    
    // 获取命令输入框所在的窗口
    ImGuiWindow* window = ImGui::FindWindowByName("CommandBar");
    if (window) {
        // 获取命令输入框的ID
        ImGuiID inputID = window->GetID("##CommandInput");
        if (activeID == inputID) {
            // 焦点确实在命令输入框上
            return true;
        }
    }
    return false;
}

} // namespace tch