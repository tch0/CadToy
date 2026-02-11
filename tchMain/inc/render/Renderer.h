#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

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
    static void setGridScale(float scale);       // 设置栅格缩放比例
    static float getGridScale();                 // 获取栅格缩放比例
    static void setOrigin(float x, float y);     // 设置坐标原点位置
    static glm::vec2 getOrigin();                // 获取坐标原点位置
    static void zoomIn();                        // 放大栅格
    static void zoomOut();                       // 缩小栅格

private:
    // 静态成员变量
    static bool s_initialized;                  // 渲染器初始化状态
    static GLFWwindow* s_window;                // 窗口指针
    static float s_cursorSize;                  // 光标大小
    
    // 栅格和坐标轴相关
    static bool s_showGrid;                     // 是否显示栅格
    static bool s_showAxes;                     // 是否显示XY轴
    static float s_gridScale;                   // 栅格缩放比例
    static float s_mainGridColor[3];            // 主栅格颜色 RGB: 54,61,78
    static float s_subGridColor[3];             // 子栅格颜色 RGB: 38,45,55
    static float s_xAxisColor[3];               // X轴颜色 RGB: 97,37,39
    static float s_yAxisColor[3];               // Y轴颜色 RGB: 34,89,41
    static float s_originX;                     // 坐标原点X位置
    static float s_originY;                     // 坐标原点Y位置

    // 辅助方法
    static void drawGrid();                     // 绘制栅格
    static void drawAxes();                     // 绘制XY轴
};

} // namespace tch