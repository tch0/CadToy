#pragma once

// C++ 标准库
#include <string>

// 第三方库
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>

// 项目头文件
#include "transform/TransformManager.h"

namespace tch {

// 渲染器类
class Renderer {
public:
    // 初始化渲染器
    static void initialize(GLFWwindow* window);
    
    // 清理渲染器
    static void cleanup();
    
    // 获取渲染器状态
    static bool isInitialized();
    
    // 重算布局、更新视口
    static void calculateLayoutAndUpdateViewport();
    
    // 开始渲染
    static void beginRender();
    
    // 结束渲染
    static void endRender();
    
    // 设置背景颜色
    static void setBackgroundColor(float r, float g, float b, float a = 1.0f);
    
    // 绘制所有图形
    static void drawAll();
    
    // 选择相关
    static void drawSelection(); // 绘制选择相关图元
    
    // 光标相关
    static void drawCursor(); // 绘制光标
    static void setCrossCursorSize(float size); // 设置十字光标大小
    static float getCrossCursorSize(); // 获取十字光标大小
    static glm::dvec3 getCursorPosWorld(); // 获取当前光标世界坐标

    // 视图操作方法
    static void zoomIn(const glm::vec2& cursorPos);  // 以光标位置为中心放大
    static void zoomOut(const glm::vec2& cursorPos); // 以光标位置为中心缩小
    static void pan(const glm::vec2& deltaScreen); // 平移功能
    
    // 状态栏相关
    static void drawStatusBar(); // 绘制状态栏
    
    // 绘制菜单栏
    static void drawMenuBar(); // 绘制菜单栏
    
    // 绘制属性栏
    static void drawPropertyBar(); // 绘制属性栏
    
    // 文件栏相关
    static void drawFileBar(); // 绘制文件栏
    
    // 变换管理器相关方法
    static TransformManager& getTransformManager(); // 获取变换管理器
    
    // 命令栏相关方法
    static void drawCommandBar(); // 绘制命令栏
    static void addContentToCommandLineHistory(const std::string& content); // 添加内容到命令行历史
    static void setShouldFocusOnCommandInput(bool shouldFocus); // 设置是否应该将焦点设置到命令输入框
    static void removeLastCharFromCommandInput(); // 从命令输入缓冲区中删除最后一个字符
    static void addInputChar(unsigned int codepoint); // 添加输入字符到命令输入框
    static std::string getAndClearCommandBuffer(); // 获取输入缓冲区内容，并清空输入缓冲区
    
    // 命令历史导航相关方法
    static bool isInCommandHistoryNavigationMode();
    static std::string navigateCommandHistoryAndGetExpectedCommand(bool isUp);
    static void exitCommandHistoryNavigationMode();
    
    // 实时渲染信息窗口相关方法
    static void drawRenderingInfoWindow(); // 绘制实时渲染信息窗口
    
    // 输入上下文信息窗口相关方法
    static void drawInputContextInfoWindow(); // 绘制输入上下文信息窗口
    
    // 窗口绘制相关方法
    static void drawModalDialogs(); // 绘制模态对话框
    static void drawNonModalWindows(); // 绘制非模态窗口
    
    // 视口相关方法
    static bool isPointInViewport(const glm::vec2& screenPos); // 判断点是否在视口内
    
    // 属性栏相关方法
    static bool isPropertyBarVisible(); // 获取属性栏是否可见
    static void setPropertyBarVisible(bool visible); // 设置属性栏是否可见
    
    // 选项对话框相关方法
    static void showOptionsDialog(bool visible); // 显示或隐藏选项对话框
    static void drawOptionsDialog(); // 绘制选项对话框
    
    // 焦点检查相关方法
    static bool focusIsOnWindow(const std::string& windowName); // 检查焦点是否位于指定窗口或其子窗口，通用方法
    static bool focusIsOnCommandBar(); // 检查焦点是否位于命令栏
    static bool focusIsOnCommandInput(); // 检查焦点是否在命令输入框上
    
    
private:
    // 静态成员变量
    static bool s_initialized;                  // 渲染器初始化状态
    static GLFWwindow* s_window;                // 窗口指针
    
