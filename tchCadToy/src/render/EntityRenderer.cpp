// 对应头文件
#include "EntityRenderer.h"

// C++ 标准库
#include <cstddef>
#include <vector>

// 第三方库
#include <glm/gtc/type_ptr.hpp>

// 项目头文件
#include "IGraphicsDataCache.h"
#include "Logger.h"
#include "DocManager.h"
#include "GLFuncs.h"
#include "Database.h"
#include "GraphicsEngine.h"
#include "Geometry.h"


namespace tch {

// ==============================================================================================================================
// 着色器实现说明：
//      当前只实现了线框渲染，可以传入任何线段类型进行渲染，足以应对绝大部分情况，三角面渲染则还未实现（有宽度的多段线、某些实体填充等）
//      无线宽版本线框渲染
//          用于绘制所有宽度为1个像素的线框
//          支持选中、暗显，因为预选高亮是加宽像素，所以不支持预选，预选实体总是使用有线宽版本进行绘制
//      有线宽版本线框渲染
//          用于绘制所有宽度大于1个像素的线框
//          完整支持了选中、暗显、预选高亮，支持绘制有线宽实体
//      多种状态：选中实体绘制为虚线、预选高亮则加宽像素、暗显则是显示为一个更暗的颜色（分命令层临时暗显与锁定图层暗显，暗度不同）
//          锁定图层暗显是实体持久状态，其他的属于交互临时状态
//          这些状态可以随意组合，当然无线宽版本中组合预选高亮也没有效果，有线宽版本可以随意组合
//          实际中只有锁定图层暗显+选中状态是合理有效的组合状态，其他状态组合从交互逻辑上来说并不应该实际发生
//              锁定图层实体暗显但不会预选高亮
//              选中实体也不会再预选高亮
//              锁定图层实体也不一般在命令执行时排除，不再参与临时暗显
//              临时暗显一般也和预选互斥(trim中选择预览时临时暗显替代预选高亮)也不再选中高亮(选中后操作时才暗显的比如move)
//      
// ============================================================================
// 线框渲染: 无线宽版本顶点着色器
// 说明: 仅传递位置、颜色和状态标志，不做几何变换以外的特殊处理。
// ============================================================================
static const char* WIREFRAME_VERTEX_SHADER_NO_LW = R"(
#version 330 core

layout (location = 0) in vec3 aPos;       // 世界坐标位置
layout (location = 1) in vec3 aColor;     // 基础颜色 (RGB)
layout (location = 2) in uint aFlags;     // 状态标志 (bit0=预选, bit1=选中, bit2=命令层临时暗显, bit3=锁定图层暗显)
// layout (location = 3) in float aLineWidth; // 线宽（本版本忽略）

uniform mat4 uMVP;                        // 模型-视图-投影矩阵

out vec3 vColor;
out uint vFlags;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
    vFlags = aFlags;
}
)";


// ============================================================================
// 线框渲染: 无线宽版本几何着色器
// 功能: 
//   1. 未选中的线段直接透传 (不生成纹理坐标)
//   2. 选中的线段计算屏幕空间长度，生成纹理坐标 (0 ~ length/period)
//   3. 始终传递 flags 供片段着色器使用
// ============================================================================
static const char* WIREFRAME_GEOMETRY_SHADER_NO_LW = R"(
#version 330 core

layout(lines) in;
layout(line_strip, max_vertices = 2) out;      // 输出仍为线段，不扩展

uniform vec2 uViewportSize;                    // 视口宽高 (像素)
uniform float uDashPeriod;                     // 虚线周期 (像素) 例如 8.0

in vec3 vColor[];
in uint vFlags[];

out vec4 gColor;
out float gTexCoord;                           // 纹理坐标 (选中时 = 屏幕长度/周期，未选中时 = -1)
flat out uint gFlags;

