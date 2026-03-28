#pragma once

// C++ 标准库
#include <vector>

// 第三方库
#include <glad/gl.h>
#include <glm/glm.hpp>

// 项目头文件
#include "gl/Shader.h"

namespace tch {

// 实体渲染器类
// 负责使用现代OpenGL shader渲染3D世界空间中的实体（线段、多边形等）
// 使用FBO离屏渲染，支持几何着色器实现可变线宽
class EntityRenderer {
private:
    // 顶点结构体
    struct Vertex {
        glm::vec3 position;  // 世界坐标位置
        glm::vec4 color;     // RGBA颜色
        float texCoord;      // 沿线段方向的纹理坐标（用于虚线）
    };
    
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
    
    // 实体着色器（顶点+几何+片段）
    Shader m_entityShader;            // 实体着色器程序
    GLint m_mvpLocation;              // uMVP uniform位置
    GLint m_isDashedLocation;         // uIsDashed uniform位置（0=实线，1=虚线）
    GLint m_dashScaleLocation;        // uDashScale uniform位置
    GLint m_useVertexColorLocation;   // uUseVertexColor uniform位置
    
    // 全屏四边形VAO/VBO（用于FBO纹理渲染到屏幕）
    GLuint m_quadVAO;        // 四边形顶点数组对象
    GLuint m_quadVBO;        // 四边形顶点缓冲对象
    Shader m_quadShader;     // 四边形着色器程序
    GLint m_quadTextureLocation;  // uTexture uniform位置
    
    // 虚线纹理
    GLuint m_dashTexture;  // 一维虚线纹理
    
    // 顶点缓冲
    std::vector<Vertex> m_vertices;  // 顶点数据缓冲区
    
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
    void initDashTexture();  // 初始化虚线纹理
    void ensureFBOSize(int width, int height);  // 确保FBO尺寸足够
    void setupQuadVAO();  // 设置全屏四边形VAO
    
    // 渲染辅助方法
    void setDashedMode(bool isDashed, float period = 8.0f);  // 设置虚线模式，period为虚线周期像素数
    void renderToFBO(const glm::mat4& mvp, int viewportLeft, int viewportBottom, 
                     int viewportWidth, int viewportHeight);  // 渲染实体到FBO
    void renderTextureToScreen();  // 将FBO纹理渲染到屏幕
    void renderGeometry(const glm::mat4& mvp);  // 渲染几何体
    
    // 顶点管理
    void clearVertices();  // 清空顶点缓冲
    void addVertex(const glm::vec3& pos, const glm::vec4& color, float texCoord = 0.0f);  // 添加顶点
    void flushVertices();  // 上传顶点数据到GPU
};

} // namespace tch
