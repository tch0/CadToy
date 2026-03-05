#include "transform/Viewport.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace tch {

Viewport::Viewport() :
    m_viewportLeft(0),
    m_viewportTop(0),
    m_viewportRight(800),
    m_viewportBottom(600),
    m_projectionMatrixDirty(true) {
}

void Viewport::setViewport(int left, int top, int right, int bottom) {
    // 确保左边界小于右边界
    if (left > right) {
        std::swap(left, right);
    }
    
    // 确保上边界小于下边界（屏幕坐标系y轴向下）
    if (top > bottom) {
        std::swap(top, bottom);
    }
    
    // 只有当视口参数发生变化时才更新并设置脏标记
    if (left != m_viewportLeft || top != m_viewportTop || right != m_viewportRight || bottom != m_viewportBottom) {
        m_viewportLeft = left;
        m_viewportTop = top;
        m_viewportRight = right;
        m_viewportBottom = bottom;
        m_projectionMatrixDirty = true;
    }
}

void Viewport::getViewport(int& left, int& top, int& right, int& bottom) const {
    left = m_viewportLeft;
    top = m_viewportTop;
    right = m_viewportRight;
    bottom = m_viewportBottom;
}

glm::ivec2 Viewport::getViewportSize() const {
    return glm::ivec2(m_viewportRight - m_viewportLeft, m_viewportBottom - m_viewportTop);
}

glm::dmat4 Viewport::getProjectionMatrix() const {
    if (m_projectionMatrixDirty) {
        // 计算视口宽高比
        double aspect = static_cast<double>(m_viewportRight - m_viewportLeft) / 
                      static_cast<double>(m_viewportBottom - m_viewportTop);
        
        // 使用正交投影，适合2D CAD应用
        double orthoSize = 100.0;
        double left = -orthoSize * aspect;
        double right = orthoSize * aspect;
        double bottom = -orthoSize;
        double top = orthoSize;
        
        m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0, 1.0);
        m_projectionMatrixDirty = false;
    }
    return m_projectionMatrix;
}

} // namespace tch
