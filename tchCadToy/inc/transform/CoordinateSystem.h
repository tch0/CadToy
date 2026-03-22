#pragma once

// C++ 标准库

// 第三方库
#include <glm/glm.hpp>

// 项目头文件

namespace tch {

class CoordinateSystem {
public:
    virtual ~CoordinateSystem() = default;
    
    // 坐标转换方法
    virtual glm::dvec3 screenToWorld(const glm::vec2& screenPos) const = 0;
    virtual glm::vec2 worldToScreen(const glm::dvec3& worldPos) const = 0;
    
    // 投影矩阵获取
    virtual glm::dmat4 getProjectionMatrix() const = 0;
    virtual glm::dmat4 getViewMatrix() const = 0;
    virtual glm::dmat4 getModelMatrix() const = 0;
    virtual glm::dmat4 getMVP() const = 0;
};

} // namespace tch
