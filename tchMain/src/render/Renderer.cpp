#include "render/Renderer.h"
#include "Layer.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "sys/Global.h"
#include "debug/Logger.h"
#include <algorithm>
#include <array>

namespace tch {

// 静态成员初始化
bool Renderer::s_initialized = false;
GLFWwindow* Renderer::s_window = nullptr;
float Renderer::s_cursorSize = 20.0f;

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

// 命令栏相关
static std::vector<std::string> s_commandHistory; // 命令执行历史
static std::string s_currentCommand = ""; // 当前命令输入
static bool s_commandBarVisible = true; // 命令栏是否可见
static float s_commandBarHeight = 150.0f; // 命令栏高度

// 属性栏相关
static bool s_propertyBarVisible = true; // 属性栏是否可见
static float s_propertyBarWidth = 250.0f; // 属性栏宽度

// 文件栏相关
static std::vector<std::string> s_files; // 打开的文件列表
static int s_currentFileIndex = 0; // 当前文件索引
static int s_fileCounter = 0; // 用于生成新文件名的计数器

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
    
    // 初始化文件列表，创建一个默认的未命名文件
    s_files.clear();
    s_files.push_back("unnamed-" + std::to_string(s_fileCounter));
    s_fileCounter++;
    s_currentFileIndex = 0;
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
    
    // 绘制空心框选框
    float boxSize = s_cursorSize * 0.25f;
    glBegin(GL_LINE_LOOP);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
    glVertex2f(cursorScreenPos.x - boxSize, cursorScreenPos.y - boxSize);
    glVertex2f(cursorScreenPos.x + boxSize, cursorScreenPos.y - boxSize);
    glVertex2f(cursorScreenPos.x + boxSize, cursorScreenPos.y + boxSize);
    glVertex2f(cursorScreenPos.x - boxSize, cursorScreenPos.y + boxSize);
    glEnd();
    
    // 绘制从正方形四条边中点向外延伸的光标线条
    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
    
    // 上边中点向上延伸
    glVertex2f(cursorScreenPos.x, cursorScreenPos.y - boxSize);
    glVertex2f(cursorScreenPos.x, cursorScreenPos.y - s_cursorSize);
    
    // 下边中点向下延伸
    glVertex2f(cursorScreenPos.x, cursorScreenPos.y + boxSize);
    glVertex2f(cursorScreenPos.x, cursorScreenPos.y + s_cursorSize);
    
    // 左边中点向左延伸
    glVertex2f(cursorScreenPos.x - boxSize, cursorScreenPos.y);
    glVertex2f(cursorScreenPos.x - s_cursorSize, cursorScreenPos.y);
    
    // 右边中点向右延伸
    glVertex2f(cursorScreenPos.x + boxSize, cursorScreenPos.y);
    glVertex2f(cursorScreenPos.x + s_cursorSize, cursorScreenPos.y);
    glEnd();
    
    // 恢复矩阵状态
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // 重新启用深度测试
    glEnable(GL_DEPTH_TEST);
}

// 设置光标大小
void Renderer::setCursorSize(float size) {
    s_cursorSize = size;
}

// 获取光标大小
float Renderer::getCursorSize() {
    return s_cursorSize;
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

// 初始化ImGui
void Renderer::initializeImGui() {
    // 设置ImGui上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // 配置ImGui
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    
    // 加载自定义字体
    std::filesystem::path fontPath = g_pathCwd / "fonts" / "consolas.ttf";
    std::string fontPathStr = fontPath.string();
    
    LOG_INFO("Attempting to load font from: {}", fontPathStr);
    
    // 尝试加载字体
    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), 16.0f);
    
    if (font) {
        LOG_INFO("Font loaded successfully: consolas.ttf");
    } else {
        LOG_WARNING("Failed to load font: {}", fontPathStr);
        LOG_INFO("Using default ImGui font instead with size 16");
        
        // 即使使用默认字体，也设置字号为16
        io.Fonts->Clear();
        ImFontConfig config;
        config.SizePixels = 16.0f;
        io.Fonts->AddFontDefault(&config);
        io.Fonts->Build();
    }
    
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

