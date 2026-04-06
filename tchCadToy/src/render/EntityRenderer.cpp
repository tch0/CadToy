// 对应头文件
#include "EntityRenderer.h"

// C++ 标准库

// 第三方库
#include <glm/gtc/type_ptr.hpp>

// 项目头文件
#include "Logger.h"
#include "DocManager.h"

namespace tch {

// ============================================================================
// 嵌入式着色器源码
// ============================================================================

// 实体顶点着色器
// 将世界坐标转换为裁剪空间坐标，传递颜色和纹理坐标
static const char* ENTITY_VERTEX_SHADER = R"(
#version 330 core

layout (location = 0) in vec3 aPos;       // 世界坐标位置
layout (location = 1) in vec4 aColor;     // RGBA颜色
layout (location = 2) in float aTexCoord; // 沿线段方向的纹理坐标

uniform mat4 uMVP;  // 模型-视图-投影矩阵

out vec4 vColor;     // 传递到几何着色器的颜色
out float vTexCoord; // 传递到几何着色器的纹理坐标

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
    vTexCoord = aTexCoord;
}
)";

// 实体几何着色器
// 将线段扩展为带宽度的四边形（三角形带）
// 通过计算线段方向的法向量，在屏幕空间中生成带宽度的线条
static const char* ENTITY_GEOMETRY_SHADER = R"(
#version 330 core

layout(lines) in;                      // 输入：线段（2个顶点）
layout(triangle_strip, max_vertices = 4) out;  // 输出：四边形（4个顶点）

uniform float uLineWidth;      // 线宽（像素）
uniform vec2 uViewportSize;    // 视口尺寸

in vec4 vColor[];      // 来自顶点着色器的颜色
in float vTexCoord[];  // 来自顶点着色器的纹理坐标

out vec4 gColor;      // 传递到片段着色器的颜色
out float gTexCoord;  // 传递到片段着色器的纹理坐标

void main() {
    // 获取线段端点的屏幕坐标（NDC -> 屏幕空间）
    vec2 p0 = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    vec2 p1 = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
    
    p0 = (p0 + 1.0) * 0.5 * uViewportSize;
    p1 = (p1 + 1.0) * 0.5 * uViewportSize;
    
    // 计算线段方向和法向量
    vec2 dir = normalize(p1 - p0);
    vec2 normal = vec2(-dir.y, dir.x);
    
    // 计算偏移量（半宽度）
    float halfWidth = uLineWidth * 0.5;
    vec2 offset = normal * halfWidth;
    
    // 生成四个顶点（屏幕空间 -> NDC）
    // 左上顶点
    gl_Position = vec4((p0 + offset) / uViewportSize * 2.0 - 1.0, 
                        gl_in[0].gl_Position.zw);
    gColor = vColor[0];
    gTexCoord = vTexCoord[0];
    EmitVertex();
    
    // 左下顶点
    gl_Position = vec4((p0 - offset) / uViewportSize * 2.0 - 1.0, 
                        gl_in[0].gl_Position.zw);
    gColor = vColor[0];
    gTexCoord = vTexCoord[0];
    EmitVertex();
    
    // 右上顶点
    gl_Position = vec4((p1 + offset) / uViewportSize * 2.0 - 1.0, 
                        gl_in[1].gl_Position.zw);
    gColor = vColor[1];
    gTexCoord = vTexCoord[1];
    EmitVertex();
    
    // 右下顶点
    gl_Position = vec4((p1 - offset) / uViewportSize * 2.0 - 1.0, 
                        gl_in[1].gl_Position.zw);
    gColor = vColor[1];
    gTexCoord = vTexCoord[1];
    EmitVertex();
    
    EndPrimitive();
}
)";

// 实体片段着色器
// 支持实线和虚线两种模式
static const char* ENTITY_FRAGMENT_SHADER = R"(
#version 330 core

uniform sampler1D uDashTexture;   // 虚线纹理
uniform int uIsDashed;            // 是否虚线模式（0=实线，1=虚线）
uniform float uDashScale;         // 虚线缩放因子（1/周期）
uniform int uUseVertexColor;      // 是否使用顶点颜色（1=是，0=使用uniform颜色）
uniform vec4 uColor;              // uniform颜色

in vec4 gColor;      // 来自几何着色器的颜色
in float gTexCoord;  // 来自几何着色器的纹理坐标
out vec4 FragColor;  // 输出颜色

