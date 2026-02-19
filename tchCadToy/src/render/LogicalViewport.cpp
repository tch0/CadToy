#include "render/LogicalViewport.h"

namespace tch {

void LogicalViewport::initialize(int windowWidth, int windowHeight) {
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;
    
    // 初始化可绘制区域为整个窗口
    m_drawableLeft = 0;
    m_drawableTop = 0;
    m_drawableRight = windowWidth;
    m_drawableBottom = windowHeight;
    
    // 计算可绘制区域的大小
    glm::ivec2 drawableSize = getDrawableAreaSizeInternal();
    int drawableWidth = drawableSize.x;
    int drawableHeight = drawableSize.y;
    
    // 计算可绘制区域宽高比
    double aspectRatio = drawableWidth > 0 ? static_cast<double>(drawableWidth) / drawableHeight : 1.0;
    
    // 初始化逻辑视口边界，根据可绘制区域宽高比调整
    // 确保逻辑视口的宽高比与可绘制区域匹配
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
    
    // 计算可绘制区域的大小
    glm::ivec2 drawableSize = getDrawableAreaSizeInternal();
    int drawableWidth = drawableSize.x;
    int drawableHeight = drawableSize.y;
    
    // 计算旧的和新的可绘制区域宽高比
    double oldAspectRatio = oldWidth > 0 ? static_cast<double>(oldWidth) / oldHeight : 1.0;
    double newAspectRatio = drawableWidth > 0 ? static_cast<double>(drawableWidth) / drawableHeight : 1.0;
    
    // 只有当窗口大小发生变化且宽高比不同时，才调整逻辑视口
    if (oldWidth > 0 && oldHeight > 0 && fabs(oldAspectRatio - newAspectRatio) > 0.001) {
        // 计算逻辑视口的中心和大小
        glm::dvec2 center = getWindowCenterLogic();
        double logicWidth = m_logicMax.x - m_logicMin.x;
        double logicHeight = m_logicMax.y - m_logicMin.y;
        
        // 计算当前逻辑视口的宽高比
        double logicAspectRatio = logicWidth / logicHeight;
        
        // 调整逻辑视口的边界，保持中心不变，使宽高比与新可绘制区域匹配
        if (newAspectRatio > logicAspectRatio) {
            // 新可绘制区域更宽，增加逻辑视口的宽度
            double newLogicWidth = logicHeight * newAspectRatio;
            double halfWidth = newLogicWidth / 2.0;
            double halfHeight = logicHeight / 2.0;
            
            m_logicMin = glm::dvec2(center.x - halfWidth, center.y - halfHeight);
            m_logicMax = glm::dvec2(center.x + halfWidth, center.y + halfHeight);
        } else {
            // 新可绘制区域更高，增加逻辑视口的高度
            double newLogicHeight = logicWidth / newAspectRatio;
            double halfWidth = logicWidth / 2.0;
            double halfHeight = newLogicHeight / 2.0;
            
            m_logicMin = glm::dvec2(center.x - halfWidth, center.y - halfHeight);
            m_logicMax = glm::dvec2(center.x + halfWidth, center.y + halfHeight);
        }
    }
}

glm::dvec3 LogicalViewport::screenToLogic(const glm::vec2& screenPos) const {
    // 计算可绘制区域的大小
    glm::ivec2 drawableSize = getDrawableAreaSizeInternal();
    int drawableWidth = drawableSize.x;
    int drawableHeight = drawableSize.y;
    
    // 计算屏幕坐标在可绘制区域内的相对位置
    double relativeX = (screenPos.x - m_drawableLeft) / (double)drawableWidth;
    // 屏幕y轴向下，逻辑y轴向上，所以需要反转y坐标
    double relativeY = 1.0 - (screenPos.y - m_drawableTop) / (double)drawableHeight;
    
    // 计算逻辑坐标
    double x = m_logicMin.x + relativeX * (m_logicMax.x - m_logicMin.x);
    double y = m_logicMin.y + relativeY * (m_logicMax.y - m_logicMin.y);
    double z = 0.0; // 目前z坐标始终为0
    
    return glm::dvec3(x, y, z);
}

glm::vec2 LogicalViewport::logicToScreen(const glm::dvec3& logicPos) const {
    // 计算可绘制区域的大小
    glm::ivec2 drawableSize = getDrawableAreaSizeInternal();
    int drawableWidth = drawableSize.x;
    int drawableHeight = drawableSize.y;
    
    // 计算逻辑坐标在逻辑视口内的相对位置
    double relativeX = (logicPos.x - m_logicMin.x) / (m_logicMax.x - m_logicMin.x);
    double relativeY = (logicPos.y - m_logicMin.y) / (m_logicMax.y - m_logicMin.y);
    
    // 计算屏幕坐标
    double x = m_drawableLeft + relativeX * drawableWidth;
    // 逻辑y轴向上，屏幕y轴向下，所以需要反转y坐标
    double y = m_drawableTop + (1.0 - relativeY) * drawableHeight;
    
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
    // 以可绘制区域中心为缩放中心
    glm::vec2 centerScreenPos(
        (m_drawableLeft + m_drawableRight) * 0.5f,
        (m_drawableTop + m_drawableBottom) * 0.5f
    );
    glm::dvec3 centerLogicPos = screenToLogic(centerScreenPos);
    glm::dvec2 center = glm::dvec2(centerLogicPos.x, centerLogicPos.y);
    
    // 应用缩放
    m_zoomFactor *= scaleFactor;
    
    // 调整逻辑视口边界
    glm::dvec2 deltaMin = m_logicMin - center;
    glm::dvec2 deltaMax = m_logicMax - center;
    
    m_logicMin = center + deltaMin * scaleFactor;
    m_logicMax = center + deltaMax * scaleFactor;
}

void LogicalViewport::zoomOut(double scaleFactor) {
    // 以可绘制区域中心为缩放中心
    glm::vec2 centerScreenPos(
        (m_drawableLeft + m_drawableRight) * 0.5f,
        (m_drawableTop + m_drawableBottom) * 0.5f
    );
    glm::dvec3 centerLogicPos = screenToLogic(centerScreenPos);
    glm::dvec2 center = glm::dvec2(centerLogicPos.x, centerLogicPos.y);
    
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

void LogicalViewport::setDrawableArea(int left, int top, int right, int bottom) {
    // 确保顶部边界小于底部边界（屏幕y轴向下）
    if (top > bottom) {
        std::swap(top, bottom);
    }
    
    // 确保左边界小于右边界
    if (left > right) {
        std::swap(left, right);
    }
    
    m_drawableLeft = left;
    m_drawableTop = top;
    m_drawableRight = right;
    m_drawableBottom = bottom;
    
    // 计算可绘制区域的大小
    glm::ivec2 drawableSize = getDrawableAreaSizeInternal();
    int drawableWidth = drawableSize.x;
    int drawableHeight = drawableSize.y;
    
    // 计算可绘制区域宽高比
    double aspectRatio = drawableWidth > 0 ? static_cast<double>(drawableWidth) / drawableHeight : 1.0;
    
    // 计算当前逻辑视口的宽高比
    double logicWidth = m_logicMax.x - m_logicMin.x;
    double logicHeight = m_logicMax.y - m_logicMin.y;
    double logicAspectRatio = logicWidth / logicHeight;
    
    // 调整逻辑视口的边界，保持中心不变，使宽高比与可绘制区域匹配
    if (drawableWidth > 0 && drawableHeight > 0) {
        glm::dvec2 center = getWindowCenterLogic();
        
        if (aspectRatio > logicAspectRatio) {
            // 可绘制区域更宽，增加逻辑视口的宽度
            double newLogicWidth = logicHeight * aspectRatio;
            double halfWidth = newLogicWidth / 2.0;
            double halfHeight = logicHeight / 2.0;
            
            m_logicMin = glm::dvec2(center.x - halfWidth, center.y - halfHeight);
            m_logicMax = glm::dvec2(center.x + halfWidth, center.y + halfHeight);
        } else {
            // 可绘制区域更高，增加逻辑视口的高度
            double newLogicHeight = logicWidth / aspectRatio;
            double halfWidth = logicWidth / 2.0;
            double halfHeight = newLogicHeight / 2.0;
            
            m_logicMin = glm::dvec2(center.x - halfWidth, center.y - halfHeight);
            m_logicMax = glm::dvec2(center.x + halfWidth, center.y + halfHeight);
        }
    }
}

glm::ivec2 LogicalViewport::getDrawableAreaSize() const {
    return getDrawableAreaSizeInternal();
}

bool LogicalViewport::isPointInDrawableArea(const glm::vec2& screenPos) const {
    // 计算可绘制区域的大小
    glm::ivec2 drawableSize = getDrawableAreaSizeInternal();
    int drawableWidth = drawableSize.x;
    int drawableHeight = drawableSize.y;
    
    // 检查屏幕坐标是否在可绘制区域内
    return !(screenPos.x < m_drawableLeft || screenPos.x > m_drawableRight || 
             screenPos.y < m_drawableTop || screenPos.y > m_drawableBottom ||
             drawableWidth <= 0 || drawableHeight <= 0);
}

glm::dvec2 LogicalViewport::getWindowCenterLogic() const {
    // 计算可绘制区域中心的逻辑坐标
    glm::vec2 centerScreenPos(
        (m_drawableLeft + m_drawableRight) * 0.5f,
        (m_drawableTop + m_drawableBottom) * 0.5f
    );
    glm::dvec3 centerLogicPos = screenToLogic(centerScreenPos);
    return glm::dvec2(centerLogicPos.x, centerLogicPos.y);
}

glm::ivec2 LogicalViewport::getDrawableAreaSizeInternal() const {
    // 计算可绘制区域的大小
    int width = m_drawableRight - m_drawableLeft;
    int height = m_drawableBottom - m_drawableTop;
    return glm::ivec2(std::max(0, width), std::max(0, height));
}

void LogicalViewport::getDrawableArea(int& left, int& top, int& right, int& bottom) const {
    left = m_drawableLeft;
    top = m_drawableTop;
    right = m_drawableRight;
    bottom = m_drawableBottom;
}

// 平移功能，根据逻辑坐标位移调整视口
void LogicalViewport::pan(const glm::dvec2& deltaLogic) {
    // 调整逻辑视口边界，实现平移效果
    // 注意：deltaLogic是鼠标拖动的位移，需要取反，因为鼠标向右拖动时，画布应该向左移动
    m_logicMin -= deltaLogic;
    m_logicMax -= deltaLogic;
}

} // namespace tch
