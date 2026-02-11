#include "render/Renderer.h"
#include "Layer.h"

namespace tch {

// 静态成员初始化
bool Renderer::s_initialized = false;
GLFWwindow* Renderer::s_window = nullptr;
float Renderer::s_cursorSize = 10.0f;

// 栅格和坐标轴相关初始化
bool Renderer::s_showGrid = true;         // 默认显示栅格
bool Renderer::s_showAxes = true;         // 默认显示XY轴
float Renderer::s_gridScale = 1.0f;       // 默认栅格缩放比例
float Renderer::s_mainGridColor[3] = {54.0f/255.0f, 61.0f/255.0f, 78.0f/255.0f}; // 主栅格颜色 RGB: 54,61,78
float Renderer::s_subGridColor[3] = {38.0f/255.0f, 45.0f/255.0f, 55.0f/255.0f};  // 子栅格颜色 RGB: 38,45,55
float Renderer::s_xAxisColor[3] = {97.0f/255.0f, 37.0f/255.0f, 39.0f/255.0f};    // X轴颜色 RGB: 97,37,39
float Renderer::s_yAxisColor[3] = {34.0f/255.0f, 89.0f/255.0f, 41.0f/255.0f};    // Y轴颜色 RGB: 34,89,41
float Renderer::s_originX = 0.0f;          // 坐标原点X位置
float Renderer::s_originY = 0.0f;          // 坐标原点Y位置

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
    
    // 设置坐标原点为窗口中心
    s_originX = width / 2.0f;
    s_originY = height / 2.0f;
    
    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// 清理渲染器
void Renderer::cleanup() {
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
}

// 结束渲染
void Renderer::endRender() {
    if (!s_initialized || !s_window) {
        return;
    }
    
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
    
    // 绘制十字线
    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f); // 白色光标
    
    // 水平线
    glVertex2f(position.x - s_cursorSize, position.y);
    glVertex2f(position.x + s_cursorSize, position.y);
    
    // 垂直线
    glVertex2f(position.x, position.y - s_cursorSize);
    glVertex2f(position.x, position.y + s_cursorSize);
    glEnd();
    
    // 绘制正方形
    float halfSize = s_cursorSize * 0.5f;
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
    
    // 基础栅格间距
    const float baseGridSize = 50.0f;
    
    // 计算当前有效的栅格间距（考虑缩放）
    float currentEffectiveSize = baseGridSize * s_gridScale;
    
    // 确定栅格级别
    // 目标是保持主栅格大小在合理范围内（50-250像素）
    float mainGridSize, subGridSize;
    
    if (currentEffectiveSize < 50.0f) {
        // 当前栅格太小，需要增加栅格级别
        int level = 0;
        float testSize = currentEffectiveSize;
        while (testSize < 50.0f) {
            testSize *= 5.0f;
            level++;
        }
        
        // 设置新的栅格大小
        mainGridSize = testSize;
        subGridSize = mainGridSize / 5.0f;
    } else if (currentEffectiveSize > 250.0f) {
        // 当前栅格太大，需要减少栅格级别
        int level = 0;
        float testSize = currentEffectiveSize;
        while (testSize > 250.0f) {
            testSize /= 5.0f;
            level++;
        }
        
        // 设置新的栅格大小
        mainGridSize = testSize;
        subGridSize = mainGridSize / 5.0f;
    } else {
        // 栅格大小在合理范围内
        mainGridSize = currentEffectiveSize;
        subGridSize = mainGridSize / 5.0f;
    }
    
    // 绘制子栅格
    glBegin(GL_LINES);
    glColor3fv(s_subGridColor);
    
    // 计算起始位置，确保栅格线与原点对齐
    float startX = fmod(s_originX, subGridSize);
    float startY = fmod(s_originY, subGridSize);
    
    // 绘制垂直线
    for (float x = startX; x < width; x += subGridSize) {
        glVertex2f(x, 0);
        glVertex2f(x, height);
    }
    
    // 绘制水平线
    for (float y = startY; y < height; y += subGridSize) {
        glVertex2f(0, y);
        glVertex2f(width, y);
    }
    glEnd();
    
    // 绘制主栅格
    glBegin(GL_LINES);
    glColor3fv(s_mainGridColor);
    
    // 计算起始位置，确保栅格线与原点对齐
    float mainStartX = fmod(s_originX, mainGridSize);
    float mainStartY = fmod(s_originY, mainGridSize);
    
    // 绘制垂直线
    for (float x = mainStartX; x < width; x += mainGridSize) {
        glVertex2f(x, 0);
        glVertex2f(x, height);
    }
    
    // 绘制水平线
    for (float y = mainStartY; y < height; y += mainGridSize) {
        glVertex2f(0, y);
        glVertex2f(width, y);
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
    
    // 绘制X轴（正半轴）
    glBegin(GL_LINES);
    glColor3fv(s_xAxisColor);
    glVertex2f(s_originX, s_originY);
    glVertex2f(width, s_originY);
    glEnd();
    
    // 绘制Y轴（正半轴，朝上）
    glBegin(GL_LINES);
    glColor3fv(s_yAxisColor);
    glVertex2f(s_originX, s_originY);
    glVertex2f(s_originX, height);
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

void Renderer::setGridScale(float scale) {
    s_gridScale = scale;
}

float Renderer::getGridScale() {
    return s_gridScale;
}

void Renderer::setOrigin(float x, float y) {
    s_originX = x;
    s_originY = y;
}

glm::vec2 Renderer::getOrigin() {
    return glm::vec2(s_originX, s_originY);
}

void Renderer::zoomIn() {
    s_gridScale *= 0.8f; // 放大20%
}

void Renderer::zoomOut() {
    s_gridScale *= 1.25f; // 缩小25%
}

} // namespace tch