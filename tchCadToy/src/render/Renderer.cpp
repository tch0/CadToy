#include "render/Renderer.h"
#include "document/DocManager.h"
#include "Layer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "sys/Global.h"
#include "debug/Logger.h"
#include "utils/LocalizationManager.h"
#include "command/CommandManager.h"
#include "input/InputHandler.h"
#include "input/InputContext.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <memory>

namespace tch {

// 静态成员初始化
bool Renderer::s_initialized = false;
GLFWwindow* Renderer::s_window = nullptr;
float Renderer::s_crossCursorSize = 50.0f;
float Renderer::s_pickBoxSize = 5.0f;      // 拾取框大小，默认值为5
Renderer::CursorMode Renderer::s_currentCursorMode = Renderer::CursorMode::kDefault;
Renderer::CursorMarker Renderer::s_currentCursorMarker = Renderer::CursorMarker::kNone;
bool Renderer::s_cursorTestWindowVisible = false;

// 栅格和坐标轴颜色初始化
float Renderer::s_mainGridColor[3] = {54.0f/255.0f, 61.0f/255.0f, 78.0f/255.0f}; // 主栅格颜色 RGB: 54,61,78
float Renderer::s_subGridColor[3] = {38.0f/255.0f, 45.0f/255.0f, 55.0f/255.0f};  // 子栅格颜色 RGB: 38,45,55
float Renderer::s_xAxisColor[3] = {97.0f/255.0f, 37.0f/255.0f, 39.0f/255.0f};    // X轴颜色 RGB: 97,37,39
float Renderer::s_yAxisColor[3] = {34.0f/255.0f, 89.0f/255.0f, 41.0f/255.0f};    // Y轴颜色 RGB: 34,89,41

// 当前光标位置的世界坐标
glm::dvec3 Renderer::s_cursorPosWorld = glm::dvec3(0.0, 0.0, 0.0);

// UI组件高度
float Renderer::s_menuBarHeight = 30.0f;              // 菜单栏高度
float Renderer::s_fileBarHeight = 30.0f;              // 文件栏高度
float Renderer::s_statusBarHeight = 35.0f;            // 状态栏高度

// 命令历史滚动控制
bool Renderer::s_bScrollCommandHistoryToBottom = false; // 是否应该将命令历史滚动到底部
// 命令输入框焦点控制
bool Renderer::s_bShouldFocusOnCommandInput = false; // 是否应该将焦点设置到命令输入框
// 命令输入缓冲区是否被修改，通过非命令输入栏的字符输入或者退格
bool Renderer::s_bCommandBufferModified = false;

// 是否需要清除ImGui的命令输入缓冲区内部副本
bool Renderer::s_bNeedClearCommandBufferInternalCopy = false;

// 命令栏相关
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

// 实时渲染信息窗口相关
static bool s_renderingInfoVisible = false;  // 实时渲染信息窗口是否可见




// 初始化渲染器
void Renderer::initialize(GLFWwindow* window) {
    s_window = window;
    s_initialized = true;
    
    // 设置默认背景颜色为RGB: 33,40,48
    setBackgroundColor(33.0f/255.0f, 40.0f/255.0f, 48.0f/255.0f, 1.0f);
    
    // 获取窗口尺寸并设置视口
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    
    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 初始化ImGui
    initializeImGui();
    
    // 初始化本地化管理器
    LocalizationManager::getInstance().initialize();
    
    // 初始化文件管理器
    DocManager::initialize();
}

// 清理渲染器
void Renderer::cleanup() {
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
            bool bFocusIsOnCommandBar = focusIsOnWindow("CommandBar");
            
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

// 设置背景颜色
void Renderer::setBackgroundColor(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
}

// 绘制所有图形
void Renderer::drawAll() {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 绘制栅格
    drawGrid();
    
    // 绘制XY轴
    drawAxes();
    
    // 绘制所有图层
    LayerManager::getInstance().draw();
    
    // TODO: 临时措施，还未实现图形引擎，先简单绘制活动命令的预览
    if (CommandManager::getInstance().hasActiveCommand()) {
        auto activeCommand = CommandManager::getInstance().getActiveCommand();
        if (activeCommand) {
            activeCommand->drawPreview();
        }
    }
}

// 绘制光标
void Renderer::drawCursor() {
    if (!s_initialized || !s_window) {
        return;
    }
    
    // 计算光标在屏幕上的位置
    glm::vec2 cursorScreenPos = InputHandler::getCursorPosition();
    
    // 更新当前光标位置（使用变换管理器转换）
    s_cursorPosWorld = getTransformManager().screenToWorld(cursorScreenPos);
    
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
    
    // 根据光标模式绘制不同的光标
    switch (s_currentCursorMode) {
        case CursorMode::kDefault:
        case CursorMode::kCrosshair: {
            // 统一处理：kCrosshair模式等价于pickbox=0
            float effectivePickBoxSize = (s_currentCursorMode == CursorMode::kCrosshair) ? 0.0f : s_pickBoxSize;
            // 绘制拾取框（如果effectivePickBoxSize > 0）
            drawPickBox(cursorScreenPos, effectivePickBoxSize);
            // 绘制十字线，根据effectivePickBoxSize决定绘制方式
            drawCrosshair(cursorScreenPos, effectivePickBoxSize);
            break;
        }
        case CursorMode::kPickbox:
            // 仅绘制拾取框
            drawPickBox(cursorScreenPos, s_pickBoxSize);
            break;
            
        case CursorMode::kPanning:
            // 绘制手掌形状
            drawHandCursor(cursorScreenPos);
            break;
    }
    
    // 绘制光标标记
    drawCursorMarker(cursorScreenPos);
    
    // 恢复矩阵状态
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // 重新启用深度测试
    glEnable(GL_DEPTH_TEST);
}

// 绘制拾取框
void Renderer::drawPickBox(const glm::vec2& pos, float pickBoxSize) {
    if (pickBoxSize > 0) {
        glBegin(GL_LINE_LOOP);
        glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
        float halfSize = pickBoxSize;
        // 考虑像素宽度，添加0.5偏移以确保线对齐像素中心
        glVertex2f(pos.x - halfSize + 0.5f, pos.y - halfSize + 0.5f);
        glVertex2f(pos.x + halfSize + 0.5f, pos.y - halfSize + 0.5f);
        glVertex2f(pos.x + halfSize + 0.5f, pos.y + halfSize + 0.5f);
        glVertex2f(pos.x - halfSize + 0.5f, pos.y + halfSize + 0.5f);
        glEnd();
    }
}

// 绘制十字线
void Renderer::drawCrosshair(const glm::vec2& pos, float pickBoxSize) {
    if (s_crossCursorSize > 0) {
        float lineLength = s_crossCursorSize;
        if (lineLength > 0) {
            // 考虑像素宽度，添加0.5偏移以确保线对齐像素中心
            float startOffset = pickBoxSize;
            float outerLength = lineLength - startOffset;
            
            // 只有当有足够空间绘制十字线时才绘制
            if (outerLength > 0) {
                glBegin(GL_LINES);
                glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
                
                // 统一计算每个方向的起点和终点
                float topStart = pos.y - startOffset + 0.5f;
                float topEnd = pos.y - lineLength + 0.5f;
                float bottomStart = pos.y + startOffset + 0.5f;
                float bottomEnd = pos.y + lineLength + 0.5f;
                float leftStart = pos.x - startOffset + 0.5f;
                float leftEnd = pos.x - lineLength + 0.5f;
                float rightStart = pos.x + startOffset + 0.5f;
                float rightEnd = pos.x + lineLength + 0.5f;
                
                // 上边
                glVertex2f(pos.x + 0.5f, topStart);
                glVertex2f(pos.x + 0.5f, topEnd);
                
                // 下边
                glVertex2f(pos.x + 0.5f, bottomStart);
                glVertex2f(pos.x + 0.5f, bottomEnd);
                
                // 左边
                glVertex2f(leftStart, pos.y + 0.5f);
                glVertex2f(leftEnd, pos.y + 0.5f);
                
                // 右边
                glVertex2f(rightStart, pos.y + 0.5f);
                glVertex2f(rightEnd, pos.y + 0.5f);
                
                glEnd();
            }
        }
    }
}

// 绘制手掌光标
void Renderer::drawHandCursor(const glm::vec2& pos) {
    // 手掌大小（固定大小，不随拾取框变化）
    float handSize = 25.0f;
    
    // 绘制手掌轮廓
    glBegin(GL_LINE_LOOP);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
    
    // 手掌底部
    glVertex2f(pos.x - handSize * 0.4f + 0.5f, pos.y + handSize * 0.2f + 0.5f);
    glVertex2f(pos.x + handSize * 0.4f + 0.5f, pos.y + handSize * 0.2f + 0.5f);
    
    // 手掌右侧
    glVertex2f(pos.x + handSize * 0.4f + 0.5f, pos.y + handSize * 0.2f + 0.5f);
    glVertex2f(pos.x + handSize * 0.3f + 0.5f, pos.y - handSize * 0.3f + 0.5f);
    
    // 手掌顶部
    glVertex2f(pos.x + handSize * 0.3f + 0.5f, pos.y - handSize * 0.3f + 0.5f);
    glVertex2f(pos.x + handSize * 0.1f + 0.5f, pos.y - handSize * 0.4f + 0.5f);
    glVertex2f(pos.x + 0.5f, pos.y - handSize * 0.45f + 0.5f);
    glVertex2f(pos.x - handSize * 0.1f + 0.5f, pos.y - handSize * 0.4f + 0.5f);
    glVertex2f(pos.x - handSize * 0.3f + 0.5f, pos.y - handSize * 0.3f + 0.5f);
    
    // 手掌左侧
    glVertex2f(pos.x - handSize * 0.3f + 0.5f, pos.y - handSize * 0.3f + 0.5f);
    glVertex2f(pos.x - handSize * 0.4f + 0.5f, pos.y + handSize * 0.2f + 0.5f);
    
    glEnd();
    
    // 绘制手指
    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
    
    // 拇指
    glVertex2f(pos.x - handSize * 0.25f + 0.5f, pos.y + handSize * 0.1f + 0.5f);
    glVertex2f(pos.x - handSize * 0.35f + 0.5f, pos.y + handSize * 0.15f + 0.5f);
    
    // 食指
    glVertex2f(pos.x + handSize * 0.25f + 0.5f, pos.y - handSize * 0.1f + 0.5f);
    glVertex2f(pos.x + handSize * 0.3f + 0.5f, pos.y - handSize * 0.35f + 0.5f);
    
    // 中指
    glVertex2f(pos.x + handSize * 0.08f + 0.5f, pos.y - handSize * 0.15f + 0.5f);
    glVertex2f(pos.x + handSize * 0.12f + 0.5f, pos.y - handSize * 0.4f + 0.5f);
    
    // 无名指
    glVertex2f(pos.x - handSize * 0.08f + 0.5f, pos.y - handSize * 0.15f + 0.5f);
    glVertex2f(pos.x - handSize * 0.04f + 0.5f, pos.y - handSize * 0.4f + 0.5f);
    
    // 小指
    glVertex2f(pos.x - handSize * 0.25f + 0.5f, pos.y - handSize * 0.1f + 0.5f);
    glVertex2f(pos.x - handSize * 0.2f + 0.5f, pos.y - handSize * 0.35f + 0.5f);
    
    glEnd();
}

// 绘制向左框选标记（参照示例，2白2透明2白2透明2白的模式）
void Renderer::drawLeftSelectMarker(const glm::vec2& pos) {
    float boxSize = 10.0f; // 整体大小
    float squareSize = 5.0f; // 小正方形大小
    
    // 绘制虚线框（2白2透明2白2透明2白模式的虚线）
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xCCCC); // 1100110011001100 模式，即2白2透明2白2透明2白
    
    glBegin(GL_LINE_LOOP);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色框
    // 小框左上角
    glVertex2f(pos.x - boxSize * 0.5f + 0.5f, pos.y - boxSize * 0.5f + 0.5f);
    // 小框右上角
    glVertex2f(pos.x + boxSize * 0.5f + 0.5f, pos.y - boxSize * 0.5f + 0.5f);
    // 小框右下角
    glVertex2f(pos.x + boxSize * 0.5f + 0.5f, pos.y + boxSize * 0.5f + 0.5f);
    // 小框左下角
    glVertex2f(pos.x - boxSize * 0.5f + 0.5f, pos.y + boxSize * 0.5f + 0.5f);
    glEnd();
    
    glDisable(GL_LINE_STIPPLE);
    
    // 绘制实心正方形（位于正方形左侧边上）
    glBegin(GL_QUADS);
    glColor3f(91.0f/255.0f, 201.0f/255.0f, 189.0f/255.0f); // 绿色 rgb(91,201,189)
    // 正方形左上角
    glVertex2f(pos.x - boxSize * 0.5f - squareSize + 2.0f + 0.5f, pos.y - squareSize * 0.5f + 0.5f);
    // 正方形右上角
    glVertex2f(pos.x - boxSize * 0.5f + 2.0f + 0.5f, pos.y - squareSize * 0.5f + 0.5f);
    // 正方形右下角
    glVertex2f(pos.x - boxSize * 0.5f + 2.0f + 0.5f, pos.y + squareSize * 0.5f + 0.5f);
    // 正方形左下角
    glVertex2f(pos.x - boxSize * 0.5f - squareSize + 2.0f + 0.5f, pos.y + squareSize * 0.5f + 0.5f);
    glEnd();
}

// 绘制向右框选标记
void Renderer::drawRightSelectMarker(const glm::vec2& pos) {
    float boxSize = 10.0f; // 整体大小
    float innerSquareSize = 5.0f; // 中间正方形大小
    
    // 绘制白色实线框
    glBegin(GL_LINE_LOOP);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色
    // 框左上角
    glVertex2f(pos.x - boxSize * 0.5f + 0.5f, pos.y - boxSize * 0.5f + 0.5f);
    // 框右上角
    glVertex2f(pos.x + boxSize * 0.5f + 0.5f, pos.y - boxSize * 0.5f + 0.5f);
    // 框右下角
    glVertex2f(pos.x + boxSize * 0.5f + 0.5f, pos.y + boxSize * 0.5f + 0.5f);
    // 框左下角
    glVertex2f(pos.x - boxSize * 0.5f + 0.5f, pos.y + boxSize * 0.5f + 0.5f);
    glEnd();
    
    // 绘制实心正方形（位于框中间）
    glBegin(GL_QUADS);
    glColor3f(56.0f/255.0f, 171.0f/255.0f, 223.0f/255.0f); // 蓝色 rgb(56,171,223)
    // 正方形左上角
    glVertex2f(pos.x - innerSquareSize * 0.5f + 0.5f, pos.y - innerSquareSize * 0.5f + 0.5f);
    // 正方形右上角
    glVertex2f(pos.x + innerSquareSize * 0.5f + 0.5f, pos.y - innerSquareSize * 0.5f + 0.5f);
    // 正方形右下角
    glVertex2f(pos.x + innerSquareSize * 0.5f + 0.5f, pos.y + innerSquareSize * 0.5f + 0.5f);
    // 正方形左下角
    glVertex2f(pos.x - innerSquareSize * 0.5f + 0.5f, pos.y + innerSquareSize * 0.5f + 0.5f);
    glEnd();
}

// 绘制锁标记
void Renderer::drawLockMarker(const glm::vec2& pos) {
    // 长方形尺寸：宽度10，高度6
    float rectWidth = 10.0f;
    float rectHeight = 6.0f;
    
    // 计算长方形位置
    float rectX = pos.x - rectWidth * 0.5f;
    float rectY = pos.y - rectHeight * 0.5f;
    
    // 锁柱高度
    float lockPostHeight = 5.0f;
    
    // 锁柱起始位置
    float leftPostX = rectX + 2.0f; // 左侧锁柱位置
    float rightPostX = rectX + rectWidth - 1.0f; // 右侧锁柱位置
    
    // 绘制下方长方形
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色
    // 长方形左上角
    glVertex2f(rectX, rectY);
    // 长方形右上角
    glVertex2f(rectX + rectWidth, rectY);
    // 长方形右下角
    glVertex2f(rectX + rectWidth, rectY + rectHeight);
    // 长方形左下角
    glVertex2f(rectX, rectY + rectHeight);
    glEnd();
    
    // 绘制锁柱，三根线段
    glBegin(GL_LINE_STRIP);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色
    
    // 左侧
    glVertex2f(leftPostX, rectY);
    glVertex2f(leftPostX, rectY - lockPostHeight);
    
    // 顶部
    glVertex2f(leftPostX, rectY - lockPostHeight);
    glVertex2f(rightPostX, rectY - lockPostHeight);
    
    // 右侧
    glVertex2f(rightPostX, rectY - lockPostHeight);
    glVertex2f(rightPostX, rectY);
    
    glEnd();
}

// 绘制加选和减选标记
void Renderer::drawSelectMarker(const glm::vec2& pos, bool isAdd) {
    float lineLength = 8.0f; // 线段长度
    float lineWidth = 2.0f;  // 线段宽度
    
    // 设置颜色
    if (isAdd) {
        glColor3f(0.0f, 1.0f, 0.0f); // 绿色
    } else {
        glColor3f(1.0f, 0.0f, 0.0f); // 红色
    }
    
    // 绘制线段
    glLineWidth(lineWidth);
    glBegin(GL_LINES);
    
    // 绘制水平线段
    glVertex2f(pos.x - lineLength * 0.5f, pos.y);
    glVertex2f(pos.x + lineLength * 0.5f, pos.y);
    
    // 如果是+号，绘制垂直线段
    if (isAdd) {
        glVertex2f(pos.x, pos.y - lineLength * 0.5f);
        glVertex2f(pos.x, pos.y + lineLength * 0.5f);
    }
    
    glEnd();
    glLineWidth(1.0f); // 恢复默认线宽
}

// 绘制正交标记
void Renderer::drawOrthogonalMarker(const glm::vec2& pos) {
    float lineLength = 8.0f; // 水平线段长度
    float verticalLineLength = 6.0f; // 垂直线段长度
    float lineWidth = 2.0f;  // 线段宽度
    
    // 设置颜色为白色
    glColor3f(1.0f, 1.0f, 1.0f);
    
    // 绘制线段
    glLineWidth(lineWidth);
    glBegin(GL_LINES);
    
    // 绘制垂直线段（从中心到下端，长度为verticalLineLength）
    glVertex2f(pos.x, pos.y);
    glVertex2f(pos.x, pos.y + verticalLineLength);
    
    // 绘制水平线段（在最下端）
    glVertex2f(pos.x - lineLength * 0.5f, pos.y + verticalLineLength);
    glVertex2f(pos.x + lineLength * 0.5f, pos.y + verticalLineLength);
    
    glEnd();
    glLineWidth(1.0f); // 恢复默认线宽
}

// 绘制光标测试窗口
void Renderer::drawCursorTestWindow() {
    if (s_cursorTestWindowVisible) {
        ImGui::Begin("Cursor Test Window", &s_cursorTestWindowVisible);
        
        // 光标模式选择
        static const char* cursorModeNames[] = {"Default", "Crosshair", "Pickbox", "Panning"};
        static int currentMode = static_cast<int>(s_currentCursorMode);
        if (ImGui::Combo("Cursor Mode", &currentMode, cursorModeNames, IM_ARRAYSIZE(cursorModeNames))) {
            s_currentCursorMode = static_cast<CursorMode>(currentMode);
        }
        
        // 光标标记选择
        static const char* cursorMarkerNames[] = {"None", "LeftSelect", "RightSelect", "Locked", "AddSelect", "RemoveSelect", "Orthogonal"};
        static int currentMarker = static_cast<int>(s_currentCursorMarker);
        if (ImGui::Combo("Cursor Marker", &currentMarker, cursorMarkerNames, IM_ARRAYSIZE(cursorMarkerNames))) {
            s_currentCursorMarker = static_cast<CursorMarker>(currentMarker);
        }
        
        // 光标尺寸拖动条（范围10~100）
        static int crossCursorSizeInt = static_cast<int>(s_crossCursorSize);
        ImGui::PushItemWidth(300);
        if (ImGui::SliderInt("Cursor Size", &crossCursorSizeInt, 10, 100, "%d")) {
            s_crossCursorSize = static_cast<float>(crossCursorSizeInt);
        }
        ImGui::PopItemWidth();
        
        // 拾取框尺寸拖动条（范围0~50）
        static int pickBoxSizeInt = static_cast<int>(s_pickBoxSize);
        ImGui::PushItemWidth(300);
        if (ImGui::SliderInt("Pickbox Size", &pickBoxSizeInt, 0, 50, "%d")) {
            s_pickBoxSize = static_cast<float>(pickBoxSizeInt);
        }
        ImGui::PopItemWidth();
        
        ImGui::End();
    }
}

// 绘制光标标记
void Renderer::drawCursorMarker(const glm::vec2& pos) {
    // 标记中心位置：拾取框右上角的右上方10个像素
    glm::vec2 markerPos = pos;
    markerPos.x += s_pickBoxSize + 10.0f;
    markerPos.y -= s_pickBoxSize + 10.0f;
    
    switch (s_currentCursorMarker) {
        case CursorMarker::kLeftSelect:
            // 绘制向左框选标记
            drawLeftSelectMarker(markerPos);
            break;
            
        case CursorMarker::kRightSelect:
            // 绘制向右框选标记
            drawRightSelectMarker(markerPos);
            break;
            
        case CursorMarker::kLocked:
            // 绘制锁标记
            drawLockMarker(markerPos);
            break;
            
        case CursorMarker::kAddSelect:
            // 绘制加选标记（+号）
            drawSelectMarker(markerPos, true);
            break;
            
        case CursorMarker::kRemoveSelect:
            // 绘制减选标记（-号）
            drawSelectMarker(markerPos, false);
            break;
            
        case CursorMarker::kOrthogonal:
            // 绘制正交标记
            drawOrthogonalMarker(markerPos);
            break;
            
        case CursorMarker::kNone:
        default:
            // 无标记，不绘制
            break;
    }
}

// 设置光标模式
void Renderer::setCursorMode(CursorMode mode) {
    s_currentCursorMode = mode;
}

// 获取当前光标模式
Renderer::CursorMode Renderer::getCursorMode() {
    return s_currentCursorMode;
}

// 设置光标标记
void Renderer::setCursorMarker(CursorMarker marker) {
    s_currentCursorMarker = marker;
}

// 获取当前光标标记
Renderer::CursorMarker Renderer::getCursorMarker() {
    return s_currentCursorMarker;
}

// 设置十字光标大小
void Renderer::setCrossCursorSize(float size) {
    s_crossCursorSize = size;
}

// 获取十字光标大小
float Renderer::getCrossCursorSize() {
    return s_crossCursorSize;
}

// 获取当前光标世界坐标
glm::dvec3 Renderer::getCursorPosWorld() {
    return s_cursorPosWorld;
}

// 绘制栅格
void Renderer::drawGrid() {
    if (!s_initialized || !s_window || !DocManager::getCurrentDocument().isShowGrid()) {
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
    
    // 获取实际的视口边界
    int viewportLeft, viewportTop, viewportRight, viewportBottom;
    getTransformManager().getViewport().getViewport(viewportLeft, viewportTop, viewportRight, viewportBottom);
    
    // 计算视口大小
    int viewportWidth = viewportRight - viewportLeft;
    int viewportHeight = viewportBottom - viewportTop;
    
    // 使用视口边界计算世界视口范围
    // 考虑屏幕坐标系y轴向下
    glm::dvec3 worldMin = getTransformManager().screenToWorld(glm::vec2(viewportLeft, viewportBottom));
    glm::dvec3 worldMax = getTransformManager().screenToWorld(glm::vec2(viewportRight, viewportTop));
    
    // 计算世界视口的宽度和高度
    double worldWidth = worldMax.x - worldMin.x;
    double worldHeight = worldMax.y - worldMin.y;
    
    // 基础栅格间距（逻辑坐标）
    const double baseGridSize = 10.0;
    
    // 确定栅格级别
    // 目标是保持栅格在屏幕上的大小在合理范围内（50-250像素）
    double mainGridSize, subGridSize;
    
    // 计算当前栅格在屏幕上的大小（水平和垂直方向）
    double gridScreenSizeX = (baseGridSize / worldWidth) * viewportWidth;
    double gridScreenSizeY = (baseGridSize / worldHeight) * viewportHeight;
    
    // 取较小的值，确保栅格单元格在屏幕上保持正方形
    double gridScreenSize = std::min(gridScreenSizeX, gridScreenSizeY);
    
    if (gridScreenSize < 50.0) {
        // 当前栅格太小，需要增加栅格级别
        double testSize = baseGridSize;
        double testScreenSize = gridScreenSize;
        while (testScreenSize < 50.0) {
            testSize *= 5.0;
            testScreenSize *= 5.0;
        }
        
        // 设置新的栅格大小
        mainGridSize = testSize;
        subGridSize = mainGridSize / 5.0;
    } else if (gridScreenSize > 250.0) {
        // 当前栅格太大，需要减少栅格级别
        double testSize = baseGridSize;
        double testScreenSize = gridScreenSize;
        while (testScreenSize > 250.0) {
            testSize /= 5.0;
            testScreenSize /= 5.0;
        }
        
        // 设置新的栅格大小
        mainGridSize = testSize;
        subGridSize = mainGridSize / 5.0;
    } else {
        // 栅格大小在合理范围内
        mainGridSize = baseGridSize;
        subGridSize = mainGridSize / 5.0;
    }
    
    // 绘制子栅格
    glBegin(GL_LINES);
    glColor3fv(s_subGridColor);
    
    // 计算起始位置，确保栅格线与原点对齐
    double startX = floor(worldMin.x / subGridSize) * subGridSize;
    double startY = floor(worldMin.y / subGridSize) * subGridSize;
    
    // 绘制垂直线
    for (double x = startX; x <= worldMax.x; x += subGridSize) {
        glm::vec2 screenPos = getTransformManager().worldToScreen(glm::dvec3(x, worldMin.y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
        screenPos = getTransformManager().worldToScreen(glm::dvec3(x, worldMax.y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
    }
    
    // 绘制水平线
    for (double y = startY; y <= worldMax.y; y += subGridSize) {
        glm::vec2 screenPos = getTransformManager().worldToScreen(glm::dvec3(worldMin.x, y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
        screenPos = getTransformManager().worldToScreen(glm::dvec3(worldMax.x, y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
    }
    glEnd();
    
    // 绘制主栅格
    glBegin(GL_LINES);
    glColor3fv(s_mainGridColor);
    
    // 计算起始位置，确保栅格线与原点对齐
    double mainStartX = floor(worldMin.x / mainGridSize) * mainGridSize;
    double mainStartY = floor(worldMin.y / mainGridSize) * mainGridSize;
    
    // 绘制垂直线
    for (double x = mainStartX; x <= worldMax.x; x += mainGridSize) {
        glm::vec2 screenPos = getTransformManager().worldToScreen(glm::dvec3(x, worldMin.y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
        screenPos = getTransformManager().worldToScreen(glm::dvec3(x, worldMax.y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
    }
    
    // 绘制水平线
    for (double y = mainStartY; y <= worldMax.y; y += mainGridSize) {
        glm::vec2 screenPos = getTransformManager().worldToScreen(glm::dvec3(worldMin.x, y, 0.0));
        glVertex2f(screenPos.x, screenPos.y);
        screenPos = getTransformManager().worldToScreen(glm::dvec3(worldMax.x, y, 0.0));
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
    if (!s_initialized || !s_window || !DocManager::getCurrentDocument().isShowAxes()) {
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
    
    // 计算世界原点在屏幕上的位置
    glm::vec2 originScreenPos = getTransformManager().worldToScreen(glm::dvec3(0.0, 0.0, 0.0));
    float originScreenX = originScreenPos.x;
    float originScreenY = originScreenPos.y;
    
    // 获取实际的视口边界
    int viewportLeft, viewportTop, viewportRight, viewportBottom;
    getTransformManager().getViewport().getViewport(viewportLeft, viewportTop, viewportRight, viewportBottom);
    
    // 绘制X轴（正半轴）
    glBegin(GL_LINES);
    glColor3fv(s_xAxisColor);
    
    // 只绘制在视口范围内的部分
    if (originScreenX >= viewportLeft && originScreenX <= viewportRight && originScreenY >= viewportTop && originScreenY <= viewportBottom) {
        glVertex2f(originScreenX, originScreenY);
        glVertex2f(viewportRight * 1.0f, originScreenY);
    }
    glEnd();
    
    // 绘制Y轴（正半轴，朝上）
    glBegin(GL_LINES);
    glColor3fv(s_yAxisColor);
    
    // 只绘制在视口范围内的部分
    if (originScreenX >= viewportLeft && originScreenX <= viewportRight && originScreenY >= viewportTop && originScreenY <= viewportBottom) {
        glVertex2f(originScreenX, originScreenY);
        glVertex2f(originScreenX, viewportTop * 1.0f);
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
void Renderer::drawStatusBar() {
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    
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
    // 直接使用已保存的光标位置的世界坐标
    ImGui::Text("%.4f, %.4f, %.4f", s_cursorPosWorld.x, s_cursorPosWorld.y, s_cursorPosWorld.z);
    ImGui::End();
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
            static bool showGrid = DocManager::getCurrentDocument().isShowGrid();
            static bool showAxes = DocManager::getCurrentDocument().isShowAxes();
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
                    
                    // 滑块控件，范围10-100，使用整数，长度设为500
                    ImGui::PushItemWidth(500); // 设置滑块宽度为500
                    ImGui::SliderInt("##CrossCursorSize", &crossCursorSize, 10, 100, "%d");
                    ImGui::PopItemWidth();
                    
                    ImGui::EndTabItem();
                }
                
                // 第二个选项卡：选择集
                if (ImGui::BeginTabItem(loc.get("optionsDialog.tab.selection").c_str())) {
                    // 拾取框大小
                    ImGui::Spacing();
                    ImGui::Text(loc.get("optionsDialog.pickBoxSize").c_str());
                    ImGui::Spacing();
                    
                    // 首先绘制预览框
                    ImGui::BeginGroup();
                    
                    // 创建一个更大的预览区域，确保最大拾取框也能完全显示
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
                DocManager::getCurrentDocument().setShowGrid(showGrid);
                DocManager::getCurrentDocument().setShowAxes(showAxes);
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
                s_optionsDialogVisible = true;
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
    // 待切换的文件索引，-1表示无待切换文件
    static std::size_t s_pendingFileIndexToSwitch = -1;
    
    // 处理上一帧的待切换文档
    if (s_pendingFileIndexToSwitch != -1) {
        DocManager::setCurrentDocumentIndex(s_pendingFileIndexToSwitch);
        // 切换文档后，自动滚动命令历史到最底部
        s_bScrollCommandHistoryToBottom = true;
        // 重置待切换标记
        s_pendingFileIndexToSwitch = -1;
    }
    
    // 获取窗口大小
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);
    
    // 计算文件栏位置和大小
    ImVec2 fileBarPos(0, s_menuBarHeight);
    ImVec2 fileBarSize(width*1.0f, s_fileBarHeight);
    
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
            // 创建新文档
            if (ImGui::TabItemButton(" + ", ImGuiTabItemFlags_Trailing)) {
                // 创建新文档
                std::size_t newFileIndex = DocManager::createNewDocument();
                // 新建文档并切换也需要取消当前命令
                CommandManager::getInstance().cancelCurrentCommand();
                // 设置待切换的文件索引，在下一帧执行切换
                s_pendingFileIndexToSwitch = newFileIndex;
            }
            
            // 遍历文档列表，绘制每一个打开文档
            std::size_t documentCount = DocManager::getDocumentCount();
            // 待关闭的文件索引，-1表示无文件待关闭
            static std::size_t s_docIndexToBeClosed = -1;
            for (std::size_t i = 0; i < documentCount; i++) {
                // 获取文件名
                std::string tabText = DocManager::getFileName(i);
                
                // 设置标签项标志
                ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_None;
                if (DocManager::isDocumentModified(i)) {
                    tabItemFlags |= ImGuiTabItemFlags_UnsavedDocument;
                }
                
                bool tabOpen = true;
                if (ImGui::BeginTabItem(tabText.c_str(), &tabOpen, tabItemFlags)) {
                    // 切换文档时，不能直接切换，命令栏的取消命令执行操作需要在当前文档上下文，所有事情做完后下一帧去切换文档上下文
                    if (DocManager::getCurrentDocumentIndex() != i) {
                        // 切换文档时，取消当前命令执行
                        CommandManager::getInstance().cancelCurrentCommand();
                        // 设置待切换的文档索引，在下一帧执行切换
                        s_pendingFileIndexToSwitch = i;
                    }
                    ImGui::EndTabItem();
                }
                
                // 添加工具提示
                if (ImGui::IsItemHovered()) {
                    const std::string& fullFileName = DocManager::getFullFileName(i);
                    const std::string& filePath = DocManager::getFilePath(i);
                    ImGui::SetTooltip(filePath.empty() ? fullFileName.c_str() : filePath.c_str());
                }
                
                // 处理标签关闭
                if (!tabOpen) {
                    s_docIndexToBeClosed = i;
                }
            }
            // 循环内执行会破坏循环条件，循环完成后再执行关闭，关闭后当前文档会自动切换，不需要再去切换
            if (s_docIndexToBeClosed != -1) {
                CommandManager::getInstance().cancelCurrentCommand();
                DocManager::closeDocument(s_docIndexToBeClosed);
                s_docIndexToBeClosed = -1;
            }
            
            ImGui::EndTabBar();
        }
        
        // 更新文件栏高度
        s_fileBarHeight = ImGui::GetWindowSize().y;
        
        ImGui::End();
    }
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
    } else {
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
    // 绘制选项对话框
    drawOptionsDialog();
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
        
        const auto& commandHistory = DocManager::getCurrentDocumentCommandHistory();
        for (std::size_t i = 0; i < commandHistory.size(); i++)
        {
            ImGui::TextUnformatted(commandHistory[i].c_str());
        }
        
        // 根据标志决定是否滚动到最后，在绘制项目之前执行
        if (!commandHistory.empty() && s_bScrollCommandHistoryToBottom) {
            ImGui::SetScrollHereY(1.0f);
            s_bScrollCommandHistoryToBottom = false;
        }

        ImGui::EndChild();

        // 命令输入栏部分
        ImGui::Separator();
        
        // 调整布局：Command提示在左边，上下居中，输入框占满剩余空间
        ImGui::AlignTextToFramePadding();
        
        // 显示命令提示信息
        if (InputContext::getInstance().isInCommandExecution()) {
            const std::string& prompt = InputContext::getInstance().getPrompt();
            if (!prompt.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", prompt.c_str());
            } else {
                ImGui::Text(loc.get("commandLine.prompt.command").c_str());
            }
        } else {
            ImGui::Text(loc.get("commandLine.prompt.command").c_str());
        }
        
        ImGui::SameLine();
        
        // 如果需要设置焦点到命令输入框
        if (s_bShouldFocusOnCommandInput) {
            ImGui::SetKeyboardFocusHere(0);
            s_bShouldFocusOnCommandInput = false;
        }
        
        // 使用PushItemWidth使输入框占满剩余空间
        ImGui::PushItemWidth(-1);

        // 检查InputContext的特殊按键事件
        auto& inputContext = InputContext::getInstance();
        SpecialKeyEventType inputEvent = inputContext.getLastSpecialKeyEvent();
        // Enter/Space 提交输入框输入到输入上下文中进行处理
        if (inputEvent == SpecialKeyEventType::kEnterPressed || inputEvent == SpecialKeyEventType::kSpacePressed) {
            // 处理输入并清空缓冲区
            inputContext.handleEnterSpace(getAndClearCommandBuffer());
            // ImGui会内部维护InputText的缓冲区副本，Enter、Esc等事件时由上面的ImGui::SetKeyboardFocusHere所控制焦点会一直维持在命令输入框上，
            // 此时光清空外部缓冲区的话，每次InpuText调用都会把内部的副本重新同步回外部缓冲区来，那么就必须通过文本处理回调函数来清空内部的副本。
            // 而如果焦点已经不在输入框上了，那么单纯清除外部缓冲区就足够了。
            s_bNeedClearCommandBufferInternalCopy = true;
            // 清除特殊按键事件
            inputContext.clearSpecialKeyEvent();
        }
        // Esc 同样提交输入到输入上下文进行处理
        else if (inputEvent == SpecialKeyEventType::kEscPressed) {
            // 处理输入并清空缓冲区
            inputContext.handleEscape(getAndClearCommandBuffer());
            // 同理清除内部副本
            s_bNeedClearCommandBufferInternalCopy = true;
            // 清除特殊按键事件
            inputContext.clearSpecialKeyEvent();
        }
        
        // 回调函数处理文本选择问题、字符过滤与清除缓冲区
        auto inputTextCallback = [](ImGuiInputTextCallbackData* data) -> int {
            // 处理字符过滤逻辑 (只有输入字符时触发)
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
            }
            return 0;
        };

        ImGui::InputTextWithHint("##CommandInput", loc.get("commandLine.inputPrompt").c_str(), s_cmdBuffer.data(), s_cmdBuffer.size(), 
            ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_CallbackCharFilter, 
            inputTextCallback, nullptr);
        
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
    DocManager::addToCurrentDocumentCommandHistory(command);
    // 设置滚动标志为true，确保新命令添加后会滚动到最底部
    s_bScrollCommandHistoryToBottom = true;
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
    return buffer;
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
bool Renderer::focusIsOnWindow(const std::string& windowName) {
    if (ImGuiWindow* targetWindow = ImGui::FindWindowByName(windowName.c_str())) {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        if (ctx && ctx->NavWindow && ctx->NavWindow->RootWindow == targetWindow) {
            return true;
        }
    }
    return false;
}

// 检查焦点是否在命令输入框上
bool Renderer::focusIsOnCommandInput() {
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