void main() {
    // 选择颜色来源
    vec4 color;
    if (uUseVertexColor == 1) {
        color = gColor;
    } else {
        color = uColor;
    }
    
    // 虚线/实线处理
    if (uIsDashed == 0) {
        FragColor = color;
    } else {
        // 虚线模式：采样虚线纹理
        float texPos = gTexCoord * uDashScale;
        float alpha = texture(uDashTexture, texPos).r;
        if (alpha < 0.5) discard;  // 透明部分丢弃
        FragColor = color;
    }
}
)";

// 全屏四边形顶点着色器
// 用于将FBO纹理渲染到屏幕
static const char* QUAD_VERTEX_SHADER = R"(
#version 330 core

layout (location = 0) in vec2 aPos;      // NDC坐标（-1到1）
layout (location = 1) in vec2 aTexCoord; // 纹理坐标（0到1）

out vec2 vTexCoord;  // 传递到片段着色器的纹理坐标

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

// 全屏四边形片段着色器
// 简单采样FBO纹理
static const char* QUAD_FRAGMENT_SHADER = R"(
#version 330 core

uniform sampler2D uTexture;  // FBO颜色纹理

in vec2 vTexCoord;    // 来自顶点着色器的纹理坐标
out vec4 FragColor;   // 输出颜色

void main() {
    FragColor = texture(uTexture, vTexCoord);
}
)";

// ============================================================================
// 构造函数与析构函数
// ============================================================================

// 构造函数：初始化所有成员变量为默认值
EntityRenderer::EntityRenderer()
    : m_fbo(0)
    , m_colorTexture(0)
    , m_depthStencilBuffer(0)
    , m_windowWidth(0)
    , m_windowHeight(0)
    , m_vao(0)
    , m_vbo(0)
    , m_mvpLocation(-1)
    , m_isDashedLocation(-1)
    , m_dashScaleLocation(-1)
    , m_useVertexColorLocation(-1)
    , m_quadVAO(0)
    , m_quadVBO(0)
    , m_quadTextureLocation(-1)
    , m_dashTexture(0)
{
}

// 析构函数：清理所有资源
EntityRenderer::~EntityRenderer() {
    cleanup();
}

// ============================================================================
// 初始化与清理
// ============================================================================

// 初始化渲染器
// 创建着色器程序、VAO/VBO、虚线纹理
bool EntityRenderer::initialize() {
    // 创建实体着色器（顶点+几何+片段）
    m_entityShader.setShaderSource(ENTITY_VERTEX_SHADER, ENTITY_FRAGMENT_SHADER, ENTITY_GEOMETRY_SHADER);
    
    GLuint shaderId = m_entityShader.getShaderId();
    if (shaderId == 0) {
        LOG_ERROR("Failed to create entity shader program");
        return false;
    }
    
    // 获取实体着色器uniform位置
    m_mvpLocation = glGetUniformLocation(shaderId, "uMVP");
    m_isDashedLocation = glGetUniformLocation(shaderId, "uIsDashed");
    m_dashScaleLocation = glGetUniformLocation(shaderId, "uDashScale");
    m_useVertexColorLocation = glGetUniformLocation(shaderId, "uUseVertexColor");
    
    // 创建四边形着色器（顶点+片段）
    m_quadShader.setShaderSource(QUAD_VERTEX_SHADER, QUAD_FRAGMENT_SHADER);
    
    GLuint quadShaderId = m_quadShader.getShaderId();
    if (quadShaderId == 0) {
        LOG_ERROR("Failed to create quad shader program");
        return false;
    }
    
    // 获取四边形着色器uniform位置
    m_quadTextureLocation = glGetUniformLocation(quadShaderId, "uTexture");
    
    // 创建实体渲染VAO/VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // 预分配足够大的缓冲区（100000个顶点）
    glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
    
    // 设置顶点属性
    // location 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // location 1: color (vec4)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
    
    // location 2: texCoord (float)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 7));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // 初始化全屏四边形VAO和虚线纹理
    setupQuadVAO();
    initDashTexture();
    
    // 预分配顶点缓冲区容量
    m_vertices.reserve(100000);
    
    LOG_INFO("EntityRenderer initialized successfully");
    return true;
}

