#include "command/CommandLine.h"
#include "input/InputContext.h"
#include "render/Renderer.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace tch {

CommandLine::CommandLine() : 
    m_state(CommandLineState::kWaitForStartPoint),
    m_startPoint(glm::dvec3(0, 0, 0)),
    m_currentPoint(glm::dvec3(0, 0, 0)),
    m_points() {
}

void CommandLine::onUpdate() {
    auto& ctx = InputContext::getInstance();
    
    // 检查是否已经完成
    if (isCompleted()) {
        return;
    }
    
    switch (m_state) {
        case CommandLineState::kWaitForStartPoint:
        {
            // 等待起点输入
            std::vector<std::string> keywords = {"end", "cancel"};
            ctx.waitForPoint("指定起点:", keywords);
            m_state = CommandLineState::kWaitForStartPointInput;
            break;
        }
            
        case CommandLineState::kWaitForStartPointInput:
        {
            // 检查输入状态
            InputStatus status = ctx.getCurrentStatus();
            
            // 无输入，直接返回
            if (status == InputStatus::kNone) {
                return;
            }
            // Esc、Enter和关键字，结束命令
            else if (status == InputStatus::kCanceled || status == InputStatus::kEnterInput || status == InputStatus::kKeywordInput) {
                m_state = CommandLineState::kFinishing;
            }
            // 获取第一点输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_startPoint)) {
                    m_currentPoint = m_startPoint;
                    m_points.push_back(m_startPoint); // 保存起点
                    m_state = CommandLineState::kWaitForNextPoint;
                }
            }
            break;
        }
            
        case CommandLineState::kWaitForNextPoint:
        {
            // 等待下一点输入
            std::vector<std::string> keywords = {"end", "cancel"};
            ctx.waitForPoint("指定下一点 (按Enter结束，按Esc取消):", m_startPoint, keywords);
            m_state = CommandLineState::kWaitForNextPointInput;
            break;
        }
            
        case CommandLineState::kWaitForNextPointInput:
        {
            // 检查输入状态
            InputStatus status = ctx.getCurrentStatus();
            
            // 无输入，直接返回
            if (status == InputStatus::kNone) {
                return;
            }
            // Esc、Enter和关键字，进入结束状态
            else if (status == InputStatus::kCanceled || status == InputStatus::kEnterInput || status == InputStatus::kKeywordInput) {
                m_state = CommandLineState::kFinishing;
            }
            // 获取点输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_currentPoint)) {
                    m_points.push_back(m_currentPoint); // 保存下一点
                    m_startPoint = m_currentPoint;
                    m_state = CommandLineState::kWaitForNextPoint;
                }
            }
            break;
        }
        
        case CommandLineState::kFinishing:
        {
            // 执行统一的结束操作
            finish();
            break;
        }
    }
}

void CommandLine::drawPreview() {
    // 检查渲染器是否初始化
    if (!Renderer::isInitialized()) {
        return;
    }
    
    // 获取变换管理器
    auto& transformManager = Renderer::getTransformManager();
    
    // 保存当前矩阵状态
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    // 设置正交投影，参考drawCursor的实现
    int width, height;
    GLFWwindow* window = glfwGetCurrentContext();
    glfwGetFramebufferSize(window, &width, &height);
    glOrtho(0, width, height, 0, -1, 1);
    
    // 切换到模型视图矩阵
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // 禁用深度测试
    glDisable(GL_DEPTH_TEST);
    
    // 绘制历史线段
    if (m_points.size() > 1) {
        // 使用蓝色绘制历史线段
        glColor3f(0.0f, 0.0f, 1.0f);
        glBegin(GL_LINES);
        for (size_t i = 0; i < m_points.size() - 1; ++i) {
            // 将世界坐标转换为屏幕坐标
            glm::vec2 screenPos1 = transformManager.worldToScreen(m_points[i]);
            glm::vec2 screenPos2 = transformManager.worldToScreen(m_points[i+1]);
            glVertex2f(screenPos1.x, screenPos1.y);
            glVertex2f(screenPos2.x, screenPos2.y);
        }
        glEnd();
    }
    
    // 绘制预览线段
    if (m_state == CommandLineState::kWaitForNextPoint) {
        // 获取当前鼠标位置
        glm::dvec3 mousePos = Renderer::getCursorPosWorld();
        
        // 将世界坐标转换为屏幕坐标
        glm::vec2 startScreenPos = transformManager.worldToScreen(m_startPoint);
        glm::vec2 mouseScreenPos = transformManager.worldToScreen(mousePos);
        
        // 使用黄色绘制预览线段
        glColor3f(1.0f, 1.0f, 0.0f);
        glBegin(GL_LINES);
        glVertex2f(startScreenPos.x, startScreenPos.y);
        glVertex2f(mouseScreenPos.x, mouseScreenPos.y);
        glEnd();
    }
    
    // 恢复矩阵状态
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // 重新启用深度测试
    glEnable(GL_DEPTH_TEST);
}

} // namespace tch