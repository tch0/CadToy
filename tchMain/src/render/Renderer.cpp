#include "render/Renderer.h"
#include "Layer.h"

namespace tch {

// 静态成员初始化
bool Renderer::m_initialized = false;
GLFWwindow* Renderer::m_window = nullptr;
float Renderer::m_cursorSize = 30.0f;

// 初始化渲染器
void Renderer::initialize(GLFWwindow* window) {
    m_window = window;
    m_initialized = true;
    
    // 设置默认背景颜色
    setBackgroundColor(0.2f, 0.2f, 0.2f, 1.0f);
    
    // 获取窗口尺寸并设置视口
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    setViewport(width, height);
    
    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// 清理渲染器
void Renderer::cleanup() {
    m_initialized = false;
    m_window = nullptr;
}

// 开始渲染
void Renderer::beginRender() {
    if (!m_initialized || !m_window) {
        return;
    }
    
    // 清除颜色缓冲
    glClear(GL_COLOR_BUFFER_BIT);
}

// 结束渲染
void Renderer::endRender() {
    if (!m_initialized || !m_window) {
        return;
    }
    
    // 交换缓冲区
    glfwSwapBuffers(m_window);
}

// 设置背景颜色
void Renderer::setBackgroundColor(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
}

// 设置视口
void Renderer::setViewport(int width, int height) {
    glViewport(0, 0, width, height);
}

// 绘制所有图形
void Renderer::drawAll() {
    if (!m_initialized || !m_window) {
        return;
    }
    
    // 绘制所有图层
    LayerManager::getInstance().draw();
}

// 获取渲染器状态
bool Renderer::isInitialized() {
    return m_initialized;
}

// 绘制光标
void Renderer::drawCursor(const glm::vec2& position) {
    if (!m_initialized || !m_window) {
        return;
    }
    
    // 保存当前矩阵状态
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    // 设置正交投影
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    glOrtho(0, width, height, 0, -1, 1);
    
    // 切换到模型视图矩阵
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // 禁用深度测试
    glDisable(GL_DEPTH_TEST);
    
    // 绘制十字线
    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
    
    // 水平线
    glVertex2f(position.x - m_cursorSize, position.y);
    glVertex2f(position.x + m_cursorSize, position.y);
    
    // 垂直线
    glVertex2f(position.x, position.y - m_cursorSize);
    glVertex2f(position.x, position.y + m_cursorSize);
    glEnd();
    
    // 绘制正方形
    float halfSize = m_cursorSize * 0.5f;
    glBegin(GL_LINE_LOOP);
    glVertex2f(position.x - halfSize, position.y - halfSize);
    glVertex2f(position.x + halfSize, position.y - halfSize);
    glVertex2f(position.x + halfSize, position.y + halfSize);
    glVertex2f(position.x - halfSize, position.y + halfSize);
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
    m_cursorSize = size;
}

// 获取光标大小
float Renderer::getCursorSize() {
    return m_cursorSize;
}

} // namespace tch