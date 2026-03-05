#include "transform/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tch {

Camera::Camera() : 
    m_position(0.0, 0.0, 0.0),
    m_rotation(0.0, 0.0, 0.0),
    m_scale(1.0),
    m_viewMatrixDirty(true)
{}

void Camera::setPosition(const glm::dvec3& position) {
    m_position = position;
    m_viewMatrixDirty = true;
}

void Camera::setRotation(const glm::dvec3& rotation) {
    m_rotation = rotation;
    m_viewMatrixDirty = true;
}

void Camera::setScale(double scale) {
    m_scale = scale;
    m_viewMatrixDirty = true;
}

glm::dvec3 Camera::getPosition() const {
    return m_position;
}

glm::dvec3 Camera::getRotation() const {
    return m_rotation;
}

double Camera::getScale() const {
    return m_scale;
}

glm::dmat4 Camera::getViewMatrix() const {
    if (m_viewMatrixDirty) {
        // 计算视图矩阵
        m_viewMatrix = glm::dmat4(1.0);
        
        // 应用缩放
        m_viewMatrix = glm::scale(m_viewMatrix, glm::dvec3(m_scale, m_scale, 1.0));
        
        // 应用旋转
        m_viewMatrix = glm::rotate(m_viewMatrix, m_rotation.z, glm::dvec3(0, 0, 1));
        m_viewMatrix = glm::rotate(m_viewMatrix, m_rotation.y, glm::dvec3(0, 1, 0));
        m_viewMatrix = glm::rotate(m_viewMatrix, m_rotation.x, glm::dvec3(1, 0, 0));
        
        // 应用平移
        m_viewMatrix = glm::translate(m_viewMatrix, -m_position);
        
        m_viewMatrixDirty = false;
    }
    return m_viewMatrix;
}

void Camera::move(const glm::dvec3& delta) {
    m_position += delta;
    m_viewMatrixDirty = true;
}

void Camera::rotate(const glm::dvec3& delta) {
    m_rotation += delta;
    m_viewMatrixDirty = true;
}

void Camera::zoom(double factor) {
    m_scale *= factor;
    m_viewMatrixDirty = true;
}

} // namespace tch
