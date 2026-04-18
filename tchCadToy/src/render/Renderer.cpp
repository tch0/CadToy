// 对应头文件
#include "Renderer.h"

// C++ 标准库
#include <algorithm>
#include <array>
#include <cstring>
#include <memory>

// 第三方库
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <glm/gtc/matrix_transform.hpp>

// 项目头文件
#include "CommonTypes.h"
#include "CommandManager.h"
#include "Logger.h"
#include "DocManager.h"
#include "InputContext.h"
#include "InputHandler.h"
#include "Global.h"
#include "DisplayConfigManager.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"

namespace tch {

// 静态成员初始化
bool Renderer::s_initialized = false;
GLFWwindow* Renderer::s_window = nullptr;
CanvasRenderer Renderer::s_canvasRenderer;
EntityRenderer Renderer::s_entityRenderer;
float Renderer::s_crossCursorSize = 50.0f;
float Renderer::s_pickBoxSize = 5.0f;      // 拾取框大小，默认值为5
bool Renderer::s_cursorTestWindowVisible = false;

// 当前光标位置的世界坐标
glm::dvec3 Renderer::s_cursorPosWorld = glm::dvec3(0.0, 0.0, 0.0);

// UI组件高度
float Renderer::s_menuBarHeight = 30.0f;              // 菜单栏高度
float Renderer::s_fileBarHeight = 30.0f;              // 文件栏高度
float Renderer::s_statusBarHeight = 35.0f;            // 状态栏高度

// 命令行历史滚动控制
bool Renderer::s_bScrollCommandLineHistoryToBottom = false; // 是否应该将命令行历史滚动到底部
// 命令输入框焦点控制
bool Renderer::s_bShouldFocusOnCommandInput = false; // 是否应该将焦点设置到命令输入框
// 命令输入缓冲区是否被修改，通过非命令输入栏的字符输入或者退格
bool Renderer::s_bCommandBufferModified = false;
// 是否需要清除ImGui的命令输入缓冲区内部副本
bool Renderer::s_bNeedClearCommandBufferInternalCopy = false;
// CommandBar窗口ID，初始化为0，表示无效ID
ImGuiID Renderer::s_commandBarId = 0;
// CommandInput输入控件ID，初始化为0，表示无效ID
ImGuiID Renderer::s_commandInputId = 0;
// 命令历史导航索引，-1 表示不在命令历史导航模式
int Renderer::s_commandHistoryNavigationIndex = -1;

// 命令栏相关
static bool s_commandBarVisible = true; // 命令栏是否可见
static float s_commandBarHeight = 150.0f; // 命令栏高度
static std::array<char, 256> s_cmdBuffer{}; // 命令输入缓冲区

// 命令补全、候选框相关
// 当前用户输入的那一部分命令，比如说输入L，补全为LINE，此时命令缓冲区已经修改为LINE，但s_userInputCommand一直都是L
// 直到焦点丢失又重新找回或者又通过键盘添加删减了字符才会更新，这个字符串就是补全拿去查找的依据
static std::string s_userInputCommand;
static std::vector<CommandCompletionItem> s_completionCandidates; // 补全候选列表
static int s_completionSelectedIndex = -1; // 选中索引

// 属性栏相关
static bool s_propertyBarVisible = true;    // 属性栏是否可见
static float s_propertyBarWidth = 250.0f;   // 属性栏宽度

// 示例与调试窗口相关
static bool s_demoWindowVisible = false;     // Demo窗口是否可见
static bool s_metricsWindowVisible = false;  // Metrics/Debugger窗口是否可见

// 实时渲染信息窗口相关
static bool s_renderingInfoVisible = false;  // 实时渲染信息窗口是否可见

// 背景颜色常量 RGB: 33,40,48
static glm::vec4 s_backgroundColor(33.0f/255.0f, 40.0f/255.0f, 48.0f/255.0f, 1.0f);




// 初始化渲染器
void Renderer::initialize(GLFWwindow* window) {
    s_window = window;
    s_initialized = true;
    
    // 获取窗口尺寸并设置视口
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    
    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 初始化ImGui
    initializeImGui();
    
    // 初始化显示器配置管理器
    DisplayConfigManager::getInstance().initialize(window);
    
    // 初始化本地化管理器
    LocalizationManager::getInstance().initialize();
    
    // 初始化文件管理器
    DocManager::initialize();
    
    // 初始化CanvasRenderer
    if (!s_canvasRenderer.initialize()) {
        LOG_ERROR("Failed to initialize CanvasRenderer");
    }
    
    // 初始化EntityRenderer
    if (!s_entityRenderer.initialize()) {
        LOG_ERROR("Failed to initialize EntityRenderer");
    }
    
    // 禁用抗锯齿，需要时再启用
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);
    glDisable(GL_MULTISAMPLE);
}

// 清理渲染器
void Renderer::cleanup() {
    // 清理EntityRenderer
    s_entityRenderer.cleanup();
    
    // 清理CanvasRenderer
    s_canvasRenderer.cleanup();
    
    // 清理ImGui
    cleanupImGui();
    
    s_initialized = false;
    s_window = nullptr;
}

// 获取渲染器状态
bool Renderer::isInitialized() {
    return s_initialized;
}

// 计算布局并更新视口
void Renderer::calculateLayoutAndUpdateViewport() {
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);

    // 设置OpenGL视口
    glViewport(0, 0, width, height);

    // 计算视口边界
    // 左侧：0
    // 顶部：菜单栏高度 + 文件栏高度
    // 右侧：窗口宽度 - (属性栏宽度 if 可见)
    // 底部：窗口高度 - 状态栏高度 - (命令栏高度 if 可见)
    int left = 0;
    int top = static_cast<int>(s_menuBarHeight + s_fileBarHeight);
    int right = width - (s_propertyBarVisible ? static_cast<int>(s_propertyBarWidth) : 0);
    int bottom = height - static_cast<int>(s_statusBarHeight) - (s_commandBarVisible ? static_cast<int>(s_commandBarHeight) : 0);
    
    // 确保视口有效
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
    
    // 更新变换管理器的视口
    getTransformManager().setViewport(left, top, right, bottom);
}

// 开始渲染
void Renderer::beginRender() {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 清除颜色缓冲和模板缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // 开始ImGui渲染
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 使用ImGui的原生API来控制光标显示
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        // 当ImGui不想要捕获鼠标时，设置鼠标光标为None，此时在画布上自绘光标
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    }

    // 每一帧开头重算布局更新视口
    Renderer::calculateLayoutAndUpdateViewport();
}

