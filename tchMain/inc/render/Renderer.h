#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace tch {

// 渲染器
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
    
    // 绘制十字光标
    static void drawCursor(const glm::vec2& position);
    
    // 设置光标大小
    static void setCursorSize(float size);
    
    // 获取光标大小
    static float getCursorSize();
    
    // 获取渲染器状态
    static bool isInitialized();

private:
    // 静态成员变量
    static bool m_initialized;
    static GLFWwindow* m_window;
    static float m_cursorSize;
};

} // namespace tch