void main() {
    uint flags0 = vFlags[0];
    bool isSelected = ((flags0 >> 1) & 1u) == 1u;

    if (!isSelected) {
        // 未选中：直接透传顶点，不生成纹理坐标
        for (int i = 0; i < 2; ++i) {
            gl_Position = gl_in[i].gl_Position;
            gColor = vec4(vColor[i], 1.0);
            gTexCoord = -1.0;                  // 无效值
            gFlags = vFlags[i];
            EmitVertex();
        }
        EndPrimitive();
        return;
    }

    // 选中状态：计算屏幕空间纹理坐标
    // 步骤1: 将裁剪坐标转换到屏幕像素坐标
    vec2 p0 = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    vec2 p1 = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
    p0 = (p0 + 1.0) * 0.5 * uViewportSize;
    p1 = (p1 + 1.0) * 0.5 * uViewportSize;

    // 步骤2: 屏幕空间长度
    float len = distance(p0, p1);
    float texScale = (len > 0.0) ? len / uDashPeriod : 0.0;

    // 步骤3: 输出两个顶点，纹理坐标分别为 0 和 texScale
    for (int i = 0; i < 2; ++i) {
        gl_Position = gl_in[i].gl_Position;
        gColor = vec4(vColor[i], 1.0);
        gTexCoord = (i == 0) ? 0.0 : texScale;
        gFlags = vFlags[i];
        EmitVertex();
    }
    EndPrimitive();
}
)";

// ============================================================================
// 线框渲染: 无线宽版本片段着色器
// 功能: 
//   1. 根据 flags 中的暗显位进行颜色变暗
//   2. 根据纹理坐标 (有效时) 进行虚线裁剪 (数学方式，无纹理采样)
// ============================================================================
static const char* WIREFRAME_FRAGMENT_SHADER_NO_LW = R"(
#version 330 core

in vec4 gColor;
in float gTexCoord;
flat in uint gFlags;
out vec4 FragColor;

uniform float uDashRatio = 0.5;               // 实线比例 (0.5 = 一半实线一半虚线)

void main() {
    vec4 color = gColor;

    // 暗显处理：区分临时暗显(bit2)和锁定图层暗显(bit3)
    // 临时暗显系数 0.4(更暗)，锁定图层暗显系数 0.6(稍亮)
    // 两者不会同时发生，都有时优先使用锁定图层(0.6)
    bool isTempDimmed = ((gFlags >> 2) & 1u) == 1u;         // kFlagTempDimmed
    bool isLockedLayerDimmed = ((gFlags >> 3) & 1u) == 1u;  // kFlagLockedLayerDimmed
    
    if (isLockedLayerDimmed) {
        // 锁定图层暗显：系数 0.6(稍亮)
        color.rgb = mix(color.rgb, vec3(0.3, 0.3, 0.3), 0.6);
    } else if (isTempDimmed) {
        // 临时暗显：系数 0.4(更暗)
        color.rgb = mix(color.rgb, vec3(0.3, 0.3, 0.3), 0.2);
    }

    // 虚线处理 (仅当 gTexCoord >= 0 时表示选中实体)
    if (gTexCoord >= 0.0) {
        float t = fract(gTexCoord);           // 当前周期内位置 [0,1)
        if (t > uDashRatio) discard;          // 超出实线比例的部分丢弃
    }

    FragColor = color;
}
)";

// ============================================================================
// 线框渲染: 有线宽版本顶点着色器
// 功能: 传递位置、颜色、状态标志、线宽，进行 MVP 变换
// ============================================================================
static const char* WIREFRAME_VERTEX_SHADER_WITH_LW = R"(
#version 330 core

layout (location = 0) in vec3 aPos;          // 世界坐标位置
layout (location = 1) in vec3 aColor;        // 基础颜色 (RGB)
layout (location = 2) in uint aFlags;        // 状态标志 (bit0=预选, bit1=选中, bit2=命令层临时暗显, bit3=锁定图层暗显)
layout (location = 3) in float aLineWidth;   // 线宽 (屏幕像素基础值)

uniform mat4 uMVP;                           // 模型-视图-投影矩阵