// 结束渲染
void Renderer::endRender() {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 处理鼠标悬停与非模态窗口的焦点问题：
    //      CAD程序需要鼠标位于画布上时时刻保持对键盘输入的获取，而imgui的非模态窗口只要点击一下后焦点就会一直留在窗口内，即使鼠标已经移出窗口。
    //      那么就需要在鼠标移出了窗口位于画布上时，如果有窗口获得了焦点就将焦点置为空，这样CAD主程序才能获取到键盘输入，每一帧在ImGui::Render前
    //      检测一次就行，这时ImGui已经计算完成所有的窗口位置和遮盖关系，这时候做最准确不会出任何问题。
    
    // 当前没有模态对话框才释放
    //      如果有模态对话框（如“确认保存”弹窗），则必须保持焦点在模态对话框上，不执行自动释放
    // 而且还必须没有任何任何弹出层（菜单、下拉菜单、Combo等）才释放
    //      菜单项就是弹出层，菜单的工作机制涉及焦点的切换，简单地释放焦点会破坏其状态导致一闪而过
    if (ImGui::GetTopMostPopupModal() == nullptr &&
        !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
        // 有任何窗口获得了焦点
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
            
            // 检查鼠标是否悬停在任何ImGui窗口（及其子菜单/弹出项）上
            // RootAndChildWindows: 保证鼠标在子菜单时主窗口不失焦
            // AllowWhenBlockedByActiveItem: 保证鼠标点击按钮或滑动条时，即便稍稍滑出边界也不失焦
            bool mouseOverUI = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | 
                                                     ImGuiHoveredFlags_RootAndChildWindows | 
                                                     ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            
            // 关键：检查是否有控件正处于“活动状态”（如正在输入文本、拖动滑块、按住按钮）
            // 如果正在操作UI，即便鼠标移出了窗口边界，也不应该强行夺走焦点
            bool isInteracting = ImGui::IsAnyItemActive();
            
            // 另外焦点位于命令栏上时也不释放，因为焦点位于画布(NULL)时有键盘输入进来后本来就会将焦点切到命令栏的命令输入框上进行命令输入，
            // 但不能仅判断焦点位于命令输入框时不释放，因为命令输入框只是一个命令栏上的控件，焦点不是全时保持在其上的，但能够确定的是
            // 在所有的键盘字符交互过程中焦点的归宿就是命令栏，那么如果焦点已经位于命令栏上了当然是不应该释放的。
            bool bFocusIsOnCommandBar = focusIsOnCommandBar();
            
            // 没有悬停在任何窗口上、且没有和任何UI元素进行交互、且焦点在除命令栏外的其他窗口上时，就释放焦点
            if (!mouseOverUI && !isInteracting && !bFocusIsOnCommandBar) {
                ImGui::SetWindowFocus(NULL); // 释放焦点，让io.WantCaptureKeyboard变为false，此时所有键盘鼠标输入都将被主程序获取
            }
        }
    }
    
    // 渲染ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // 交换缓冲区
    glfwSwapBuffers(s_window);
}

// 绘制所有图形
void Renderer::drawAll() {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    
    // 更新 EntityRenderer 窗口尺寸
    s_entityRenderer.updateWindowSize(width, height);
    
    // 设置投影矩阵（屏幕坐标系，Y轴向下）
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), 
                                       static_cast<float>(height), 0.0f, 
                                       -1.0f, 1.0f);
    s_canvasRenderer.setProjection(projection);
    
    // 设置视图矩阵（单位矩阵，因为我们在屏幕坐标系中绘制）
    glm::mat4 view = glm::mat4(1.0f);
    s_canvasRenderer.setView(view);
    
    // 1. 清除默认帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClearColor(s_backgroundColor.r, s_backgroundColor.g, s_backgroundColor.b, s_backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // 2. 背景层（CanvasRenderer）
    s_canvasRenderer.drawGrid();
    s_canvasRenderer.drawAxes();
    
    // 3. 实体层（EntityRenderer）
    s_entityRenderer.drawEntities();
    
    // 4. 前景层（CanvasRenderer）
    s_canvasRenderer.drawSelection();
    s_canvasRenderer.drawCursor();
    s_canvasRenderer.drawCursorMarker();
}

// 绘制光标测试窗口
void Renderer::drawCursorTestWindow() {
    if (s_cursorTestWindowVisible) {
        float uiScale = Utils::getUIScaleFactor();
        ImGui::Begin("Cursor Test Window", &s_cursorTestWindowVisible);
        
        // 获取交互数据
        InteractionData& interactionData = InputContext::getInstance().getInteractionData();
        
        // 光标模式选择
        static const char* cursorModeNames[] = {"Default", "Crosshair", "Pickbox", "Panning"};
        static int currentMode = static_cast<int>(interactionData.cursorMode);
        if (ImGui::Combo("Cursor Mode", &currentMode, cursorModeNames, IM_ARRAYSIZE(cursorModeNames), 4)) {
            interactionData.cursorMode = static_cast<CursorMode>(currentMode);
        }
        
        // 光标标记选择
        static const char* cursorMarkerNames[] = {"None", "Locked", "Orthogonal", "Erase", "Copy", "Move", "Rotate", "Scale", "AddSelect", "RemoveSelect", "CrossingSelect", "WindowSelect"};
        static int currentMarker = static_cast<int>(interactionData.cursorMarker);
        if (ImGui::Combo("Cursor Marker", &currentMarker, cursorMarkerNames, IM_ARRAYSIZE(cursorMarkerNames), 12)) {
            interactionData.cursorMarker = static_cast<CursorMarker>(currentMarker);
        }
        
        // 光标尺寸拖动条（范围10~200）
        static int crossCursorSizeInt = static_cast<int>(s_crossCursorSize);
        ImGui::PushItemWidth(300 * uiScale);
        if (ImGui::SliderInt("Cursor Size", &crossCursorSizeInt, 10, 200, "%d")) {
            s_crossCursorSize = static_cast<float>(crossCursorSizeInt);
        }
        ImGui::PopItemWidth();
        
        // 拾取框尺寸拖动条（范围0~50）
        static int pickBoxSizeInt = static_cast<int>(s_pickBoxSize);
        ImGui::PushItemWidth(300 * uiScale);
        if (ImGui::SliderInt("Pickbox Size", &pickBoxSizeInt, 0, 50, "%d")) {
            s_pickBoxSize = static_cast<float>(pickBoxSizeInt);
        }
        ImGui::PopItemWidth();
        
        ImGui::End();
    }
}

// 设置十字光标大小
void Renderer::setCrossCursorSize(float size) {
    s_crossCursorSize = size;
}

// 获取十字光标大小
float Renderer::getCrossCursorSize() {
    return s_crossCursorSize;
}

// 设置拾取框大小
void Renderer::setPickBoxSize(float size) {
    s_pickBoxSize = size;
}

