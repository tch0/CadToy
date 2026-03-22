#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "transform/Camera.h"
#include "transform/CoordinateSystem.h"
#include "transform/Viewport.h"

namespace tch {

class TransformManager : public CoordinateSystem {
public:
    // 坐标转换
    glm::dvec3 screenToWorld(const glm::vec2& screenPos) const override;
    glm::vec2 worldToScreen(const glm::dvec3& worldPos) const override;
    
    // 矩阵获取
    glm::dmat4 getProjectionMatrix() const override;
    glm::dmat4 getViewMatrix() const override;
    glm::dmat4 getModelMatrix() const override;
    glm::dmat4 getMVP() const override;
    
    // 视图操作
    void setViewport(int left, int top, int right, int bottom);
    void zoom(const glm::vec2& screenPos, double factor);
    void zoomIn(const glm::vec2& screenPos, double factor = 1.25);
    void zoomOut(const glm::vec2& screenPos, double factor = 0.8);
    void pan(const glm::vec2& deltaScreen);
    
    // 状态获取
    const Viewport& getViewport() const;
    const Camera& getCamera() const;
    Camera& getCamera();
    
private:
    Viewport m_viewport;
    Camera m_camera;
};

} // namespace tch
