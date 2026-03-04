#pragma once

#include <glm/glm.hpp>

namespace tch {

class Camera {
public:
    // 构造函数
    Camera();
    
    // 位置和旋转
    void setPosition(const glm::dvec3& position);
    void setRotation(const glm::dvec3& rotation);
    void setScale(double scale);
    
    // 获取属性
    glm::dvec3 getPosition() const;
    glm::dvec3 getRotation() const;
    double getScale() const;
    
    // 视图矩阵
    glm::dmat4 getViewMatrix() const;
    
    // 相机操作
    void move(const glm::dvec3& delta);
    void rotate(const glm::dvec3& delta);
    void zoom(double factor);
    
private:
    glm::dvec3 m_position;
    glm::dvec3 m_rotation;
    double m_scale;
    mutable glm::dmat4 m_viewMatrix;
    mutable bool m_viewMatrixDirty;
};

} // namespace tch
