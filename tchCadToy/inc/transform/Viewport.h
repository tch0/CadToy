#pragma once

#include <glm/glm.hpp>

namespace tch {

class Viewport {
public:
    // 初始化
    void initialize(int width, int height);
    
    // 设置参数
    void setViewport(int left, int top, int right, int bottom);
    
    // 获取参数
    void getViewport(int& left, int& top, int& right, int& bottom) const;
    glm::ivec2 getViewportSize() const;
    
    // 投影矩阵
    glm::dmat4 getProjectionMatrix() const;
    
private:
    int m_viewportLeft;
    int m_viewportTop;
    int m_viewportRight;
    int m_viewportBottom;
    mutable glm::dmat4 m_projectionMatrix;
    mutable bool m_projectionMatrixDirty;
};

} // namespace tch