// 清理所有OpenGL资源
void EntityRenderer::cleanup() {
    // 清理FBO相关资源
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    
    if (m_colorTexture) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    
    if (m_depthStencilBuffer) {
        glDeleteRenderbuffers(1, &m_depthStencilBuffer);
        m_depthStencilBuffer = 0;
    }
    
    // 清理实体VAO/VBO
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    
    // 清理四边形VAO/VBO
    if (m_quadVAO) {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    
    if (m_quadVBO) {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
    
    // 清理虚线纹理
    if (m_dashTexture) {
        glDeleteTextures(1, &m_dashTexture);
        m_dashTexture = 0;
    }
}

// ============================================================================
// 窗口尺寸更新与FBO管理
// ============================================================================

// 更新窗口尺寸
void EntityRenderer::updateWindowSize(int width, int height) {
    ensureFBOSize(width, height);
}

// 确保FBO尺寸足够大
// 如果窗口尺寸变化，重新创建FBO
void EntityRenderer::ensureFBOSize(int width, int height) {
    if (width <= 0 || height <= 0) {
        // 最小化就会进入这里，直接返回
        return;
    }
    
    // 尺寸未变化，无需重建
    if (width == m_windowWidth && height == m_windowHeight) {
        return;
    }
    
    // 首次创建FBO资源
    if (m_fbo == 0) {
        glGenFramebuffers(1, &m_fbo);
        glGenTextures(1, &m_colorTexture);
        glGenRenderbuffers(1, &m_depthStencilBuffer);
    }
    
    // 创建颜色纹理附件
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // 创建深度/模板缓冲附件
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    
    // 绑定附件到FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                           GL_TEXTURE_2D, m_colorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, m_depthStencilBuffer);
    
    // 检查FBO完整性
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer is not complete! Status: {}", status);
    } else {
        LOG_INFO("EntityRenderer FBO created successfully: {}x{}", width, height);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // 更新记录的窗口尺寸
    m_windowWidth = width;
    m_windowHeight = height;
}

// ============================================================================
// 初始化辅助方法
// ============================================================================

// 初始化虚线纹理
// 创建一维纹理，4白4透明模式
void EntityRenderer::initDashTexture() {
    // 4白4透明虚线模式
    unsigned char pattern[8] = {255, 255, 255, 255, 0, 0, 0, 0};
    
    glGenTextures(1, &m_dashTexture);
    glBindTexture(GL_TEXTURE_1D, m_dashTexture);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RED, 8, 0, GL_RED, GL_UNSIGNED_BYTE, pattern);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

// 设置全屏四边形VAO
// 用于将FBO纹理渲染到屏幕
void EntityRenderer::setupQuadVAO() {
    // 四边形顶点数据：位置(xy) + 纹理坐标(uv)
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,  // 左上
        -1.0f, -1.0f,  0.0f, 0.0f,  // 左下
         1.0f, -1.0f,  1.0f, 0.0f,  // 右下
         1.0f,  1.0f,  1.0f, 1.0f   // 右上
    };
    
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // location 0: position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // location 1: texCoord (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// ============================================================================
// 顶点管理
// ============================================================================

// 清空顶点缓冲
void EntityRenderer::clearVertices() {
    m_vertices.clear();
}

// 添加顶点到缓冲区
void EntityRenderer::addVertex(const glm::vec3& pos, const glm::vec4& color, float texCoord) {
    m_vertices.push_back({pos, color, texCoord});
}

// 上传顶点数据到GPU
void EntityRenderer::flushVertices() {
    if (m_vertices.empty()) {
        return;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data());
}

// ============================================================================
// 渲染辅助方法
// ============================================================================

// 设置虚线模式
// isDashed: 是否启用虚线
// period: 虚线周期（像素数）
void EntityRenderer::setDashedMode(bool isDashed, float period) {
    glUniform1i(m_isDashedLocation, isDashed ? 1 : 0);
    glUniform1f(m_dashScaleLocation, 1.0f / period);
}

// 渲染几何体
// 收集实体顶点并绘制
void EntityRenderer::renderGeometry(const glm::mat4& mvp) {
    // TODO: 需要考虑透明实体的混合方式
    
    // TODO: 实现实体渲染（待 Entity 类和 Document::getEntities() 接口完成后启用）
    auto& doc = DocManager::getCurrentDocument();
    // const auto& entities = doc.getEntities();
    
    // clearVertices();
    
    // // 收集实体顶点
    // for (const auto& entity : entities) {
    //     auto segments = entity->getSegments();
    //     float texCoord = 0.0f;
        
    //     for (const auto& seg : segments) {
    //         float length = glm::length(seg.end - seg.start);
    //         addVertex(seg.start, seg.color, texCoord);
    //         addVertex(seg.end, seg.color, texCoord + length);
    //         texCoord += length;
    //     }
    // }
    
    if (m_vertices.empty())  {
        return;
    }
    
    // 上传顶点数据
    flushVertices();
    
    // 使用实体着色器
    m_entityShader.use();
    glUniformMatrix4fv(m_mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1i(m_isDashedLocation, 0);
    glUniform1f(m_dashScaleLocation, 1.0f);
    glUniform1i(m_useVertexColorLocation, 1);
    
    // 设置线宽（通过几何着色器实现）
    float lineWidth = 1.0f;  // TODO: 从实体获取线宽
    glUniform1f(glGetUniformLocation(m_entityShader.getShaderId(), "uLineWidth"), lineWidth);
    glUniform2f(glGetUniformLocation(m_entityShader.getShaderId(), "uViewportSize"),
                (float)m_windowWidth, (float)m_windowHeight);
    
    // 绑定虚线纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, m_dashTexture);
    glUniform1i(glGetUniformLocation(m_entityShader.getShaderId(), "uDashTexture"), 0);
    
    // 渲染
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);
}

// 渲染实体到FBO
// 在离屏帧缓冲中绘制实体
void EntityRenderer::renderToFBO(const glm::mat4& mvp, 
                                  int viewportLeft, int viewportBottom,
                                  int viewportWidth, int viewportHeight) {
    if (m_fbo == 0) {
        return;
    }
    
    // 绑定FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    
    // 设置视口（参数已经是 OpenGL 屏幕坐标系，Y向上，原点左下角）
    glViewport(viewportLeft, viewportBottom, viewportWidth, viewportHeight);
    
    // 使用裁剪测试限制渲染区域
    glEnable(GL_SCISSOR_TEST);
    glScissor(viewportLeft, viewportBottom, viewportWidth, viewportHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // 透明背景
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    
    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    
    // 禁用混合：绘制到FBO时不需要混合，避免透明物体被混合两次
    // 后续支持透明实体绘制时，FBO内部应使用预乘alpha混合确保正确性
    glDisable(GL_BLEND);
    
    // 渲染几何体
    renderGeometry(mvp);
    
    // 恢复默认帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// 将FBO纹理渲染到屏幕
// 使用全屏四边形绘制FBO的颜色纹理
void EntityRenderer::renderTextureToScreen() {
    if (m_colorTexture == 0) {
        return;
    }
    
    // 绑定默认帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_windowWidth, m_windowHeight);
    
    // 禁用深度测试，启用混合
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 使用四边形着色器
    m_quadShader.use();
    
    // 绑定FBO颜色纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glUniform1i(m_quadTextureLocation, 0);
    
    // 绘制全屏四边形
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

// ============================================================================
// 主绘制方法
// ============================================================================

// 绘制所有实体
// 主入口函数，协调FBO渲染和屏幕合成
void EntityRenderer::drawEntities() {
    if (m_fbo == 0) {
        LOG_WARNING("EntityRenderer::drawEntities() - FBO not created yet");
        return;
    }
    
    // 获取当前文档和变换管理器
    auto& doc = DocManager::getCurrentDocument();
    auto& transformManager = doc.getTransformManager();
    
    // 获取画布区域（鼠标坐标系，Y向下，原点左上角）
    int canvasLeft, canvasTop, canvasRight, canvasBottom;
    transformManager.getViewport().getViewport(canvasLeft, canvasTop, canvasRight, canvasBottom);
    
    int canvasWidth = canvasRight - canvasLeft;
    int canvasHeight = canvasBottom - canvasTop;
    
    if (canvasWidth <= 0 || canvasHeight <= 0) {
        LOG_WARNING("EntityRenderer::drawEntities() - Invalid canvas size: {}x{}", canvasWidth, canvasHeight);
        return;
    }
    
    // 坐标系转换：鼠标坐标系（Y向下）-> OpenGL屏幕坐标系（Y向上）
    int glViewportY = m_windowHeight - canvasTop - canvasHeight;
    
    // 获取MVP矩阵
    glm::mat4 mvp = transformManager.getMVP();
    
    // 渲染到FBO，然后合成到屏幕
    renderToFBO(mvp, canvasLeft, glViewportY, canvasWidth, canvasHeight);
    renderTextureToScreen();
}

} // namespace tch
