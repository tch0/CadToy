#include "render/LogicalViewport.h"

namespace tch {

void LogicalViewport::initialize(int windowWidth, int windowHeight) {
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;
    
    // 计算窗口宽高比
    double aspectRatio = static_cast<double>(windowWidth) / windowHeight;
    
    // 初始化逻辑视口边界，根据窗口宽高比调整
    // 确保逻辑视口的宽高比与窗口匹配
    double logicHeight = 200.0;
    double logicWidth = logicHeight * aspectRatio;
    
    m_logicMin = glm::dvec2(-logicWidth / 2.0, -logicHeight / 2.0);
    m_logicMax = glm::dvec2(logicWidth / 2.0, logicHeight / 2.0);
    
    // 初始化缩放因子
    m_zoomFactor = 1.0;
}

void LogicalViewport::setWindowSize(int width, int height) {
    // 保存旧的窗口大小
    int oldWidth = m_windowWidth;
    int oldHeight = m_windowHeight;
    
    // 更新窗口大小
    m_windowWidth = width;
    m_windowHeight = height;
    
    // 计算旧的和新的窗口宽高比
    double oldAspectRatio = static_cast<double>(oldWidth) / oldHeight;
    double newAspectRatio = static_cast<double>(width) / height;
    
    // 只有当窗口大小发生变化且宽高比不同时，才调整逻辑视口
    if (oldWidth > 0 && oldHeight > 0 && fabs(oldAspectRatio - newAspectRatio) > 0.001) {
        // 计算逻辑视口的中心和大小
        glm::dvec2 center = getWindowCenterLogic();
        double logicWidth = m_logicMax.x - m_logicMin.x;
        double logicHeight = m_logicMax.y - m_logicMin.y;
        
        // 计算当前逻辑视口的宽高比
        double logicAspectRatio = logicWidth / logicHeight;
        
        // 调整逻辑视口的边界，保持中心不变，使宽高比与新窗口匹配
        if (newAspectRatio > logicAspectRatio) {
            // 新窗口更宽，增加逻辑视口的宽度
            double newLogicWidth = logicHeight * newAspectRatio;
            double halfWidth = newLogicWidth / 2.0;
            double halfHeight = logicHeight / 2.0;
            
            m_logicMin = glm::dvec2(center.x - halfWidth, center.y - halfHeight);
            m_logicMax = glm::dvec2(center.x + halfWidth, center.y + halfHeight);
        } else {
            // 新窗口更高，增加逻辑视口的高度
            double newLogicHeight = logicWidth / newAspectRatio;
            double halfWidth = logicWidth / 2.0;
            double halfHeight = newLogicHeight / 2.0;
            
            m_logicMin = glm::dvec2(center.x - halfWidth, center.y - halfHeight);
            m_logicMax = glm::dvec2(center.x + halfWidth, center.y + halfHeight);
        }
    }
}

glm::dvec3 LogicalViewport::screenToLogic(const glm::vec2& screenPos) const {
    // 计算屏幕坐标到逻辑坐标的转换
    double x = m_logicMin.x + (screenPos.x / m_windowWidth) * (m_logicMax.x - m_logicMin.x);
    // 屏幕y轴向下，逻辑y轴向上，所以需要反转y坐标
    double y = m_logicMin.y + ((m_windowHeight - screenPos.y) / m_windowHeight) * (m_logicMax.y - m_logicMin.y);
    double z = 0.0; // 目前z坐标始终为0
    
    return glm::dvec3(x, y, z);
}

glm::vec2 LogicalViewport::logicToScreen(const glm::dvec3& logicPos) const {
    // 计算逻辑坐标到屏幕坐标的转换
    double x = (logicPos.x - m_logicMin.x) / (m_logicMax.x - m_logicMin.x) * m_windowWidth;
    // 逻辑y轴向上，屏幕y轴向下，所以需要反转y坐标
    double y = m_windowHeight - (logicPos.y - m_logicMin.y) / (m_logicMax.y - m_logicMin.y) * m_windowHeight;
    
    return glm::vec2(static_cast<float>(x), static_cast<float>(y));
}

void LogicalViewport::zoomIn(const glm::vec2& screenPos, double scaleFactor) {
    // 将屏幕坐标转换为逻辑坐标
    glm::dvec3 logicPos = screenToLogic(screenPos);
    
    // 应用缩放
    m_zoomFactor *= scaleFactor;
    
    // 调整逻辑视口边界，以指定点为中心缩放
    glm::dvec2 center = glm::dvec2(logicPos.x, logicPos.y);
    glm::dvec2 deltaMin = m_logicMin - center;
    glm::dvec2 deltaMax = m_logicMax - center;
    
    m_logicMin = center + deltaMin * scaleFactor;
    m_logicMax = center + deltaMax * scaleFactor;
}

void LogicalViewport::zoomOut(const glm::vec2& screenPos, double scaleFactor) {
    // 将屏幕坐标转换为逻辑坐标
    glm::dvec3 logicPos = screenToLogic(screenPos);
    
    // 应用缩放
    m_zoomFactor *= scaleFactor;
    
    // 调整逻辑视口边界，以指定点为中心缩放
    glm::dvec2 center = glm::dvec2(logicPos.x, logicPos.y);
    glm::dvec2 deltaMin = m_logicMin - center;
    glm::dvec2 deltaMax = m_logicMax - center;
    
    m_logicMin = center + deltaMin * scaleFactor;
    m_logicMax = center + deltaMax * scaleFactor;
}

void LogicalViewport::zoomIn(double scaleFactor) {
    // 以窗口中心为缩放中心
    glm::dvec2 center = getWindowCenterLogic();
    
    // 应用缩放
    m_zoomFactor *= scaleFactor;
    
    // 调整逻辑视口边界
    glm::dvec2 deltaMin = m_logicMin - center;
    glm::dvec2 deltaMax = m_logicMax - center;
    
    m_logicMin = center + deltaMin * scaleFactor;
    m_logicMax = center + deltaMax * scaleFactor;
}

void LogicalViewport::zoomOut(double scaleFactor) {
    // 以窗口中心为缩放中心
    glm::dvec2 center = getWindowCenterLogic();
    
    // 应用缩放
    m_zoomFactor *= scaleFactor;
    
    // 调整逻辑视口边界
    glm::dvec2 deltaMin = m_logicMin - center;
    glm::dvec2 deltaMax = m_logicMax - center;
    
    m_logicMin = center + deltaMin * scaleFactor;
    m_logicMax = center + deltaMax * scaleFactor;
}

glm::dvec2 LogicalViewport::getLogicMin() const {
    return m_logicMin;
}

glm::dvec2 LogicalViewport::getLogicMax() const {
    return m_logicMax;
}

double LogicalViewport::getZoomFactor() const {
    return m_zoomFactor;
}

glm::dvec2 LogicalViewport::getWindowCenterLogic() const {
    // 计算窗口中心的逻辑坐标
    return (m_logicMin + m_logicMax) * 0.5;
}

} // namespace tch
