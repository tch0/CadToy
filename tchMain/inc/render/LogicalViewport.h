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
    
    // 缩放功能，以可绘制区域中心为中心
    void zoomIn(double scaleFactor = 1.25);
    void zoomOut(double scaleFactor = 0.8);
    
    // 设置窗口大小
    void setWindowSize(int width, int height);
    
    // 设置可绘制区域边界
    void setDrawableArea(int left, int top, int right, int bottom);
    
    // 获取逻辑视口边界
    glm::dvec2 getLogicMin() const;
    glm::dvec2 getLogicMax() const;
    
    // 获取缩放因子
    double getZoomFactor() const;
    
    // 获取可绘制区域大小
    glm::ivec2 getDrawableAreaSize() const;
    
    // 检查屏幕坐标是否在可绘制区域内
    bool isPointInDrawableArea(const glm::vec2& screenPos) const;
    
    // 获取可绘制区域边界
    void getDrawableArea(int& left, int& top, int& right, int& bottom) const;
    
    // 计算可绘制区域中心的逻辑坐标
    glm::dvec2 getWindowCenterLogic() const;

private:
    // 窗口大小
    int m_windowWidth;
    int m_windowHeight;
    
    // 可绘制区域边界
    int m_drawableLeft;
    int m_drawableTop;
    int m_drawableRight;
    int m_drawableBottom;
    
    // 逻辑视口边界
    glm::dvec2 m_logicMin;
    glm::dvec2 m_logicMax;
    
    // 缩放因子
    double m_zoomFactor;
    
    // 计算可绘制区域的大小
    glm::ivec2 getDrawableAreaSizeInternal() const;
};

} // namespace tch