// 获取拾取框大小
float Renderer::getPickBoxSize() {
    return s_pickBoxSize;
}

// 获取当前光标世界坐标
glm::dvec3 Renderer::getCursorPosWorld() {
    return s_cursorPosWorld;
}

void Renderer::zoomIn(const glm::vec2& cursorPos) {
    // 使用变换管理器进行缩放，以光标位置为中心
    getTransformManager().zoomIn(cursorPos);
}

void Renderer::zoomOut(const glm::vec2& cursorPos) {
    // 使用变换管理器进行缩放，以光标位置为中心
    getTransformManager().zoomOut(cursorPos);
}

// 平移功能
void Renderer::pan(const glm::vec2& deltaScreen) {
    // 使用变换管理器进行平移
    getTransformManager().pan(deltaScreen);
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
    std::string consolasPathStr = PathUtils::toString(consolasPath);
    
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
    std::string msyhPathStr = PathUtils::toString(msyhPath);
    
    LOG_INFO("Attempting to load Chinese font from: {}", msyhPathStr);
    
    // 尝试加载微软雅黑字体，加载完整中文字符集以确保所有汉字都能显示，中文字号略大看起来才和英文匹配
    // v1.92版本后，按需动态加载字形，能够大幅减少内存占用，不再需要指定字形范围
    ImFont* msyhFont = io.Fonts->AddFontFromFileTTF(msyhPathStr.c_str(), 22.0f, &config, nullptr);
    
    if (!msyhFont) {
        LOG_WARNING("Failed to load Microsoft YaHei font: {}", msyhPathStr);
        LOG_INFO("Chinese characters may not display correctly");
    }
    
    // 4. 构建字体图集
    // 这一步不再需要了，imgui更新到1.92版本后支持了动态加载字形，需要由imgui自动构建，而不是手动调用
    // io.Fonts->Build();
    
    LOG_INFO("Font loading completed");
    
    // 设置ImGui样式
    ImGui::StyleColorsDark();
    
    // 初始化ImGui GLFW后端
    ImGui_ImplGlfw_InitForOpenGL(s_window, true);
    
    // 初始化ImGui OpenGL3后端
    const char* glsl_version = "#version 330";
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // 修改UI设置
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;       // 窗口圆角
    style.ChildRounding  = 2.0f;       // 子窗口圆角
    style.FrameRounding  = 4.0f;       // 输入框、按钮圆角
    style.PopupRounding  = 4.0f;       // 菜单、弹窗圆角
    style.ScrollbarRounding = 9.0f;    // 滚动条圆角
    style.GrabRounding   = 2.0f;       // 滑块圆角
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // 输入框获得焦点后不再显示蓝色边框
    
    // 抗锯齿开关（确保开启）
    style.AntiAliasedLines = true;
    style.AntiAliasedFill  = true;
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
void Renderer::drawStatusBar() {
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    
    // 更新状态栏高度：一行文本的高度 + 内边距
    s_statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2;
    
    // 计算状态栏位置和大小
    float statusBarHeight = s_statusBarHeight;
    ImVec2 statusBarPos(0, height - statusBarHeight);
    ImVec2 statusBarSize(width*1.0f, statusBarHeight);
    
    // 绘制状态栏
    ImGui::SetNextWindowPos(statusBarPos);
    ImGui::SetNextWindowSize(statusBarSize);
    ImGui::SetNextWindowBgAlpha(0.9f);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::Begin("StatusBar", nullptr, flags);
    // 计算世界坐标，保存并显示
    s_cursorPosWorld = getTransformManager().screenToWorld(InputHandler::getCursorPosition());
    ImGui::Text("%.4f, %.4f, %.4f", s_cursorPosWorld.x, s_cursorPosWorld.y, s_cursorPosWorld.z);
    
    ImGui::End();
}

// 绘制菜单栏
void Renderer::drawMenuBar() {
    auto& loc = LocalizationManager::getInstance();
    if (ImGui::BeginMainMenuBar()) {
        // File菜单
        if (ImGui::BeginMenu(loc.get("menu.file").c_str())) {
            if (ImGui::MenuItem(loc.get("menu.file.new").c_str(), "Ctrl+N")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("new");
            }
            if (ImGui::MenuItem(loc.get("menu.file.open").c_str(), "Ctrl+O")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("open");
            }
            ImGui::MenuItem(loc.get("menu.file.openRecent").c_str());
            if (ImGui::MenuItem(loc.get("menu.file.save").c_str(), "Ctrl+S")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("save");
            }
            if (ImGui::MenuItem(loc.get("menu.file.saveAs").c_str(), "Ctrl+Shift+S")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("saveas");
            }
            if (ImGui::MenuItem(loc.get("menu.file.close").c_str(), "Ctrl+W")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("close");
            }
            ImGui::Separator();
            if (ImGui::MenuItem(loc.get("menu.file.quit").c_str(), "Ctrl+Q")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("quit");
            }
            ImGui::EndMenu();
        }
        
        // Edit菜单
        if (ImGui::BeginMenu(loc.get("menu.edit").c_str())) {
            if (ImGui::MenuItem(loc.get("menu.edit.undo").c_str(), "Ctrl+Z")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("undo");
            }
            if (ImGui::MenuItem(loc.get("menu.edit.redo").c_str(), "Ctrl+Y")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("redo");
            }
            ImGui::Separator();
            if (ImGui::MenuItem(loc.get("menu.edit.cut").c_str(), "Ctrl+X")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("cut");
            }
            if (ImGui::MenuItem(loc.get("menu.edit.copy").c_str(), "Ctrl+C")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("copy");
            }
            if (ImGui::MenuItem(loc.get("menu.edit.paste").c_str(), "Ctrl+V")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("paste");
            }
            ImGui::Separator();
            if (ImGui::MenuItem(loc.get("menu.edit.selectAll").c_str(), "Ctrl+A")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("selectall");
            }
            if (ImGui::MenuItem(loc.get("menu.edit.erase").c_str(), "Del")) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("erase");
            }
            ImGui::EndMenu();
        }
        
        // Tools菜单
        if (ImGui::BeginMenu(loc.get("menu.tools").c_str())) {
            if (ImGui::MenuItem(loc.get("menu.tools.options").c_str())) {
                CommandManager::getInstance().cancelCurrentCommandAndExecute("options");
            }
            ImGui::MenuItem(loc.get("menu.tools.properties").c_str(), nullptr, &s_propertyBarVisible);
            ImGui::Separator();
            ImGui::MenuItem(loc.get("menu.tools.demo").c_str(), nullptr, &s_demoWindowVisible);
            ImGui::MenuItem(loc.get("menu.tools.metrics").c_str(), nullptr, &s_metricsWindowVisible);
            ImGui::Separator();
            ImGui::MenuItem(loc.get("menu.tools.renderingInfo").c_str(), nullptr, &s_renderingInfoVisible);
            ImGui::MenuItem(loc.get("menu.tools.inputTextInfo").c_str(), nullptr, &InputContext::getInstance().getInputContextInfoVisible());
            ImGui::MenuItem(loc.get("menu.tools.cursorTestWindow").c_str(), nullptr, &s_cursorTestWindowVisible);
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
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;
    
    auto& loc = LocalizationManager::getInstance();
    // 使用ImGui的命名机制，##前面的内容显示在界面上，##后面的内容作为内部标识符
    std::string windowName = loc.get("propertyBar.title") + "##PropertyBar";
    ImGui::Begin(windowName.c_str(), &s_propertyBarVisible, flags);
    
    // 等待添加实际属性
    
    // 监听属性栏宽度变化
    s_propertyBarWidth = ImGui::GetWindowSize().x;
    
    ImGui::End();
}

// 绘制文件栏
void Renderer::drawFileBar() {
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    
    // 计算文件栏位置和大小
    ImVec2 fileBarPos(0, s_menuBarHeight);
    ImVec2 fileBarSize(width*1.0f, 0); // 高度为0表示使用自适应高度
    
    // 绘制文件栏背景
    ImGui::SetNextWindowPos(fileBarPos);
    ImGui::SetNextWindowSize(fileBarSize);
    ImGui::SetNextWindowBgAlpha(0.9f);
    
    ImGuiWindowFlags tabWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
                                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav | 
                                     ImGuiWindowFlags_NoSavedSettings;
    // 在 Begin 之前 Push 样式变量
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 3.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20.0f, 4.0f));
    
    // 文件栏子窗口
    ImGui::Begin("FileBar", nullptr, tabWindowFlags);
    // 隐藏 TabBar 下方的分割线
    ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 0.0f);
    
    // 文件栏绘制在子窗口中
    ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs | 
                                   ImGuiTabBarFlags_TabListPopupButton | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | 
                                   ImGuiTabBarFlags_FittingPolicyScroll;
    if (ImGui::BeginTabBar("FileTabBar", tabBarFlags)) {
        // 待关闭的文件索引，-1表示无文件待关闭
        static std::size_t s_docIndexToBeClosed = DocManager::InvalidDocIndex;
        
        // 创建新文档
        if (ImGui::TabItemButton(" + ", ImGuiTabItemFlags_Trailing)) {
            // 新建文档并切换也需要取消当前命令
            CommandManager::getInstance().cancelCurrentCommand();
            // 创建新文档
            std::size_t newDocIndex = DocManager::createNewDocument();
            // 直接切换文档
            DocManager::setCurrentDocumentIndex(newDocIndex);
        }
        
        // 遍历文档列表，绘制每一个打开文档
        std::size_t documentCount = DocManager::getDocumentCount();
        // 需要处理来自命令的切换，如果命令中(典型场景是open命令)需要切换文档，则会延迟到此处处理，如果这一帧有命令中的待切换索引
        // 则不进行传统点击判定，而是通过ImGuiTabItemFlags_SetSelected标记去进行切换
        std::size_t pendingSwitchIndexFromCommand = DocManager::getPendingSwitchIndexFromCommand();
        
        for (std::size_t i = 0; i < documentCount; i++) {
            // 获取文件名
            std::string tabText = DocManager::getFileName(i) + StringUtils::format("##TabButton{}", i);
            
            // 设置标签项标志
            ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_None;
            // 未保存标志
            if (DocManager::isDocumentModified(i)) {
                tabItemFlags |= ImGuiTabItemFlags_UnsavedDocument;
            }
            
            bool tabOpen = true;
            
            // 处理来自命令的切换
            if (pendingSwitchIndexFromCommand != DocManager::InvalidDocIndex) {
                // 设置ImGuiTabItemFlags_SetSelected标记则调用BeginTabItem后会自动切换到这个tab
                if (i == pendingSwitchIndexFromCommand) {
                    tabItemFlags |= ImGuiTabItemFlags_SetSelected;
                    // 清除待切换索引
                    DocManager::setPendingSwitchIndexFromCommand(DocManager::InvalidDocIndex);
                }
                if (ImGui::BeginTabItem(tabText.c_str(), &tabOpen, tabItemFlags)) {
                    if (DocManager::getCurrentDocumentIndex() != i) {
                        // 这里不需要也不应该取消命令执行，因为不是从UI切换的，我们知道此时open命令已经进入结束状态
                        // 切换文档
                        DocManager::setCurrentDocumentIndex(i);
                        // 切换文档后，滚动命令行历史到最底部
                        s_bScrollCommandLineHistoryToBottom = true;
                    }
                    ImGui::EndTabItem();
                }
            }
            // 没有来自命令的切换，正常判定是否点击以切换文档
            else {
                if (ImGui::BeginTabItem(tabText.c_str(), &tabOpen, tabItemFlags)) {
                    // 直接切换文档
                    if (DocManager::getCurrentDocumentIndex() != i) {
                        // 切换文档前，取消当前命令执行
                        CommandManager::getInstance().cancelCurrentCommand();
                        // 切换文档
                        DocManager::setCurrentDocumentIndex(i);
                        // 切换文档后，滚动命令行历史到最底部
                        s_bScrollCommandLineHistoryToBottom = true;
                    }
                    ImGui::EndTabItem();
                }
            }
            
            // 添加工具提示
            if (ImGui::IsItemHovered()) {
                const std::string& fullFileName = DocManager::getFullFileName(i);
                std::string filePath = PathUtils::toString(DocManager::getFilePath(i));
                ImGui::SetTooltip(filePath.empty() ? fullFileName.c_str() : filePath.c_str());
            }
            
            // 处理标签关闭
            if (!tabOpen) {
                s_docIndexToBeClosed = i;
            }
        }
        // 循环内执行会破坏循环条件，循环完成后再执行关闭，关闭后当前文档会自动切换，不需要再去切换
        if (s_docIndexToBeClosed != DocManager::InvalidDocIndex) {
            CommandManager::getInstance().cancelCurrentCommand();
            DocManager::closeDocument(s_docIndexToBeClosed);
            s_docIndexToBeClosed = DocManager::InvalidDocIndex;
        }
        
        ImGui::EndTabBar();
    }
    // Pop TabBarBorderSize
    ImGui::PopStyleVar();
    
    // 更新文件栏高度
    s_fileBarHeight = ImGui::GetWindowSize().y;
    
    ImGui::End();
    // Pop WindowPadding 和 FramePadding
    ImGui::PopStyleVar(2);
}

