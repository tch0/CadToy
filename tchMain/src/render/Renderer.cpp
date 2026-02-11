#include "Renderer.h"
#include "Layer.h"

namespace tch {

// 静态成员初始化
bool Renderer::m_initialized = false;
GLFWwindow* Renderer::m_window = nullptr;

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

} // namespace tch