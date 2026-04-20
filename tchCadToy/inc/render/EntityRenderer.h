#pragma once

// C++ 标准库

// 第三方库
#include <glad/gl.h>
#include <glm/glm.hpp>

// 项目头文件


namespace tch {

// 顶点数据结构，后续应该移动到图形引擎相关头文件中。
// 用于实体渲染的顶点格式，包含位置、颜色、状态标志和线宽
struct Vertex {
    glm::vec3 position;     // 位置（世界坐标）
    glm::vec3 color;        // 基础颜色 (RGB)
    uint32_t flags;         // 状态标志位：bit0=预选高亮(加宽2个像素), bit1=选中高亮(绘制为虚线), bit2=暗显(颜色变浅)
    float lineWidth;        // 线宽值（屏幕像素为单位），线宽LineWeight应该需要经过换算才能得到这个像素线宽
};

// 实体渲染器类
// 负责使用现代OpenGL shader渲染3D世界空间中的实体（线段、多边形等）
// 使用FBO离屏渲染，支持几何着色器实现可变线宽
class EntityRenderer {
private:
    // FBO（帧缓冲对象）
    GLuint m_fbo;                 // 离屏渲染帧缓冲
    GLuint m_colorTexture;        // 颜色附件纹理
    GLuint m_depthStencilBuffer;  // 深度/模板缓冲
    
    // 窗口尺寸
    int m_windowWidth;   // 窗口宽度
    int m_windowHeight;  // 窗口高度
    
    // 实体渲染VAO/VBO
    GLuint m_vao;  // 顶点数组对象
    GLuint m_vbo;  // 顶点缓冲对象
    
    // 无线宽着色器程序
    GLuint m_noLWProgram;
    GLint m_noLWMvpLoc;
    GLint m_noLWViewportSizeLoc;
    
    // 有线宽着色器程序
    GLuint m_withLWProgram;
    GLint m_withLWMvpLoc;
    GLint m_withLWViewportSizeLoc;
    
    // 全屏四边形VAO/VBO（用于FBO纹理渲染到屏幕）
    GLuint m_quadVAO;        // 四边形顶点数组对象
    GLuint m_quadVBO;        // 四边形顶点缓冲对象
    GLuint m_quadProgram;    // 四边形着色器程序
    GLint m_quadTextureLoc;  // uTexture uniform位置
    
public:
    EntityRenderer();
    ~EntityRenderer();
    
    // 初始化与清理
    bool initialize();  // 初始化渲染器（创建FBO、加载shader、创建VAO/VBO）
    void cleanup();     // 清理资源
    
    // 窗口尺寸更新
    void updateWindowSize(int width, int height);  // 更新窗口尺寸，调整FBO大小
    
    // 绘制方法
    void drawEntities();  // 绘制所有实体
    
private:
    // 初始化方法
    void ensureFBOSize(int width, int height);  // 确保FBO尺寸足够
    void setupQuadVAO();  // 设置全屏四边形VAO
    
    // 渲染辅助方法
    void renderToFBO(const glm::mat4& mvp, int viewportLeft, int viewportBottom, 
                     int viewportWidth, int viewportHeight);  // 渲染实体到FBO
    void renderTextureToScreen();  // 将FBO纹理渲染到屏幕
    void renderGeometry(const glm::mat4& mvp);  // 渲染几何体
};

} // namespace tch