// 获取变换管理器
TransformManager& Renderer::getTransformManager() {
    return DocManager::getCurrentDocument().getTransformManager();
}

// 绘制实时渲染信息窗口
void Renderer::drawRenderingInfoWindow() {
    if (!s_renderingInfoVisible) {
        return;
    }
    
    LocalizationManager& loc = LocalizationManager::getInstance();
    ImGui::Begin(loc.get("window.renderingInfo.title").c_str(), &s_renderingInfoVisible);
    
    // 获取视口信息
    int viewportLeft, viewportTop, viewportRight, viewportBottom;
    getTransformManager().getViewport().getViewport(viewportLeft, viewportTop, viewportRight, viewportBottom);
    
    // 计算视口大小
    int viewportWidth = viewportRight - viewportLeft;
    int viewportHeight = viewportBottom - viewportTop;
    
    // 获取相机信息
    const Camera& camera = getTransformManager().getCamera();
    glm::dvec3 cameraPos = camera.getPosition();
    glm::dvec3 cameraRot = camera.getRotation();
    double cameraScale = camera.getScale();
    
    // 计算视口的世界坐标系范围
    glm::dvec3 worldMin = getTransformManager().screenToWorld(glm::vec2(viewportLeft, viewportBottom));
    glm::dvec3 worldMax = getTransformManager().screenToWorld(glm::vec2(viewportRight, viewportTop));
    
    // 显示视口信息
    ImGui::Text(loc.get("window.renderingInfo.viewport.title").c_str());
    ImGui::Text("  %s: %d x %d", loc.get("window.renderingInfo.viewport.size").c_str(), viewportWidth, viewportHeight);
    ImGui::Text("  %s: Left=%d, Top=%d, Right=%d, Bottom=%d", loc.get("window.renderingInfo.viewport.bounds").c_str(), viewportLeft, viewportTop, viewportRight, viewportBottom);
    ImGui::Separator();
    
    // 显示世界坐标系信息
    ImGui::Text(loc.get("window.renderingInfo.world.title").c_str());
    ImGui::Text("  %s: (%.4f, %.4f, %.4f)", loc.get("window.renderingInfo.world.bottomLeft").c_str(), worldMin.x, worldMin.y, worldMin.z);
    ImGui::Text("  %s: (%.4f, %.4f, %.4f)", loc.get("window.renderingInfo.world.topRight").c_str(), worldMax.x, worldMax.y, worldMax.z);
    
    // 计算并显示世界坐标系宽高
    double worldWidth = worldMax.x - worldMin.x;
    double worldHeight = worldMax.y - worldMin.y;
    ImGui::Text("  %s: %.4f x %.4f", loc.get("window.renderingInfo.world.size").c_str(), worldWidth, worldHeight);
    
    ImGui::Separator();
    
    // 显示相机信息
    ImGui::Text(loc.get("window.renderingInfo.camera.title").c_str());
    // 根据缩放因子大小决定输出格式
    if (cameraScale < 0.01 || cameraScale >= 1e8) {
        ImGui::Text("  %s: %.8e", loc.get("window.renderingInfo.camera.zoom").c_str(), cameraScale);
    }
    else {
        ImGui::Text("  %s: %.8f", loc.get("window.renderingInfo.camera.zoom").c_str(), cameraScale);
    }
    ImGui::Text("  %s: (%.2f, %.2f, %.2f) degrees", loc.get("window.renderingInfo.camera.rotation").c_str(), cameraRot.x, cameraRot.y, cameraRot.z);
    ImGui::Text("  %s: (%.4f, %.4f, %.4f)", loc.get("window.renderingInfo.camera.position").c_str(), cameraPos.x, cameraPos.y, cameraPos.z);
    
    ImGui::Separator();
    
    // 显示光标信息
    ImGui::Text(loc.get("window.renderingInfo.cursor.title").c_str());
    glm::vec2 cursorScreenPos = InputHandler::getCursorPosition();
    glm::dvec3 cursorWorldPos = getTransformManager().screenToWorld(cursorScreenPos);
    ImGui::Text("  %s: (%.1f, %.1f)", loc.get("window.renderingInfo.cursor.screenPosition").c_str(), cursorScreenPos.x, cursorScreenPos.y);
    ImGui::Text("  %s: (%.4f, %.4f)", loc.get("window.renderingInfo.cursor.worldPosition").c_str(), cursorWorldPos.x, cursorWorldPos.y);
    
    ImGui::Separator();
    
    // 显示其他相关信息
    ImGui::Text(loc.get("window.renderingInfo.other.title").c_str());
    ImGui::Text("  %s: %d", loc.get("window.renderingInfo.other.frame").c_str(), ImGui::GetFrameCount());
    ImGui::Text("  %s: %.1f", loc.get("window.renderingInfo.other.fps").c_str(), ImGui::GetIO().Framerate);
    
    ImGui::End();
}

