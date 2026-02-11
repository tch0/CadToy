#include "Geometry.h"
#include <GLFW/glfw3.h>
#include <cmath>

namespace tch {

// 点的绘制实现
void Point::draw() const {
    glColor3f(m_color.r, m_color.g, m_color.b);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glVertex2f(m_position.x, m_position.y);
    glEnd();
}

// 直线的绘制实现
void Line::draw() const {
    glColor3f(m_color.r, m_color.g, m_color.b);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(m_start.x, m_start.y);
    glVertex2f(m_end.x, m_end.y);
    glEnd();
}

// 圆的绘制实现
void Circle::draw() const {
    glColor3f(m_color.r, m_color.g, m_color.b);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    
    const int segments = 100;
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * static_cast<float>(i) / static_cast<float>(segments);
        float x = m_center.x + m_radius * cos(angle);
        float y = m_center.y + m_radius * sin(angle);
        glVertex2f(x, y);
    }
    
    glEnd();
}

// 矩形的绘制实现
void Rectangle::draw() const {
    glColor3f(m_color.r, m_color.g, m_color.b);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(m_position.x, m_position.y);
    glVertex2f(m_position.x + m_width, m_position.y);
    glVertex2f(m_position.x + m_width, m_position.y + m_height);
    glVertex2f(m_position.x, m_position.y + m_height);
    glEnd();
}

// 几何工具函数实现
namespace GeometryUtils {
    // 计算两点之间的距离
    float distance(const glm::vec2& p1, const glm::vec2& p2) {
        return glm::distance(p1, p2);
    }
    
    // 计算点到直线的距离
    float distanceToLine(const glm::vec2& point, const Line& line) {
        glm::vec2 A = line.getStart();
        glm::vec2 B = line.getEnd();
        glm::vec2 AP = point - A;
        glm::vec2 AB = B - A;
        
        float t = glm::dot(AP, AB) / glm::dot(AB, AB);
        t = glm::clamp(t, 0.0f, 1.0f);
        
        glm::vec2 closestPoint = A + t * AB;
        return glm::distance(point, closestPoint);
    }
    
    // 计算两条直线的交点
    bool intersectLines(const Line& line1, const Line& line2, glm::vec2& intersection) {
        glm::vec2 A = line1.getStart();
        glm::vec2 B = line1.getEnd();
        glm::vec2 C = line2.getStart();
        glm::vec2 D = line2.getEnd();
        
        glm::vec2 AB = B - A;
        glm::vec2 CD = D - C;
        glm::vec2 AC = C - A;
        
        float denominator = AB.x * CD.y - AB.y * CD.x;
        if (fabs(denominator) < 1e-6) {
            return false; // 直线平行
        }
        
        float t = (AC.x * CD.y - AC.y * CD.x) / denominator;
        float u = (AC.x * AB.y - AC.y * AB.x) / denominator;
        
        if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
            intersection = A + t * AB;
            return true;
        }
        
        return false;
    }
    
    // 计算直线与圆的交点
    std::vector<glm::vec2> intersectLineCircle(const Line& line, const Circle& circle) {
        std::vector<glm::vec2> intersections;
        
        glm::vec2 A = line.getStart();
        glm::vec2 B = line.getEnd();
        glm::vec2 C = circle.getCenter();
        float r = circle.getRadius();
        
        glm::vec2 AB = B - A;
        glm::vec2 AC = C - A;
        
        float a = glm::dot(AB, AB);
        float b = 2.0f * glm::dot(AC, AB);
        float c = glm::dot(AC, AC) - r * r;
        
        float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f) {
            return intersections; // 无交点
        }
        
        discriminant = sqrt(discriminant);
        float t1 = (-b - discriminant) / (2.0f * a);
        float t2 = (-b + discriminant) / (2.0f * a);
        
        if (t1 >= 0.0f && t1 <= 1.0f) {
            intersections.push_back(A + t1 * AB);
        }
        
        if (t2 >= 0.0f && t2 <= 1.0f && t2 != t1) {
            intersections.push_back(A + t2 * AB);
        }
        
        return intersections;
    }
    
    // 计算两个圆的交点
    std::vector<glm::vec2> intersectCircles(const Circle& circle1, const Circle& circle2) {
        std::vector<glm::vec2> intersections;
        
        glm::vec2 C1 = circle1.getCenter();
        glm::vec2 C2 = circle2.getCenter();
        float r1 = circle1.getRadius();
        float r2 = circle2.getRadius();
        
        float d = glm::distance(C1, C2);
        if (d > r1 + r2 || d < fabs(r1 - r2)) {
            return intersections; // 无交点
        }
        
        float a = (r1 * r1 - r2 * r2 + d * d) / (2.0f * d);
        float h = sqrt(r1 * r1 - a * a);
        
        glm::vec2 C = C1 + a * (C2 - C1) / d;
        glm::vec2 H = glm::vec2(-(C2.y - C1.y) * h / d, (C2.x - C1.x) * h / d);
        
        intersections.push_back(C + H);
        if (h > 1e-6) {
            intersections.push_back(C - H);
        }
        
        return intersections;
    }
}

} // namespace tch