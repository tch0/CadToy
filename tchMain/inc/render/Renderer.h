#pragma once
#include <string>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "render/LogicalViewport.h"

namespace tch {

// 渲染器类
class Renderer {
public:
    // 初始化渲染器
    static void initialize(GLFWwindow* window);
    
    // 清理渲染器
    static void cleanup();
    
    // 开始渲染
    static void beginRender();
    
    // 结束渲染
    static void endRender();
    
    // 设置背景颜色
    static void setBackgroundColor(float r, float g, float b, float a = 1.0f);
    
    // 设置视口
    static void setViewport(int width, int height);
    
    // 绘制所有图形
    static void drawAll();
    
    // 更新视口大小（窗口大小变化时调用）
    static void updateViewport(int width, int height);
    
    // 绘制十字光标
    static void drawCursor(const glm::vec2& position);
    
    // 设置光标大小
    static void setCursorSize(float size);
    
    // 获取光标大小
    static float getCursorSize();
    
    // 获取渲染器状态
    static bool isInitialized();

    // 栅格和坐标轴控制方法
    static void setShowGrid(bool show);         // 设置是否显示栅格
    static void setShowAxes(bool show);         // 设置是否显示XY轴
    static void setOrigin(float x, float y);     // 设置坐标原点位置
    static glm::vec2 getOrigin();                // 获取坐标原点位置
    static void zoomIn();                        // 放大栅格
    static void zoomOut();                       // 缩小栅格
    static void zoomIn(const glm::vec2& mousePos); // 以鼠标位置为中心放大
    static void zoomOut(const glm::vec2& mousePos); // 以鼠标位置为中心缩小
    static void drawStatusBar(const glm::vec2& cursorPos); // 绘制状态栏
    
    // 绘制菜单栏
    static void drawMenuBar(); // 绘制菜单栏
    
    // 绘制命令栏
    static void drawCommandBar(); // 绘制命令栏
    
    // 绘制属性栏
    static void drawPropertyBar(); // 绘制属性栏
    
    // 绘制文件栏
    static void drawFileBar(); // 绘制文件栏
    
    // 更新可绘制区域
    static void updateDrawableArea(); // 更新可绘制区域
    
    // 逻辑视口相关方法
    static LogicalViewport& getLogicalViewport(); // 获取逻辑视口
    
    // 命令栏相关方法
    static void addCommandToHistory(const std::string& command); // 添加命令到历史记录
    
    // 属性栏相关方法
    static bool isPropertyBarVisible(); // 获取属性栏是否可见
    static void setPropertyBarVisible(bool visible); // 设置属性栏是否可见
    
    // 选项对话框相关方法
    static void showOptionsDialog(bool visible); // 显示或隐藏选项对话框
    static void drawOptionsDialog(); // 绘制选项对话框
    
    

private:
    // 静态成员变量
    static bool s_initialized;                  // 渲染器初始化状态
    static GLFWwindow* s_window;                // 窗口指针
    static float s_cursorSize;                  // 光标大小
    static float s_pickBoxSize;                // 拾取框大小
    
    // 栅格和坐标轴相关
    static bool s_showGrid;                     // 是否显示栅格
    static bool s_showAxes;                     // 是否显示XY轴
    static float s_mainGridColor[3];            // 主栅格颜色 RGB: 54,61,78
    static float s_subGridColor[3];             // 子栅格颜色 RGB: 38,45,55
    static float s_xAxisColor[3];               // X轴颜色 RGB: 97,37,39
    static float s_yAxisColor[3];               // Y轴颜色 RGB: 34,89,41
    static float s_originX;                     // 坐标原点X位置
    static float s_originY;                     // 坐标原点Y位置
    static glm::dvec3 s_cursorPosition;          // 当前光标位置（以窗口中央为原点）
    
    // 逻辑视口
    static LogicalViewport s_logicalViewport; // 逻辑视口
    
    // UI组件高度
    static float s_menuBarHeight;              // 菜单栏高度
    static float s_fileBarHeight;              // 文件栏高度
    static float s_statusBarHeight;            // 状态栏高度

    // 辅助方法
    static void drawGrid();                     // 绘制栅格
    static void drawAxes();                     // 绘制XY轴
    static void initializeImGui();              // 初始化ImGui
    static void cleanupImGui();                 // 清理ImGui
};

} // namespace tch