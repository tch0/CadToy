#include "transform/TransformManager.h"

namespace tch {

void TransformManager::initialize(int windowWidth, int windowHeight) {
    m_viewport.initialize(windowWidth, windowHeight);
    m_projectionMatrixDirty = true;
}

glm::dvec3 TransformManager::screenToWorld(const glm::vec2& screenPos) const {
    // 获取视口大小
    int viewportLeft, viewportTop, viewportRight, viewportBottom;
    m_viewport.getViewport(viewportLeft, viewportTop, viewportRight, viewportBottom);
    int viewportWidth = viewportRight - viewportLeft;
    int viewportHeight = viewportBottom - viewportTop;
    
    // 计算屏幕坐标在视口中的相对位置（-1到1）
    double normalizedX = (screenPos.x - viewportLeft) / static_cast<double>(viewportWidth) * 2.0 - 1.0;
    double normalizedY = 1.0 - (screenPos.y - viewportTop) / static_cast<double>(viewportHeight) * 2.0;
    
    // 创建齐次坐标
    glm::dvec4 clipSpace(normalizedX, normalizedY, 0.0, 1.0);
    
    // 计算逆变换矩阵
    glm::dmat4 invMVP = glm::inverse(getMVP());
    
    // 转换到世界空间
    glm::dvec4 worldSpace = invMVP * clipSpace;
    worldSpace /= worldSpace.w;
    
    return glm::dvec3(worldSpace.x, worldSpace.y, worldSpace.z);
}

glm::vec2 TransformManager::worldToScreen(const glm::dvec3& worldPos) const {
    // 转换到裁剪空间
    glm::dvec4 clipSpace = getMVP() * glm::dvec4(worldPos.x, worldPos.y, worldPos.z, 1.0);
    clipSpace /= clipSpace.w;
    
    // 获取视口大小
    int viewportLeft, viewportTop, viewportRight, viewportBottom;
    m_viewport.getViewport(viewportLeft, viewportTop, viewportRight, viewportBottom);
    int viewportWidth = viewportRight - viewportLeft;
    int viewportHeight = viewportBottom - viewportTop;
    
    // 转换到屏幕空间
    float screenX = static_cast<float>((clipSpace.x + 1.0) / 2.0) * viewportWidth + viewportLeft;
    float screenY = static_cast<float>((1.0 - clipSpace.y) / 2.0) * viewportHeight + viewportTop;
    
    return glm::vec2(screenX, screenY);
}

glm::dmat4 TransformManager::getProjectionMatrix() const {
    return m_viewport.getProjectionMatrix();
}

glm::dmat4 TransformManager::getViewMatrix() const {
    return m_camera.getViewMatrix();
}

glm::dmat4 TransformManager::getModelMatrix() const {
    // 模型矩阵，默认为单位矩阵
    return glm::dmat4(1.0);
}

glm::dmat4 TransformManager::getMVP() const {
    return getProjectionMatrix() * getViewMatrix() * getModelMatrix();
}

void TransformManager::setViewport(int left, int top, int right, int bottom) {
    m_viewport.setViewport(left, top, right, bottom);
}

void TransformManager::zoomIn(const glm::vec2& screenPos, double factor) {
    // 获取当前鼠标位置的世界坐标
    glm::dvec3 worldPos = screenToWorld(screenPos);
    
    // 缩放相机
    m_camera.zoom(factor);
    
    // 重新计算鼠标位置的屏幕坐标
    glm::vec2 newScreenPos = worldToScreen(worldPos);
    
    // 计算位移并调整相机位置，保持鼠标位置对应相同的世界坐标
    glm::vec2 deltaScreen = screenPos - newScreenPos;
    glm::dvec3 deltaWorld = screenToWorld(screenPos + deltaScreen) - worldPos;
    m_camera.move(deltaWorld);
}

void TransformManager::zoomOut(const glm::vec2& screenPos, double factor) {
    // 获取当前鼠标位置的世界坐标
    glm::dvec3 worldPos = screenToWorld(screenPos);
    
    // 缩放相机
    m_camera.zoom(factor);
    
    // 重新计算鼠标位置的屏幕坐标
    glm::vec2 newScreenPos = worldToScreen(worldPos);
    
    // 计算位移并调整相机位置，保持鼠标位置对应相同的世界坐标
    glm::vec2 deltaScreen = screenPos - newScreenPos;
    glm::dvec3 deltaWorld = screenToWorld(screenPos + deltaScreen) - worldPos;
    m_camera.move(deltaWorld);
}

void TransformManager::pan(const glm::vec2& deltaScreen) {
    // 计算屏幕位移对应的世界位移
    glm::dvec3 worldPos1 = screenToWorld(glm::vec2(0, 0));
    glm::dvec3 worldPos2 = screenToWorld(deltaScreen);
    glm::dvec3 deltaWorld = worldPos1 - worldPos2;
    
    // 移动相机
    m_camera.move(deltaWorld);
}

const Viewport& TransformManager::getViewport() const {
    return m_viewport;
}

const Camera& TransformManager::getCamera() const {
    return m_camera;
}

Camera& TransformManager::getCamera() {
    return m_camera;
}

} // namespace tch