out vec3 vColor;
out uint vFlags;
out float vLineWidth;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
    vFlags = aFlags;
    vLineWidth = aLineWidth;
}
)";
// ============================================================================
// 线框渲染: 有线宽版本几何着色器
// 功能: 
//   1. 将线段扩展为带宽度的四边形 (屏幕空间固定像素宽度)
//   2. 预选高亮时线宽增加4~10像素
//   3. 传递暗显标志 (flat) 和中间区域标志 (flat) 以及纹理坐标
//   4. 生成沿线段方向的归一化坐标和屏幕空间纹理坐标 (用于虚线周期)
// ============================================================================
static const char* WIREFRAME_GEOMETRY_SHADER_WITH_LW = R"(
#version 330 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 uViewportSize;      // 视口宽高 (像素)
uniform float uDashPeriod;       // 虚线周期 (像素)，例如 8.0

in vec3 vColor[];
in uint vFlags[];
in float vLineWidth[];

// 输出到片段着色器
out vec4 gColor;                 // 颜色 (插值)
out float gTexCoord;             // 屏幕空间纹理坐标 (范围 0 ~ len/period) (插值)
flat out int gIsSelected;        // 是否选中 (1=选中, 0=未选中)
flat out float gDimmedFactor;    // 暗显系数 (0.0=正常, 0.4=临时暗显, 0.6=锁定图层暗显)

void main() {
    uint flags0 = vFlags[0];
    bool isPreHighlight = ((flags0 >> 0) & 1u) == 1u;
    bool isSelected     = ((flags0 >> 1) & 1u) == 1u;
    bool isTempDimmed   = ((flags0 >> 2) & 1u) == 1u;      // kFlagTempDimmed
    bool isLockedLayerDimmed = ((flags0 >> 3) & 1u) == 1u; // kFlagLockedLayerDimmed

    // 计算最终线宽 (屏幕像素)
    float baseWidth = (vLineWidth[0] + vLineWidth[1]) * 0.5;
    float lineWidth = baseWidth;
    if (isPreHighlight) {
        float increment = max(4.0, baseWidth * 0.4); // 预选最小加粗4像素
        increment = min(increment, 10.0);            // 也可加宽至线宽的40%, 最大10像素
        lineWidth = baseWidth + increment;
    }

    // 将线段端点转换到屏幕像素坐标
    vec2 p0_ndc = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    vec2 p1_ndc = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
    vec2 p0_screen = (p0_ndc + 1.0) * 0.5 * uViewportSize;
    vec2 p1_screen = (p1_ndc + 1.0) * 0.5 * uViewportSize;

    // 屏幕空间长度
    float len = distance(p0_screen, p1_screen);
    float texScale = (len > 0.0) ? len / uDashPeriod : 0.0;

    // 计算垂直于线段方向的单位向量
    vec2 dir = normalize(p1_screen - p0_screen);
    vec2 normal = vec2(-dir.y, dir.x);
    vec2 offset = normal * lineWidth * 0.5;

    // 四个顶点屏幕坐标 (左下、左上、右下、右上)
    vec2 quad_screen[4];
    quad_screen[0] = p0_screen - offset;
    quad_screen[1] = p0_screen + offset;
    quad_screen[2] = p1_screen - offset;
    quad_screen[3] = p1_screen + offset;

    // 屏幕空间纹理坐标 (用于虚线周期)
    float texCoord[4];
    texCoord[0] = 0.0;
    texCoord[1] = 0.0;
    texCoord[2] = texScale;
    texCoord[3] = texScale;

    for (int i = 0; i < 4; ++i) {
        // 屏幕坐标转回 NDC
        vec2 ndc = quad_screen[i] / uViewportSize * 2.0 - 1.0;
        gl_Position = vec4(ndc, 0.0, 1.0);   // 深度简化

        // 颜色: 前两个顶点使用起点颜色，后两个使用终点颜色
        vec3 col = (i < 2) ? vColor[0] : vColor[1];
        gColor = vec4(col, 1.0);

        gTexCoord = texCoord[i];
        gIsSelected = isSelected ? 1 : 0;
        // 暗显系数：锁定图层 0.6(稍亮)，临时暗显 0.4(更暗)，正常 0.0
        // 两者不会同时发生，都有时优先使用锁定图层(0.6)
        if (isLockedLayerDimmed) {
            gDimmedFactor = 0.6;
        } else if (isTempDimmed) {
            gDimmedFactor = 0.2;
        } else {
            gDimmedFactor = 0.0;
        }

        EmitVertex();
    }
    EndPrimitive();
}
)";