    // 光标相关
    static float s_crossCursorSize;             // 十字光标大小（包含拾取框的总大小，线段长度为十字光标大小减去拾取框大小）
    static float s_pickBoxSize;                 // 拾取框大小
    static glm::dvec3 s_cursorPosWorld;         // 当前光标位置的世界坐标
    static bool s_cursorTestWindowVisible;      // 光标测试窗口可见性
    
    // 栅格和坐标轴颜色
    static float s_mainGridColor[3];            // 主栅格颜色 RGB: 54,61,78
    static float s_subGridColor[3];             // 子栅格颜色 RGB: 38,45,55
    static float s_xAxisColor[3];               // X轴颜色 RGB: 97,37,39
    static float s_yAxisColor[3];               // Y轴颜色 RGB: 34,89,41
    
    // 选择区域颜色
    static const float s_windowSelectionColor[4]; // 窗口选择颜色（蓝色，透明度0.2）
    static const float s_crossingSelectionColor[4]; // 交叉选择颜色（绿色，透明度0.2）
    
    // UI组件高度
    static float s_menuBarHeight;              // 菜单栏高度
    static float s_fileBarHeight;              // 文件栏高度
    static float s_statusBarHeight;            // 状态栏高度
    
    // 命令栏相关
    static bool s_bScrollCommandLineHistoryToBottom; // 是否应该将命令历史滚动到底部
    static bool s_bShouldFocusOnCommandInput; // 是否应该将焦点设置到命令输入框
    static bool s_bCommandBufferModified; // 命令输入缓冲区是否被修改，通过非命令输入栏的字符输入或者退格
    static bool s_bNeedClearCommandBufferInternalCopy; // 是否需要清除命令输入缓冲区的内部副本
    static ImGuiID s_commandBarId; // 每一帧记录CommandBar的ID，判断焦点时直接通过此ID比较而不需要任何查找
    static ImGuiID s_commandInputId; // 每一帧记录CommandInput输入框的ID，判断焦点时直接通过此ID比较而不需要任何查找
    static int s_commandHistoryNavigationIndex; // 命令历史导航索引，-1 表示不在导航模式
    
    
    // 辅助方法
    static void drawGrid();                     // 绘制栅格
    static void drawAxes();                     // 绘制XY轴
    static void initializeImGui();              // 初始化ImGui
    static void cleanupImGui();                 // 清理ImGui
    // 光标绘制相关
    static void drawPickBox(const glm::vec2& pos, float pickBoxSize); // 绘制拾取框
    static void drawCrosshair(const glm::vec2& pos, float pickBoxSize); // 绘制十字线
    static void drawHandCursor(const glm::vec2& pos); // 绘制手掌光标
    static void drawCrossingSelectMarker(const glm::vec2& pos); // 绘制交叉选择标记
    static void drawWindowSelectMarker(const glm::vec2& pos); // 绘制窗口选择标记
    static void drawLockMarker(const glm::vec2& pos); // 绘制锁标记
    static void drawSelectMarker(const glm::vec2& pos, bool isAdd); // 绘制加选和减选标记
    static void drawOrthogonalMarker(const glm::vec2& pos); // 绘制正交标记
    static void drawCopyMarker(const glm::vec2& pos); // 绘制复制标记
    static void drawEraseMarker(const glm::vec2& pos); // 绘制删除标记
    static void drawMoveMarker(const glm::vec2& pos); // 绘制移动标记
    static void drawRotateMarker(const glm::vec2& pos); // 绘制旋转标记
    static void drawScaleMarker(const glm::vec2& pos); // 绘制缩放标记
    static void drawCursorMarker(const glm::vec2& pos); // 绘制光标标记
    static void drawCursorTestWindow(); // 绘制光标测试窗口
    
    // 选择相关
    static void drawWindowSelection();          // 绘制框选
    static void drawLassoSelection();           // 绘制套索选择
    static void drawPolygonSelection();         // 绘制多边形选择
    static void drawFenceSelection();           // 绘制栏选
};

} // namespace tch