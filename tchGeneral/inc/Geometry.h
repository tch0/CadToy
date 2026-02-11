#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace tch {

// 图形类型枚举
enum class ShapeType {
    POINT,
    LINE,
    CIRCLE,
    RECTANGLE
};

// 基础图形类
class Shape {
public:
    virtual ~Shape() = default;
    
    // 获取图形类型
    virtual ShapeType getType() const = 0;
    
    // 平移图形
    virtual void translate(const glm::vec2& delta) = 0;
    
    // 旋转图形
    virtual void rotate(float angle, const glm::vec2& center) = 0;
    
    // 缩放图形
    virtual void scale(float factor, const glm::vec2& center) = 0;
    
    // 绘制图形
    virtual void draw() const = 0;
    
    // 设置颜色
    void setColor(const glm::vec3& color) {
        m_color = color;
    }
    
    // 获取颜色
    glm::vec3 getColor() const {
        return m_color;
    }
    
    // 设置图层
    void setLayer(int layer) {
        m_layer = layer;
    }
    
    // 获取图层
    int getLayer() const {
        return m_layer;
    }

protected:
    glm::vec3 m_color = glm::vec3(1.0f, 1.0f, 1.0f); // 默认白色
    int m_layer = 0; // 默认图层
};

// 点类
class Point : public Shape {
public:
    Point(const glm::vec2& position) : m_position(position) {}
    
    ShapeType getType() const override {
        return ShapeType::POINT;
    }
    
    void translate(const glm::vec2& delta) override {
        m_position += delta;
    }
    
    void rotate(float angle, const glm::vec2& center) override {
        // 计算旋转矩阵
        float cosA = cos(angle);
        float sinA = sin(angle);
        
        // 平移到原点
        glm::vec2 relativePos = m_position - center;
        
        // 旋转
        glm::vec2 rotatedPos = glm::vec2(
            relativePos.x * cosA - relativePos.y * sinA,
            relativePos.x * sinA + relativePos.y * cosA
        );
        
        // 平移回原位置
        m_position = rotatedPos + center;
    }
    
    void scale(float factor, const glm::vec2& center) override {
        // 计算缩放后的位置
        m_position = center + (m_position - center) * factor;
    }
    
    void draw() const override;
    
    // 获取位置
    glm::vec2 getPosition() const {
        return m_position;
    }
    
    // 设置位置
    void setPosition(const glm::vec2& position) {
        m_position = position;
    }

private:
    glm::vec2 m_position;
};

// 直线类
class Line : public Shape {
public:
    Line(const glm::vec2& start, const glm::vec2& end) : m_start(start), m_end(end) {}
    
    ShapeType getType() const override {
        return ShapeType::LINE;
    }
    
    void translate(const glm::vec2& delta) override {
        m_start += delta;
        m_end += delta;
    }
    
    void rotate(float angle, const glm::vec2& center) override {
        // 计算旋转矩阵
        float cosA = cos(angle);
        float sinA = sin(angle);
        
        // 旋转起点
        glm::vec2 relativeStart = m_start - center;
        glm::vec2 rotatedStart = glm::vec2(
            relativeStart.x * cosA - relativeStart.y * sinA,
            relativeStart.x * sinA + relativeStart.y * cosA
        );
        m_start = rotatedStart + center;
        
        // 旋转终点
        glm::vec2 relativeEnd = m_end - center;
        glm::vec2 rotatedEnd = glm::vec2(
            relativeEnd.x * cosA - relativeEnd.y * sinA,
            relativeEnd.x * sinA + relativeEnd.y * cosA
        );
        m_end = rotatedEnd + center;
    }
    
    void scale(float factor, const glm::vec2& center) override {
        // 缩放起点
        m_start = center + (m_start - center) * factor;
        
        // 缩放终点
        m_end = center + (m_end - center) * factor;
    }
    
    void draw() const override;
    
    // 获取起点
    glm::vec2 getStart() const {
        return m_start;
    }
    
    // 获取终点
    glm::vec2 getEnd() const {
        return m_end;
    }
    
    // 设置起点
    void setStart(const glm::vec2& start) {
        m_start = start;
    }
    
    // 设置终点
    void setEnd(const glm::vec2& end) {
        m_end = end;
    }

private:
    glm::vec2 m_start;
    glm::vec2 m_end;
};