// 绘制模态对话框
void Renderer::drawModalDialogs() {
    // Options对话框现在由命令系统管理，不需要在这里绘制
}

// 绘制非模态窗口
void Renderer::drawNonModalWindows() {
    // 绘制Demo窗口
    if (s_demoWindowVisible) {
        ImGui::ShowDemoWindow(&s_demoWindowVisible);
    }
    
    // 绘制Metrics窗口
    if (s_metricsWindowVisible) {
        ImGui::ShowMetricsWindow(&s_metricsWindowVisible);
    }
    
    // 绘制实时渲染信息窗口
    drawRenderingInfoWindow();
    
    // 绘制输入上下文信息窗口
    InputContext::getInstance().drawInfoWindow();
    
    // 绘制光标测试窗口
    drawCursorTestWindow();
}

// 判断点是否在视口内
bool Renderer::isPointInViewport(const glm::vec2& screenPos) {
    // 获取视口边界
    int viewportLeft, viewportTop, viewportRight, viewportBottom;
    getTransformManager().getViewport().getViewport(viewportLeft, viewportTop, viewportRight, viewportBottom);
    
    // 计算视口大小
    int viewportWidth = viewportRight - viewportLeft;
    int viewportHeight = viewportBottom - viewportTop;
    
    // 检查屏幕坐标是否在视口范围内
    return !(screenPos.x < viewportLeft || screenPos.x > viewportRight || 
             screenPos.y < viewportTop || screenPos.y > viewportBottom || 
             viewportWidth <= 0 || viewportHeight <= 0);
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
    ImGui::Begin("CommandBar", nullptr, flags);
    // 记录ID
    s_commandBarId = ImGui::GetCurrentWindow()->RootWindow->ID;
    
    // 创建区域显示命令行历史，添加垂直和水平滚动条，留出空间给命令输入栏
    const float footerReserveHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("CommandLineHistory", ImVec2(0, -footerReserveHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    // TODO：使用ImGuiListClipper会导致滚动条的行为变得奇怪，无法自动滚动到末尾，暂不使用
    // // 使用静态的ImGuiListClipper来优化渲染，只绘制可见区域，提升历史条目过多时的性能
    // static ImGuiListClipper clipper;
    // float itemHeight = ImGui::GetTextLineHeight();
    // clipper.Begin(s_CommandLineHistory.size(), itemHeight);
    
    // while (clipper.Step()) {
    //     for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
    //         // 绘制每一条命令行历史
    //         ImGui::TextUnformatted(s_CommandLineHistory[i].c_str());
    //     }
    // }
    // clipper.End();
    
    const auto& cmdLineHistory = DocManager::getCurrentDocumentCommandLineHistory();
    for (std::size_t i = 0; i < cmdLineHistory.size(); i++)
    {
        ImGui::TextUnformatted(cmdLineHistory[i].c_str());
    }
    
    // 根据标志决定是否滚动到最后，在绘制项目之前执行
    if (!cmdLineHistory.empty() && s_bScrollCommandLineHistoryToBottom) {
        ImGui::SetScrollHereY(1.0f);
        s_bScrollCommandLineHistoryToBottom = false;
    }
    ImGui::EndChild();
    // 命令输入栏部分
    ImGui::Separator();
    
    // 调整布局：Command提示在左边，上下居中，输入框占满剩余空间
    ImGui::AlignTextToFramePadding();
    
    // 显示命令提示信息
    auto& inputContext = InputContext::getInstance();
    // 命令或者选择任务中需要显示提示信息
    if (inputContext.isAnyCommandOrTaskRunning()) {
        std::string commandName = CommandManager::getInstance().getRunningCommandName();
        const std::string& prompt = inputContext.getPrompt();
        // 如果在命令中还需要显示当前命令名称
        if (inputContext.isInCommandExecution()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s: %s", commandName.c_str(), prompt.c_str());
        }
        else if (!prompt.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", prompt.c_str());
        }
        else {
            ImGui::Text(loc.get("commandLine.prompt.command").c_str());
        }
    }
    else {
        ImGui::Text(loc.get("commandLine.prompt.command").c_str());
    }
    
    ImGui::SameLine();
    
    // 如果需要设置焦点到命令输入框
    if (s_bShouldFocusOnCommandInput) {
        ImGui::SetKeyboardFocusHere(0);
        s_bShouldFocusOnCommandInput = false;
        
        // 命令补全: 重新获得焦点，根据当前输入框中内容重建补全列表
        if (!InputContext::getInstance().isAnyCommandOrTaskRunning()) {
            std::string currentInput(s_cmdBuffer.data());
            if (currentInput != s_userInputCommand) {
               s_userInputCommand = currentInput;
               s_completionCandidates = CommandManager::getInstance().getCompletionCandidates(currentInput);
               s_completionSelectedIndex = -1;
            }
        }
    }
    
    // 使用PushItemWidth使输入框占满剩余空间
    ImGui::PushItemWidth(-1);
    // 检查InputContext的特殊按键事件
    SpecialKeyEventType inputEvent = inputContext.getLastSpecialKeyEvent();
    // Enter/Space 提交输入框输入到输入上下文中进行处理
    if (inputEvent == SpecialKeyEventType::kEnterPressed || inputEvent == SpecialKeyEventType::kSpacePressed) {
        // 获取当前 InputText 中的实际内容并清空缓冲区
        std::string input = getAndClearCommandBuffer();
        
        // 命令补全
        if (!InputContext::getInstance().isAnyCommandOrTaskRunning()) {
            // 执行时不是直接执行命令，而是去补全列表找到第一项来执行
            if (!s_completionCandidates.empty()) {
                input = s_completionCandidates[0].fullName;
            }
            
            // 清除补全相关状态
            s_completionCandidates.clear();
            s_completionSelectedIndex = -1;
            s_userInputCommand.clear();
        }
        
        // 处理输入
        inputContext.handleEnterSpace(input);
        // ImGui会内部维护InputText的缓冲区副本，Enter、Esc等事件时由上面的ImGui::SetKeyboardFocusHere所控制焦点会一直维持在命令输入框上，
        // 此时光清空外部缓冲区的话，每次InpuText调用都会把内部的副本重新同步回外部缓冲区来，那么就必须通过文本处理回调函数来清空内部的副本。
        // 而如果焦点已经不在输入框上了，那么单纯清除外部缓冲区就足够了。
        s_bNeedClearCommandBufferInternalCopy = true;
        // 清除特殊按键事件
        inputContext.clearSpecialKeyEvent();
    }
    // Esc 同样提交输入到输入上下文进行处理
    else if (inputEvent == SpecialKeyEventType::kEscPressed) {
        
        // 命令补全
        if (!InputContext::getInstance().isAnyCommandOrTaskRunning()) {
            // 清除补全相关状态
            s_completionCandidates.clear();
            s_completionSelectedIndex = -1;
            s_userInputCommand.clear();
        }
        
        // 如果在命令历史导航模式，Esc会退出导航模式，清空输入并且什么也不输出
        if (isInCommandHistoryNavigationMode()) {
            exitCommandHistoryNavigationMode();
            getAndClearCommandBuffer();
        }
        // 否则就正常处理输入并清空缓冲区
        else {
            inputContext.handleEscape(getAndClearCommandBuffer());
        }
        
        // 同理清除内部副本
        s_bNeedClearCommandBufferInternalCopy = true;
        // 清除特殊按键事件
        inputContext.clearSpecialKeyEvent();
    }
    // Up/Down 键：命令历史导航
    else if (inputEvent == SpecialKeyEventType::kUpPressed || inputEvent == SpecialKeyEventType::kDownPressed) {
        // 只有不在命令执行或任务中时才处理
        if (!inputContext.isAnyCommandOrTaskRunning()) {
            // 在命令历史中进行导航并获取对应的命令
            bool isUp = (inputEvent == SpecialKeyEventType::kUpPressed);
            std::string command = navigateCommandHistoryAndGetExpectedCommand(isUp);
            
            // 修改外部缓冲区
            if (!command.empty()) {
                std::fill(s_cmdBuffer.begin(), s_cmdBuffer.end(), 0);
                for (size_t i = 0; i < command.size() && i < s_cmdBuffer.size() - 1; ++i) {
                    s_cmdBuffer[i] = command[i];
                }
                s_bCommandBufferModified = true;
            }
            // 退出了命令历史导航模式，清空输入
            else {
                getAndClearCommandBuffer();
            }
        }
        // 清除特殊按键事件
        inputContext.clearSpecialKeyEvent();
    }
    
    // 回调函数处理文本选择问题、字符过滤、清除缓冲区与命令补全
    auto inputTextCallback = [](ImGuiInputTextCallbackData* data) -> int {
        // 1. 字符过滤逻辑(只有输入字符时触发)
        if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
            if (data->EventChar > 127) {
                return 1; // 丢弃非 ASCII 字符
            }
            // Space执行命令，这里直接丢弃，InputHandler中已经将按键事件转发给InputContext
            else if (data->EventChar == ' ')
            {
                return 1;
            }
        }
        
        // 2. 命令历史导航，Up/Down按下触发
        if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
            // 只有不在命令执行或任务中时才进行导航
            if (!InputContext::getInstance().isAnyCommandOrTaskRunning()) {
                // 检查 EventKey 来判断是 Up 还是 Down
                if (data->EventKey == ImGuiKey_UpArrow || data->EventKey == ImGuiKey_DownArrow) {
                    std::string command = navigateCommandHistoryAndGetExpectedCommand(data->EventKey == ImGuiKey_UpArrow);
                    
                    // 修改内部缓冲区
                    if (!command.empty()) {
                        data->DeleteChars(0, data->BufTextLen);
                        data->InsertChars(0, command.c_str());
                        data->CursorPos = data->BufTextLen;
                    }
                    // 退出了命令历史导航模式，清空缓冲区
                    if (command.empty() || !isInCommandHistoryNavigationMode()) {
                        data->DeleteChars(0, data->BufTextLen);
                    }
                }
            }
        }
        
        // 3. 文本编辑回调 - 重构补全候选列表 + 退出导航模式
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            // 如果在导航模式，任何编辑都会退出导航模式
            if (isInCommandHistoryNavigationMode()) {
                exitCommandHistoryNavigationMode();
            }
            
            // 命令补全
            if (!InputContext::getInstance().isAnyCommandOrTaskRunning()) {
                std::string currentInput(data->Buf, data->BufTextLen);
                
                // 输入变化则更新候选列表
                if (currentInput != s_userInputCommand) {
                    s_userInputCommand = currentInput;
                    s_completionCandidates = CommandManager::getInstance().getCompletionCandidates(currentInput);
                    s_completionSelectedIndex = -1;
                }
            }
        }
        
        // 4. 补全回调 - Tab 补全
        if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion &&
            !InputContext::getInstance().isAnyCommandOrTaskRunning()) {
            if (!s_completionCandidates.empty()) {
                // 循环切换到下一个候选项，-1没选中状态则变为选中第一个
                s_completionSelectedIndex = (s_completionSelectedIndex + 1) % s_completionCandidates.size();
                
                const auto& item = s_completionCandidates[s_completionSelectedIndex];
                
                // 获取输入的字符串
                std::string userInput = StringUtils::toUpperCase(s_userInputCommand);
                std::string completionCommand;
                
                // 输入是命令全名的前缀，那么补全为全名
                if (item.fullName.compare(0, userInput.size(), userInput) == 0) {
                    completionCommand = item.fullName;
                }
                // 虽然大部分别名都是命令全名的一个前缀，但有可能不是，此时输入如果是别名的前缀，那么将补全为别名而非全名
                else if (item.key.compare(0, userInput.size(), userInput) == 0) {
                    completionCommand = item.key;
                }
                // 既不是别名前缀也不是全名前缀，那么可能是新实现的模糊匹配之类，直接补全为全名
                else {
                    completionCommand = item.fullName;
                }
                
                // 清除当前内容，插入补全内容
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, completionCommand.c_str());
                
                // 将通过补全填入的字符置于选中状态，同时光标置于末尾，方便用户通过一次Backspace就轻松删除
                data->SelectionStart = static_cast<int>(userInput.size());
                data->SelectionEnd = data->BufTextLen;
                data->CursorPos = data->BufTextLen;
            }
        }
        
        // 5. Always 回调 - 处理缓冲区修改标志、补充清空补全信息
        if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
            // 命令输入缓冲区被修改，那么就解除选中并移动光标到末尾
            if (s_bCommandBufferModified) {
                // 直接修改外部缓冲区后，多出来的字符在下一帧可能处于选中状态，所以需要取消其选中状态
                data->SelectionStart = data->SelectionEnd = data->BufTextLen;
                // 焦点丢失后，其他位置的输入总是追加到末尾，所以总是移动光标到末尾，不管焦点丢失前光标在什么位置
                data->CursorPos = data->BufTextLen;
                // 重置标记
                s_bCommandBufferModified = false;
            }
            // 需要清除命令输入缓冲区的内部副本
            if (s_bNeedClearCommandBufferInternalCopy) {
                data->DeleteChars(0, data->BufTextLen); // 强制抹除 ImGui 内部的副本
                data->CursorPos = 0;
                // 重置标记
                s_bNeedClearCommandBufferInternalCopy = false;
            }
            
            // 命令补全
            // 某些情况下会无法清空补全信息(极少数corner case)，导致输入清空了还会显示补全框，这里做一个补充清空
            if (!InputContext::getInstance().isAnyCommandOrTaskRunning()) {
                // 如果输入已经空了，那么无条件清空补全信息
                if (data->BufTextLen == 0) {
                    s_userInputCommand.clear();
                    s_completionCandidates.clear();
                    s_completionSelectedIndex = -1;
                }
            }
        }
        return 0;
    };
    
    // 命令输入框
    ImGui::InputTextWithHint("##CommandInput", loc.get("commandLine.inputPrompt").c_str(), s_cmdBuffer.data(), s_cmdBuffer.size(), 
        ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackCompletion, 
        inputTextCallback, nullptr);
    
    // 获取刚刚渲染的InputText控件的ID，在检测焦点是否在CommandInput上时使用，每一帧记录确保不会失效
    s_commandInputId = ImGui::GetItemID(); 
    
    // 命令补全候选框
    // 条件：没有命令或者任务执行时也就是输入的是命令、且补全列表有东西且输入框有焦点才绘制
    //      另外命令历史导航模式下也不进行补全
    if (!InputContext::getInstance().isAnyCommandOrTaskRunning() &&
        !s_completionCandidates.empty() && ImGui::IsItemFocused() &&
        !isInCommandHistoryNavigationMode()) {
        // 首次计算候选框宽度（40个字符宽度）
        static float completionPopupWidth = ImGui::CalcTextSize("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijkl").x;
        
        ImVec2 inputMin = ImGui::GetItemRectMin();
        float itemHeight = ImGui::GetTextLineHeightWithSpacing();
        float popupHeight = s_completionCandidates.size() * itemHeight + ImGui::GetStyle().WindowPadding.y * 2;
        ImVec2 popupPos(inputMin.x, inputMin.y - popupHeight - 2);
        
        ImGui::SetNextWindowPos(popupPos);
        ImGui::SetNextWindowSize(ImVec2(completionPopupWidth, popupHeight));
        ImGui::SetNextWindowBgAlpha(0.95f);
        
        // 补全窗口
        ImGui::Begin("##CompletionPopup", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
            
        bool bItemClicked = false;
        std::string commandToBeExecuted;
        for (std::size_t i = 0; i < s_completionCandidates.size(); i++) {
            const auto& item = s_completionCandidates[i];
            // 别名显示 key (fullName)，全称直接显示
            std::string displayText = item.isAlias ? (item.key + " (" + item.fullName + ")") : item.key;
            
            // 还没有通过Tab来选择补全项，那么以一种不同的颜色高亮第一项，表示现在直接回车将默认执行的命令
            if (s_completionSelectedIndex == -1 && i == 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                ImGui::Selectable(displayText.c_str(), false);
                ImGui::PopStyleColor();
            }
            // 通过Tab选中的当前补全项
            else if (i == s_completionSelectedIndex) {
                ImGui::Selectable(displayText.c_str(), true);
            }
            // 未选中
            else {
                ImGui::Selectable(displayText.c_str(), false);
            }
            
            // 通过鼠标点击
            if (ImGui::IsItemClicked()) {
                bItemClicked = true;
                commandToBeExecuted = s_completionCandidates[i].fullName;
            }
        }
        // 鼠标点击补全项则直接执行命令
        if (bItemClicked) {
            // 清空命令缓冲区
            getAndClearCommandBuffer();
            // 清除补全相关状态
            s_completionCandidates.clear();
            s_completionSelectedIndex = -1;
            s_userInputCommand.clear();
            // 采用通用流程处理，将命令全名发送个InputContext执行
            inputContext.handleEnterSpace(commandToBeExecuted);
        }
        ImGui::End();
    }
    
    // 平衡PushItemWidth调用
    ImGui::PopItemWidth();
    
    ImGui::End();

    
    // 绘制拖动条，允许调整命令栏高度
    static bool isResizing = false;
    static float resizeStartY = 0.0f;
    static float resizeStartHeight = 0.0f;
    float uiScale = Utils::getUIScaleFactor();
    
    // 减小拖动条高度，使其更美观
    float resizeBarHeight = 2.0f * uiScale;
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
        
        // 限制拖动的高度范围
        float minHeight = 150.0f;
        float maxHeight = 1080.0f * uiScale;
        if (newHeight > minHeight && newHeight < maxHeight) {
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
void Renderer::addContentToCommandLineHistory(const std::string& content) {
    DocManager::addToCurrentDocumentCommandLineHistory(content);
    // 设置滚动标志为true，确保新命令添加后会滚动到最底部
    s_bScrollCommandLineHistoryToBottom = true;
}

// 设置是否应该将焦点设置到命令输入框
void Renderer::setShouldFocusOnCommandInput(bool shouldFocus) {
    s_bShouldFocusOnCommandInput = shouldFocus;
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

// 获取输入缓冲区内容，并清空输入缓冲区
std::string Renderer::getAndClearCommandBuffer()
{
    std::string buffer = s_cmdBuffer.data();
    std::fill(s_cmdBuffer.begin(), s_cmdBuffer.end(), 0);
    // 如果焦点就在命令输入框上，那么还需要同时设置标记以便在字符回调中清空内部缓冲区
    // 如果焦点不在，则不能设置，因为不会进入回调，设置了反而会导致下一次得到焦点时在绘图区输入的第一个字符被吞掉
    if (focusIsOnCommandInput()) {
        s_bNeedClearCommandBufferInternalCopy = true;
    }
    
    // 清空缓冲区时，同时退出命令历史导航模式，这在内部调用时或许不必要，但通过cancelCurrentCommand调用则必须有
    exitCommandHistoryNavigationMode();
    
    return buffer;
}

// 检查是否在命令历史导航模式
bool Renderer::isInCommandHistoryNavigationMode() {
    return s_commandHistoryNavigationIndex >= 0;
}

// 在命令历史中进行导航，返回应该填充的命令，为空则表示已经退出了命令历史导航模式(比如命令历史是空的，当前已经是最新一条然后按下Down)
std::string Renderer::navigateCommandHistoryAndGetExpectedCommand(bool isUp) {
    auto& doc = DocManager::getCurrentDocument();
    auto& history = doc.getCommandExecutionHistory();
    
    if (history.empty()) {
        return "";
    }
    
    if (isUp) {
        if (s_commandHistoryNavigationIndex == -1) {
            s_commandHistoryNavigationIndex = static_cast<int>(history.size()) - 1;
        }
        else if (s_commandHistoryNavigationIndex > 0) {
            s_commandHistoryNavigationIndex--;
        }
    }
    else {
        if (s_commandHistoryNavigationIndex == -1) {
            return "";
        }
        
        if (s_commandHistoryNavigationIndex < static_cast<int>(history.size()) - 1) {
            s_commandHistoryNavigationIndex++;
        }
        else {
            s_commandHistoryNavigationIndex = -1;
            return "";
        }
    }
    
    if (s_commandHistoryNavigationIndex >= 0 && s_commandHistoryNavigationIndex < static_cast<int>(history.size())) {
        return history[s_commandHistoryNavigationIndex];
    }
    
    return "";
}

// 退出命令历史导航模式
void Renderer::exitCommandHistoryNavigationMode() {
    s_commandHistoryNavigationIndex = -1;
}

// 获取属性栏是否可见
bool Renderer::isPropertyBarVisible() {
    return s_propertyBarVisible;
}

// 设置属性栏是否可见
void Renderer::setPropertyBarVisible(bool visible) {
    s_propertyBarVisible = visible;
}

// 检查焦点是否位于指定窗口或其子窗口
bool Renderer::focusIsOnWindow(const std::string& windowName) {
    if (ImGuiWindow* targetWindow = ImGui::FindWindowByName(windowName.c_str())) {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        if (ctx && ctx->NavWindow && ctx->NavWindow->RootWindow == targetWindow) {
            return true;
        }
    }
    return false;
}

// 检查焦点是否位于命令栏
bool Renderer::focusIsOnCommandBar() {
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    return ctx && ctx->NavWindow && ctx->NavWindow->RootWindow->ID == s_commandBarId;
}

// 检查焦点是否在命令输入框上
bool Renderer::focusIsOnCommandInput() {
    /*
    旧的通用实现，能够实现但相比保存ID并直接比较会多一些按字符串查找操作(内部也是哈希之后比较的，对性能并不影响)
    
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
    */
    return s_commandInputId == ImGui::GetActiveID();
}

} // namespace tch