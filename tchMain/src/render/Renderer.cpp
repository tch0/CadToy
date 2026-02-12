#include "render/Renderer.h"
#include "Layer.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "sys/Global.h"
#include "debug/Logger.h"
#include <algorithm>

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
}

// 清理渲染器
void Renderer::cleanup() {
    // 清理ImGui
    cleanupImGui();
    
    s_initialized = false;
    s_window = nullptr;
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
    
    // 更新当前光标位置（使用逻辑视口转换）
    glm::dvec3 logicPos = s_logicalViewport.screenToLogic(position);
    s_cursorPosition = logicPos;
    
    // 保存当前矩阵状态
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    // 设置正交投影，Y轴朝上
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    glOrtho(0, width, 0, height, -1, 1);
    
    // 切换到模型视图矩阵
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // 禁用深度测试
    glDisable(GL_DEPTH_TEST);
    
    // 绘制空心框选框
    float boxSize = s_cursorSize * 0.25f;
    glBegin(GL_LINE_LOOP);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
    glVertex2f(position.x - boxSize, position.y - boxSize);
    glVertex2f(position.x + boxSize, position.y - boxSize);
    glVertex2f(position.x + boxSize, position.y + boxSize);
    glVertex2f(position.x - boxSize, position.y + boxSize);
    glEnd();
    
    // 绘制从正方形四条边中点向外延伸的光标线条
    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
    
    // 上边中点向上延伸
    glVertex2f(position.x, position.y - boxSize);
    glVertex2f(position.x, position.y - s_cursorSize);
    
    // 下边中点向下延伸
    glVertex2f(position.x, position.y + boxSize);
    glVertex2f(position.x, position.y + s_cursorSize);
    
    // 左边中点向左延伸
    glVertex2f(position.x - boxSize, position.y);
    glVertex2f(position.x - s_cursorSize, position.y);
    
    // 右边中点向右延伸
    glVertex2f(position.x + boxSize, position.y);
    glVertex2f(position.x + s_cursorSize, position.y);
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
    
    // 设置正交投影，Y轴朝上
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    glOrtho(0, width, 0, height, -1, 1);
    
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
    
    // 基础栅格间距（逻辑坐标）
    const double baseGridSize = 10.0;
    
    // 计算当前有效的栅格间距（考虑视口大小）
    double currentEffectiveSize = baseGridSize;
    
    // 确定栅格级别
    // 目标是保持栅格在屏幕上的大小在合理范围内（50-250像素）
    double mainGridSize, subGridSize;
    
    // 计算当前栅格在屏幕上的大小（水平和垂直方向）
    double gridScreenSizeX = (baseGridSize / logicWidth) * width;
    double gridScreenSizeY = (baseGridSize / logicHeight) * height;
    
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
    
    // 设置正交投影，Y轴朝上
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    glOrtho(0, width, 0, height, -1, 1);
    
    // 切换到模型视图矩阵
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // 禁用深度测试
    glDisable(GL_DEPTH_TEST);
    
    // 计算逻辑原点在屏幕上的位置
    glm::vec2 originScreenPos = s_logicalViewport.logicToScreen(glm::dvec3(0.0, 0.0, 0.0));
    float originScreenX = originScreenPos.x;
    float originScreenY = originScreenPos.y;
    
    // 绘制X轴（正半轴）
    glBegin(GL_LINES);
    glColor3fv(s_xAxisColor);
    
    // 只绘制在屏幕范围内的部分
    if (originScreenX >= 0 && originScreenX <= width && originScreenY >= 0 && originScreenY <= height) {
        glVertex2f(originScreenX, originScreenY);
        glVertex2f(width, originScreenY);
    }
    glEnd();
    
    // 绘制Y轴（正半轴，朝上）
    glBegin(GL_LINES);
    glColor3fv(s_yAxisColor);
    
    // 只绘制在屏幕范围内的部分
    if (originScreenX >= 0 && originScreenX <= width && originScreenY >= 0 && originScreenY <= height) {
        glVertex2f(originScreenX, originScreenY);
        glVertex2f(originScreenX, height);
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
    float statusBarHeight = 35.0f;
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
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

// 获取逻辑视口
LogicalViewport& Renderer::getLogicalViewport() {
    return s_logicalViewport;
}

} // namespace tch