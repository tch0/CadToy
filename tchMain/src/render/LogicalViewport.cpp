#include "render/LogicalViewport.h"

namespace tch {

void LogicalViewport::initialize(int windowWidth, int windowHeight) {
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;
    
    // 初始化逻辑视口边界
    // 默认情况下，逻辑视口与屏幕视口保持一致
    // 但后续可以通过缩放进行调整
    m_logicMin = glm::dvec2(-100.0, -100.0);
    m_logicMax = glm::dvec2(100.0, 100.0);
    
    // 初始化缩放因子
    m_zoomFactor = 1.0;
}

void LogicalViewport::setWindowSize(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

glm::dvec3 LogicalViewport::screenToLogic(const glm::vec2& screenPos) const {
    // 计算屏幕坐标到逻辑坐标的转换
    double x = m_logicMin.x + (screenPos.x / m_windowWidth) * (m_logicMax.x - m_logicMin.x);
    double y = m_logicMin.y + (screenPos.y / m_windowHeight) * (m_logicMax.y - m_logicMin.y);
    double z = 0.0; // 目前z坐标始终为0
    
    return glm::dvec3(x, y, z);
}

glm::vec2 LogicalViewport::logicToScreen(const glm::dvec3& logicPos) const {
    // 计算逻辑坐标到屏幕坐标的转换
    double x = (logicPos.x - m_logicMin.x) / (m_logicMax.x - m_logicMin.x) * m_windowWidth;
    double y = (logicPos.y - m_logicMin.y) / (m_logicMax.y - m_logicMin.y) * m_windowHeight;
    
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