// ============================================================================
// 线框渲染: 有线宽版本片段着色器
// 功能: 
//   1. 根据暗显标志降低颜色亮度
//   2. 虚线周期屏幕固定像素
// ============================================================================
static const char* WIREFRAME_FRAGMENT_SHADER_WITH_LW = R"(
#version 330 core

in vec4 gColor;
in float gTexCoord;
flat in int gIsSelected;
flat in float gDimmedFactor;      // 暗显系数 (0.0=正常, 0.4=临时暗显, 0.6=锁定图层暗显)
out vec4 FragColor;

uniform float uDashRatio = 0.5;       // 实线比例 (0~1)

void main() {
    vec4 color = gColor;

    // 暗显处理：使用传递的暗显系数
    // 临时暗显 0.4(更暗)，锁定图层暗显 0.6(稍亮)
    if (gDimmedFactor > 0.0) {
        color.rgb = mix(color.rgb, vec3(0.3, 0.3, 0.3), gDimmedFactor);
    }

    // 选中时整条线显示为虚线(原色，不反色)
    if (gIsSelected == 1) {
        float t = fract(gTexCoord);
        if (t > uDashRatio) discard;
    }

    FragColor = color;
}
)";

// ============================================================================
// 全屏四边形顶点着色器
// 用于将FBO纹理渲染到屏幕
// ============================================================================
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

// ============================================================================
// 全屏四边形片段着色器
// 简单采样FBO纹理
// ============================================================================
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

// 构造函数：初始化所有成员为默认值
EntityRenderer::EntityRenderer() :
    m_fbo(0),
    m_colorTexture(0),
    m_depthStencilBuffer(0),
    m_windowWidth(0),
    m_windowHeight(0),
    m_vao(0),
    m_vbo(0),
    m_noLWProgram(0),
    m_noLWMvpLoc(-1),
    m_noLWViewportSizeLoc(-1),
    m_withLWProgram(0),
    m_withLWMvpLoc(-1),
    m_withLWViewportSizeLoc(-1),
    m_quadVAO(0),
    m_quadVBO(0),
    m_quadProgram(0),
    m_quadTextureLoc(-1)
{
    // 预分配顶点缓冲区内存（避免首次渲染时的内存分配和后续扩容）
    m_noLWVertices.reserve(500000);
    m_withLWVertices.reserve(500000);
}

// 析构函数：清理所有资源
EntityRenderer::~EntityRenderer() {
    cleanup();
}

// ============================================================================
// 初始化与清理
// ============================================================================