// 圆类
class Circle : public Shape {
public:
    Circle(const glm::vec2& center, float radius) : m_center(center), m_radius(radius) {}
    
    ShapeType getType() const override {
        return ShapeType::CIRCLE;
    }
    
    void translate(const glm::vec2& delta) override {
        m_center += delta;
    }
    
    void rotate(float angle, const glm::vec2& center) override {
        // 圆绕任意点旋转，中心会移动
        glm::vec2 relativePos = m_center - center;
        float cosA = cos(angle);
        float sinA = sin(angle);
        glm::vec2 rotatedPos = glm::vec2(
            relativePos.x * cosA - relativePos.y * sinA,
            relativePos.x * sinA + relativePos.y * cosA
        );
        m_center = rotatedPos + center;
    }
    
    void scale(float factor, const glm::vec2& center) override {
        // 缩放半径
        m_radius *= factor;
        
        // 缩放圆心位置
        m_center = center + (m_center - center) * factor;
    }
    
    void draw() const override;
    
    // 获取圆心
    glm::vec2 getCenter() const {
        return m_center;
    }
    
    // 获取半径
    float getRadius() const {
        return m_radius;
    }
    
    // 设置圆心
    void setCenter(const glm::vec2& center) {
        m_center = center;
    }
    
    // 设置半径
    void setRadius(float radius) {
        m_radius = radius;
    }

private:
    glm::vec2 m_center;
    float m_radius;
};

// 矩形类
class Rectangle : public Shape {
public:
    Rectangle(const glm::vec2& position, float width, float height) 
        : m_position(position), m_width(width), m_height(height) {}
    
    ShapeType getType() const override {
        return ShapeType::RECTANGLE;
    }
    
    void translate(const glm::vec2& delta) override {
        m_position += delta;
    }
    
    void rotate(float angle, const glm::vec2& center) override {
        // 计算旋转矩阵
        float cosA = cos(angle);
        float sinA = sin(angle);
        
        // 矩形的四个顶点
        std::vector<glm::vec2> vertices = {
            m_position,
            m_position + glm::vec2(m_width, 0.0f),
            m_position + glm::vec2(m_width, m_height),
            m_position + glm::vec2(0.0f, m_height)
        };
        
        // 旋转所有顶点
        for (auto& vertex : vertices) {
            glm::vec2 relativePos = vertex - center;
            glm::vec2 rotatedPos = glm::vec2(
                relativePos.x * cosA - relativePos.y * sinA,
                relativePos.x * sinA + relativePos.y * cosA
            );
            vertex = rotatedPos + center;
        }
        
        // 重新计算矩形的位置和尺寸
        // 这里简化处理，实际应该计算边界框
        m_position = vertices[0];
        m_width = glm::distance(vertices[0], vertices[1]);
        m_height = glm::distance(vertices[0], vertices[3]);
    }
    
    void scale(float factor, const glm::vec2& center) override {
        // 缩放位置
        m_position = center + (m_position - center) * factor;
        
        // 缩放尺寸
        m_width *= factor;
        m_height *= factor;
    }
    
    void draw() const override;
    
    // 获取位置
    glm::vec2 getPosition() const {
        return m_position;
    }
    
    // 获取宽度
    float getWidth() const {
        return m_width;
    }
    
    // 获取高度
    float getHeight() const {
        return m_height;
    }
    
    // 设置位置
    void setPosition(const glm::vec2& position) {
        m_position = position;
    }
    
    // 设置宽度
    void setWidth(float width) {
        m_width = width;
    }
    
    // 设置高度
    void setHeight(float height) {
        m_height = height;
    }

private:
    glm::vec2 m_position; // 左下角位置
    float m_width;
    float m_height;
};

// 几何工具函数
namespace GeometryUtils {
    // 计算两点之间的距离
    float distance(const glm::vec2& p1, const glm::vec2& p2);
    
    // 计算点到直线的距离
    float distanceToLine(const glm::vec2& point, const Line& line);
    
    // 计算两条直线的交点
    bool intersectLines(const Line& line1, const Line& line2, glm::vec2& intersection);
    
    // 计算直线与圆的交点
    std::vector<glm::vec2> intersectLineCircle(const Line& line, const Circle& circle);
    
    // 计算两个圆的交点
    std::vector<glm::vec2> intersectCircles(const Circle& circle1, const Circle& circle2);
}

} // namespace tch