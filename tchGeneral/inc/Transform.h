#pragma once
#include <glm/glm.hpp>
#include <Geometry.h>
#include <vector>

namespace tch {

// 变换模块
class Transform {
public:
    // 平移单个图形
    static void translate(Shape& shape, const glm::vec2& delta);
    
    // 平移多个图形
    static void translate(const std::vector<std::shared_ptr<Shape>>& shapes, const glm::vec2& delta);
    
    // 旋转单个图形
    static void rotate(Shape& shape, float angle, const glm::vec2& center);
    
    // 旋转多个图形
    static void rotate(const std::vector<std::shared_ptr<Shape>>& shapes, float angle, const glm::vec2& center);
    
    // 缩放单个图形
    static void scale(Shape& shape, float factor, const glm::vec2& center);
    
    // 缩放多个图形
    static void scale(const std::vector<std::shared_ptr<Shape>>& shapes, float factor, const glm::vec2& center);
    
    // 计算变换矩阵
    static glm::mat3 calculateTransformMatrix(const glm::vec2& translation, float rotation, float scale);
    
    // 应用变换矩阵到点
    static glm::vec2 applyTransform(const glm::vec2& point, const glm::mat3& transform);
};

} // namespace tch