// 初始化渲染器
// 创建着色器程序、VAO/VBO
bool EntityRenderer::initialize() {
    // 创建无线宽着色器程序
    m_noLWProgram = createShaderProgramFromSource(
        WIREFRAME_VERTEX_SHADER_NO_LW,
        WIREFRAME_FRAGMENT_SHADER_NO_LW,
        WIREFRAME_GEOMETRY_SHADER_NO_LW
    );
    
    if (m_noLWProgram == 0) {
        LOG_ERROR("Failed to create no line width shader program");
        return false;
    }
    
    // 获取无线宽着色器uniform位置
    m_noLWMvpLoc = glGetUniformLocation(m_noLWProgram, "uMVP");
    m_noLWViewportSizeLoc = glGetUniformLocation(m_noLWProgram, "uViewportSize");
    
    // 设置固定uniform值
    glUseProgram(m_noLWProgram);
    glUniform1f(glGetUniformLocation(m_noLWProgram, "uDashPeriod"), 10.0f); // 虚线周期像素为单位的长度
    glUniform1f(glGetUniformLocation(m_noLWProgram, "uDashRatio"), 0.5f); // 虚线中实线段的比例
    glUseProgram(0);
    
    // 创建有线宽着色器程序
    m_withLWProgram = createShaderProgramFromSource(
        WIREFRAME_VERTEX_SHADER_WITH_LW,
        WIREFRAME_FRAGMENT_SHADER_WITH_LW,
        WIREFRAME_GEOMETRY_SHADER_WITH_LW
    );
    
    if (m_withLWProgram == 0) {
        LOG_ERROR("Failed to create with line width shader program");
        return false;
    }
    
    // 获取有线宽着色器uniform位置
    m_withLWMvpLoc = glGetUniformLocation(m_withLWProgram, "uMVP");
    m_withLWViewportSizeLoc = glGetUniformLocation(m_withLWProgram, "uViewportSize");
    
    // 设置固定uniform值
    glUseProgram(m_withLWProgram);
    glUniform1f(glGetUniformLocation(m_withLWProgram, "uDashPeriod"), 16.0f); // 虚线周期像素为单位的长度，有宽度的虚线周期稍长一点
    glUniform1f(glGetUniformLocation(m_withLWProgram, "uDashRatio"), 0.5f); // 虚线中实线段的比例
    glUseProgram(0);
    
    // 创建四边形着色器程序
    m_quadProgram = createShaderProgramFromSource(QUAD_VERTEX_SHADER, QUAD_FRAGMENT_SHADER);
    
    if (m_quadProgram == 0) {
        LOG_ERROR("Failed to create quad shader program");
        return false;
    }
    
    // 获取四边形着色器uniform位置
    m_quadTextureLoc = glGetUniformLocation(m_quadProgram, "uTexture");
    
    // 创建实体渲染VAO/VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // 预分配足够大的缓冲区（500000个顶点）
    glBufferData(GL_ARRAY_BUFFER, 500000 * sizeof(DataCacheVertex), nullptr, GL_DYNAMIC_DRAW);
    
    // 设置顶点属性（根据新的Vertex结构体，使用glm::vec3）
    // location 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DataCacheVertex), (void*)offsetof(DataCacheVertex, position));
    
    // location 1: color (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DataCacheVertex), (void*)offsetof(DataCacheVertex, color));
    
    // location 2: flags (uint32)
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(DataCacheVertex), (void*)offsetof(DataCacheVertex, flags));
    
    // location 3: lineWeight (float)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(DataCacheVertex), (void*)offsetof(DataCacheVertex, lineWidth));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // 初始化全屏四边形VAO
    setupQuadVAO();
    
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
    
    // 清理着色器程序
    if (m_noLWProgram) {
        glDeleteProgram(m_noLWProgram);
        m_noLWProgram = 0;
    }
    
    if (m_withLWProgram) {
        glDeleteProgram(m_withLWProgram);
        m_withLWProgram = 0;
    }
    
    if (m_quadProgram) {
        glDeleteProgram(m_quadProgram);
        m_quadProgram = 0;
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
// 渲染辅助方法
// ============================================================================

// 渲染几何体
// 从图形数据缓存获取顶点数组并绘制
void EntityRenderer::renderGeometry(const glm::mat4& mvp) {
    // 获取当前文档和图形数据缓存
    auto& doc = DocManager::getCurrentDocument();
    auto* pDataCache = doc.getGraphicsDataCache();
    if (!pDataCache) {
        LOG_WARNING("EntityRenderer::renderGeometry() - No graphics data cache available");
        return;
    }
    
    // 更新视口信息（将屏幕视口转换为世界坐标AABB）
    auto& transformManager = doc.getTransformManager();
    int vpLeft, vpTop, vpRight, vpBottom;
    transformManager.getViewport().getViewport(vpLeft, vpTop, vpRight, vpBottom);
    glm::dvec3 worldMin = transformManager.screenToWorld(glm::vec2(vpLeft, vpBottom));
    glm::dvec3 worldMax = transformManager.screenToWorld(glm::vec2(vpRight, vpTop));
    pDataCache->updateViewport({worldMin, worldMax});

    // 生成图形数据（为脏实体生成缓存）
    GraphicsEngine::getInstance().generate(pDataCache);
    
    // 获取数据库的LWDISPLAY设置
    auto* pDatabase = doc.getDatabase();
    bool lineWeightDisplay = pDatabase ? pDatabase->lineWeightDisplay() : false;
    
    // 清空顶点缓冲区（保留已分配内存）
    m_noLWVertices.clear();
    m_withLWVertices.clear();
    
    // 遍历所有缓存数据，根据类型分发到不同批次
    // 注意：getEntityCacheData 已经返回修饰后的最终数据（包含临时状态）
    pDataCache->iterateAllCacheData([&](ObjectId id, const EntityGraphicsCacheData& cacheData) {
        // 跳过无效数据与不可见实体
        if (cacheData.type == EntityGraphicsCacheData::kInvalidEmptyData ||
            cacheData.type == EntityGraphicsCacheData::kInvisibleEntity) {
            return;
        }

        // 直接使用缓存数据（已经包含预选/选中等临时状态）
        const EntityGraphicsCacheData* pRenderData = &cacheData;
        
        // 根据缓存类型分发到不同批次
        switch (pRenderData->type) {
            case EntityGraphicsCacheData::kAlwaysNoLineWidth:
                // 无线宽批次（始终不显示线宽）
                for (const auto& vertex : pRenderData->vertices) {
                    m_noLWVertices.push_back(vertex);
                }
                break;
                
            case EntityGraphicsCacheData::kAlwaysShowLineWidth:
                // 有线宽批次（始终显示线宽）
                for (const auto& vertex : pRenderData->vertices) {
                    m_withLWVertices.push_back(vertex);
                }
                break;
                
            case EntityGraphicsCacheData::kLineWidthDependsOnLwDisplay:
                // 有线宽但是否显示取决于LWDISPLAY设置
                if (lineWeightDisplay) {
                    // LWDISPLAY=1，显示线宽，使用有线宽渲染
                    for (const auto& vertex : pRenderData->vertices) {
                        m_withLWVertices.push_back(vertex);
                    }
                } else {
                    // LWDISPLAY=0，不显示线宽，使用无线宽渲染
                    for (const auto& vertex : pRenderData->vertices) {
                        m_noLWVertices.push_back(vertex);
                    }
                }
                break;
                
            default:
                // 其他类型（如kInvalidEmptyData）跳过
                break;
        }
    });
    
    // 绑定VAO和VBO（顶点属性已在initialize中设置）
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    // 渲染无线宽批次
    if (!m_noLWVertices.empty()) {
        glUseProgram(m_noLWProgram);
        glUniformMatrix4fv(m_noLWMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform2f(m_noLWViewportSizeLoc, (float)m_windowWidth, (float)m_windowHeight);
        
        // 上传无线宽顶点数据
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_noLWVertices.size() * sizeof(DataCacheVertex), m_noLWVertices.data());
        
        // 绘制无线宽线段
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_noLWVertices.size()));
    }
    
    // 渲染有线宽批次
    if (!m_withLWVertices.empty()) {
        glUseProgram(m_withLWProgram);
        glUniformMatrix4fv(m_withLWMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform2f(m_withLWViewportSizeLoc, (float)m_windowWidth, (float)m_windowHeight);
        
        // 上传有线宽顶点数据
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_withLWVertices.size() * sizeof(DataCacheVertex), m_withLWVertices.data());
        
        // 绘制有线宽线段
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_withLWVertices.size()));
    }
    
    // 清理
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
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
    glUseProgram(m_quadProgram);
    
    // 绑定FBO颜色纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glUniform1i(m_quadTextureLoc, 0);
    
    // 绘制全屏四边形
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    
    glUseProgram(0);
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