// 绘制菜单栏
void Renderer::drawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        // File菜单
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New");
            ImGui::MenuItem("Open");
            ImGui::MenuItem("Open Recent");
            ImGui::MenuItem("Save");
            ImGui::MenuItem("Save As...");
            ImGui::MenuItem("Close");
            ImGui::Separator();
            ImGui::MenuItem("Quit");
            ImGui::EndMenu();
        }
        
        // Edit菜单
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::Separator();
            ImGui::MenuItem("Cut");
            ImGui::MenuItem("Copy");
            ImGui::MenuItem("Paste");
            ImGui::MenuItem("Select All");
            ImGui::MenuItem("Erase");
            ImGui::EndMenu();
        }
        
        // Tools菜单
        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("Options");
            ImGui::MenuItem("Properties", nullptr, &s_propertyBarVisible);
            ImGui::EndMenu();
        }
        
        // 更新菜单栏高度
        s_menuBarHeight = ImGui::GetFrameHeight();
        
        ImGui::EndMainMenuBar();
    }
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
    
    if (ImGui::Begin("CommandBar", nullptr, flags)) {
        // 命令执行历史部分（去掉标题和最上方的分隔符）
        
        // 创建区域显示命令历史，去掉周围的框，添加水平滚动条
        ImGui::BeginChild("CommandHistory", ImVec2(0, -30), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        // 使用单个只读InputText显示所有命令历史，支持多行选择和复制
        std::string historyText;
        size_t maxCommandLength = 0;
        for (size_t i = 0; i < s_commandHistory.size(); i++) {
            const auto& cmd = s_commandHistory[i];
            historyText += cmd;
            // 最后一行不添加换行符
            if (i < s_commandHistory.size() - 1) {
                historyText += "\n";
            }
            if (cmd.length() > maxCommandLength) {
                maxCommandLength = cmd.length();
            }
        }
        
        // 为InputText创建一个静态缓冲区，避免每次都重新分配内存
        static std::string buffer;
        buffer = historyText;
        
        // 计算InputText的宽度，使用最长命令的长度，确保能显示完整的命令
        // 每个字符大约占8个像素宽度（根据字体大小调整）
        float charWidth = 8.0f;
        float minWidth = ImGui::GetWindowWidth();
        float neededWidth = maxCommandLength * charWidth + 20; // 20是额外的边距
        float inputTextWidth = std::max(minWidth, neededWidth);
        
        // 保存当前样式
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
        
        // 获取子窗口高度，直接使用子窗口的完整高度，不再减去30
        float childHeight = ImGui::GetWindowHeight();
        
        // 使用只读InputText，支持多行选择和复制，设置宽度为计算出的宽度，高度为子窗口的完整大小
        ImGui::InputTextMultiline("##CommandHistory", &buffer[0], buffer.size(), ImVec2(inputTextWidth, childHeight), 
                                 ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoUndoRedo | 
                                 ImGuiInputTextFlags_AllowTabInput);
        
        // 暂时移除自动滚动功能，等后续命令执行相关内容完成后再实现
        // ImGui::SetScrollHereY(1.0f);
        // ImGui::SetScrollY(ImGui::GetScrollMaxY());
        
        // 恢复样式
        ImGui::PopStyleColor(3);
        
        ImGui::EndChild();
        
        // 命令输入栏部分
        ImGui::Separator();
        
        // 调整布局：Command提示在左边，上下居中，输入框占满剩余空间
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Command:");
        ImGui::SameLine();
        
        std::array<char, 256> commandBuffer{};
        if (!s_currentCommand.empty()) {
            // 安全地复制字符串，避免越界
            size_t copySize = std::min(s_currentCommand.size(), commandBuffer.size() - 1);
            std::copy(s_currentCommand.begin(), s_currentCommand.begin() + copySize, commandBuffer.begin());
            commandBuffer[copySize] = '\0'; // 确保以空字符结尾
        }
        
        // 使用PushItemWidth使输入框占满剩余空间
        ImGui::PushItemWidth(-1);
        // 添加提示字符串
        ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
        if (s_currentCommand.empty()) {
            inputFlags |= ImGuiInputTextFlags_CallbackAlways;
        }
        
        // 使用更简单的方式添加提示文本
        if (ImGui::InputTextWithHint("##CommandInput", "Input Command Here", commandBuffer.data(), commandBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            // 当用户按下Enter键时，添加命令到历史记录并清空输入
            std::string command(commandBuffer.data());
            if (!command.empty()) {
                s_commandHistory.push_back(command);
                s_currentCommand.clear();
            }
        } else {
            // 更新当前命令输入
            s_currentCommand = commandBuffer.data();
        }
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
    
    if (ImGui::Begin("Properties", &s_propertyBarVisible, flags)) {
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
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing)) {
                // 创建新文件
                s_files.push_back("unnamed-" + std::to_string(s_fileCounter));
                s_fileCounter++;
                s_currentFileIndex = s_files.size() - 1;
            }
            
            // 遍历文件列表
            for (size_t i = 0; i < s_files.size(); i++) {
                // 使用ImGui的TabItem，带有关闭按钮
                ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_None;
                
                bool tabOpen = true;
                if (ImGui::BeginTabItem(s_files[i].c_str(), &tabOpen, tabItemFlags)) {
                    // 设置当前文件索引
                    if (s_currentFileIndex != i) {
                        s_currentFileIndex = i;
                    }
                    ImGui::EndTabItem();
                }
                
                // 添加工具提示
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(s_files[i].c_str());
                }
                
                // 处理标签关闭
                if (!tabOpen) {
                    if (s_files.size() > 1) {
                        s_files.erase(s_files.begin() + i);
                        if (s_currentFileIndex >= i) {
                            s_currentFileIndex = std::max(0, s_currentFileIndex - 1);
                        }
                    } else {
                        // 如果只剩最后一个文件，关闭后创建新文件
                        s_files.clear();
                        s_files.push_back("unnamed-" + std::to_string(s_fileCounter));
                        s_fileCounter++;
                        s_currentFileIndex = 0;
                    }
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

// 添加命令到历史记录
void Renderer::addCommandToHistory(const std::string& command) {
    s_commandHistory.push_back(command);
}



} // namespace tch