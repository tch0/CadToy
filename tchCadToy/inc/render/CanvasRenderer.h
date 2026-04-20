#pragma once

// C++ 标准库
#include <vector>

// 第三方库
#include <glad/gl.h>
#include <glm/glm.hpp>

// 项目头文件


namespace tch {

// RAII状态守卫：用于模板缓冲填充操作
class StencilFillGuard {
public:
    StencilFillGuard();
    ~StencilFillGuard();
    
    void setupFill();  // 切换到填充模式
    
private:
    GLboolean m_stencilEnabled;
    GLboolean m_colorMask[4];
    GLint m_stencilWriteMask;
    GLint m_stencilFunc;
    GLint m_stencilRef;
    GLint m_stencilValueMask;
    GLint m_stencilFail;
    GLint m_stencilZFail;
    GLint m_stencilZPass;
    GLboolean m_depthTestEnabled;
    GLboolean m_depthMask;
    GLboolean m_blendEnabled;
};

// RAII状态守卫：控制抗锯齿状态
// enable=true时启用抗锯齿，enable=false时禁用抗锯齿
// 析构时恢复原始状态
class AAGuard {
public:
    explicit AAGuard(bool enable);
    ~AAGuard();
    
private:
    GLboolean m_lineSmoothEnabled;
    GLboolean m_polygonSmoothEnabled;
    GLboolean m_multisampleEnabled;
};

// 画布渲染器类
// 负责使用现代OpenGL shader渲染栅格、坐标轴、光标、选择区域等画布元素
class CanvasRenderer {
private:
    // 顶点结构体
    struct Vertex {
        glm::vec2 position;  // 屏幕坐标位置，标准鼠标坐标系y轴向下
        glm::vec4 color;     // RGBA颜色
        float texCoord;      // 沿线段方向的纹理坐标（0到线段长度）
    };
    
    // 顶点缓冲
    std::vector<Vertex> m_vertices;  // 顶点数据缓冲区
    GLuint m_vao;                    // 顶点数组对象
    GLuint m_vbo;                    // 顶点缓冲对象
    
    // 变换矩阵
    glm::mat4 m_projection;  // 投影矩阵（屏幕坐标系）
    glm::mat4 m_view;        // 视图矩阵
    
    // Shader
    GLuint m_canvasProgram;  // 统一的画布着色器程序（支持实线和虚线模式）
    
    // 虚线纹理
    GLuint m_dashTexture;  // 一维虚线纹理
    
    // Uniform locations
    GLint m_mvpLocation;       // uProjection uniform位置
    GLint m_isDashedLocation;  // uIsDashed uniform位置（0=实线，1=虚线）
    GLint m_dashScaleLocation; // uDashScale uniform位置
    
public:
    CanvasRenderer();
    ~CanvasRenderer();
    
    // 初始化与清理
    bool initialize();  // 初始化渲染器（加载shader、创建VAO/VBO、初始化虚线纹理）
    void cleanup();     // 清理资源
    
    // 变换矩阵设置
    void setProjection(const glm::mat4& projection);  // 设置投影矩阵
    void setView(const glm::mat4& view);              // 设置视图矩阵
    
    // 绘制方法
    void drawGrid();          // 绘制栅格
    void drawAxes();          // 绘制XY坐标轴
    void drawCursor();        // 绘制光标（十字、拾取框等）
    void drawSelection();     // 绘制选择区域
    void drawCursorMarker();  // 绘制光标标记（锁定、正交、选择模式等）
    
    // 顶点管理
    void clearVertices();                                           // 清空顶点缓冲
    void addVertex(const glm::vec2& pos, const glm::vec4& color);   // 添加顶点（texCoord=0）
    void addVertex(const glm::vec2& pos, const glm::vec4& color, float texCoord);  // 添加顶点（带纹理坐标）
    void flushVertices();                                           // 上传顶点数据到GPU

private:
    // Shader辅助方法
    void initDashTexture();        // 初始化虚线纹理
    void setDashedMode(bool isDashed, float period = 8.0f);  // 设置虚线模式，period为虚线周期像素数
    
    // 多边形填充（使用模板缓冲支持凹多边形）
    void drawPolygonFill(const std::vector<glm::vec2>& points, const glm::vec4& fillColor);
    
    // 选择区域绘制辅助方法
    void drawWindowSelectionVertices(const std::vector<glm::vec2>& points, 
                                      const glm::vec4& fillColor, 
                                      const glm::vec4& lineColor,
                                      bool useDashedLine = false);  // 绘制窗口选择顶点
    void drawLassoSelectionVertices(const std::vector<glm::vec2>& points,
                                     const glm::vec4& fillColor,
                                     const glm::vec4& lineColor,
                                     bool useDashedLine = false);  // 绘制套索选择顶点
    void drawPolygonSelectionVertices(const std::vector<glm::vec2>& points,
                                       const glm::vec4& fillColor,
                                       const glm::vec4& lineColor,
                                       bool useDashedLine = false);  // 绘制多边形选择顶点
    void drawFenceSelectionVertices(const std::vector<glm::vec2>& points,
                                     const glm::vec4& lineColor,
                                     bool useDashedLine = false);  // 绘制栏选顶点
};

} // namespace tch
