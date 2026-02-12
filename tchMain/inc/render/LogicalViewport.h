#pragma once

#include <glm/glm.hpp>

namespace tch {

class LogicalViewport {
public:
    // 初始化逻辑视口
    void initialize(int windowWidth, int windowHeight);
    
    // 屏幕坐标到逻辑坐标的转换
    glm::dvec3 screenToLogic(const glm::vec2& screenPos) const;
    
    // 逻辑坐标到屏幕坐标的转换
    glm::vec2 logicToScreen(const glm::dvec3& logicPos) const;
    
    // 缩放功能，以指定点为中心
    void zoomIn(const glm::vec2& screenPos, double scaleFactor = 1.25);
    void zoomOut(const glm::vec2& screenPos, double scaleFactor = 0.8);
    
    // 缩放功能，以窗口中心为中心
    void zoomIn(double scaleFactor = 1.25);
    void zoomOut(double scaleFactor = 0.8);
    
    // 设置窗口大小
    void setWindowSize(int width, int height);
    
    // 获取逻辑视口边界
    glm::dvec2 getLogicMin() const;
    glm::dvec2 getLogicMax() const;
    
    // 获取缩放因子
    double getZoomFactor() const;
    
private:
    // 窗口大小
    int m_windowWidth;
    int m_windowHeight;
    
    // 逻辑视口边界
    glm::dvec2 m_logicMin;
    glm::dvec2 m_logicMax;
    
    // 缩放因子
    double m_zoomFactor;
    
    // 计算窗口中心的逻辑坐标
    glm::dvec2 getWindowCenterLogic() const;
};

} // namespace tch
