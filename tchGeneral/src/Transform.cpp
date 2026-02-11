#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tch {

// 平移单个图形
void Transform::translate(Shape& shape, const glm::vec2& delta) {
    shape.translate(delta);
}

// 平移多个图形
void Transform::translate(const std::vector<std::shared_ptr<Shape>>& shapes, const glm::vec2& delta) {
    for (const auto& shape : shapes) {
        shape->translate(delta);
    }
}

// 旋转单个图形
void Transform::rotate(Shape& shape, float angle, const glm::vec2& center) {
    shape.rotate(angle, center);
}

// 旋转多个图形
void Transform::rotate(const std::vector<std::shared_ptr<Shape>>& shapes, float angle, const glm::vec2& center) {
    for (const auto& shape : shapes) {
        shape->rotate(angle, center);
    }
}

// 缩放单个图形
void Transform::scale(Shape& shape, float factor, const glm::vec2& center) {
    shape.scale(factor, center);
}

// 缩放多个图形
void Transform::scale(const std::vector<std::shared_ptr<Shape>>& shapes, float factor, const glm::vec2& center) {
    for (const auto& shape : shapes) {
        shape->scale(factor, center);
    }
}

// 计算变换矩阵
glm::mat3 Transform::calculateTransformMatrix(const glm::vec2& translation, float rotation, float scale) {
    // 先缩放，再旋转，最后平移
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(translation, 0.0f));
    transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, glm::vec3(scale, scale, 1.0f));
    return glm::mat3(transform);
}

// 应用变换矩阵到点
glm::vec2 Transform::applyTransform(const glm::vec2& point, const glm::mat3& transform) {
    glm::vec3 homogeneousPoint(point.x, point.y, 1.0f);
    glm::vec3 transformedPoint = transform * homogeneousPoint;
    return glm::vec2(transformedPoint.x, transformedPoint.y);
}

} // namespace tch