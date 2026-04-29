// 对应头文件
#include "DbArc.h"

// C++ 标准库
#include <cmath>

// 第三方库

// 项目头文件


namespace tch {

DbArc::DbArc(const Geometry::Point& center, double radius,
             double startAngle, double endAngle)
    : m_arc(center, Geometry::Vector(0, 0, 1), radius, startAngle, endAngle) {}

void DbArc::setCenter(const Geometry::Point& c) {
    m_arc.center = c;
    notifyModified();
}

void DbArc::setRadius(double r) {
    m_arc.radius = r;
    notifyModified();
}

void DbArc::setStartAngle(double angle) {
    m_arc.startAngle = angle;
    notifyModified();
}

void DbArc::setEndAngle(double angle) {
    m_arc.endAngle = angle;
    notifyModified();
}

Geometry::AABB DbArc::boundingBox() const {
    double sx = m_arc.center.x + m_arc.radius * std::cos(m_arc.startAngle);
    double sy = m_arc.center.y + m_arc.radius * std::sin(m_arc.startAngle);
    double ex = m_arc.center.x + m_arc.radius * std::cos(m_arc.endAngle);
    double ey = m_arc.center.y + m_arc.radius * std::sin(m_arc.endAngle);
    
    double minX = std::min(sx, ex);
    double maxX = std::max(sx, ex);
    double minY = std::min(sy, ey);
    double maxY = std::max(sy, ey);
    
    auto normalizeAngle = [](double angle) {
        angle = std::fmod(angle, 2 * Geometry::PI);
        if (angle < 0) {
            angle += 2 * Geometry::PI;
        }
        return angle;
    };
    
    double start = normalizeAngle(m_arc.startAngle);
    double end = normalizeAngle(m_arc.endAngle);
    
    auto inRange = [&](double angle) {
        if (start <= end) {
            return angle >= start && angle <= end;
        } else {
            return angle >= start || angle <= end;
        }
    };
    
    if (inRange(0)) {
        maxX = m_arc.center.x + m_arc.radius;
    }
    if (inRange(Geometry::PI / 2)) {
        maxY = m_arc.center.y + m_arc.radius;
    }
    if (inRange(Geometry::PI)) {
        minX = m_arc.center.x - m_arc.radius;
    }
    if (inRange(3 * Geometry::PI / 2)) {
        minY = m_arc.center.y - m_arc.radius;
    }
    
    return Geometry::AABB(
        Geometry::Point(minX, minY, m_arc.center.z),
        Geometry::Point(maxX, maxY, m_arc.center.z)
    );
}

// 实体是否完全位于给定的轴对齐包围盒内
bool DbArc::isInside(const Geometry::AABB& rect) const {
    // 圆弧完全在矩形内：圆心在矩形内且两个端点和所有极值点都在矩形内
    // 简化实现：使用包围盒检查
    return rect.contains(Geometry::Point(m_arc.center.x - m_arc.radius, m_arc.center.y, m_arc.center.z)) &&
           rect.contains(Geometry::Point(m_arc.center.x + m_arc.radius, m_arc.center.y, m_arc.center.z)) &&
           rect.contains(Geometry::Point(m_arc.center.x, m_arc.center.y - m_arc.radius, m_arc.center.z)) &&
           rect.contains(Geometry::Point(m_arc.center.x, m_arc.center.y + m_arc.radius, m_arc.center.z));
}

// 实体是否与给定轴对齐包围盒相交（包括完全包含在内）
bool DbArc::intersects(const Geometry::AABB& rect) const {
    // 圆弧与矩形相交：使用包围盒近似
    // 找到矩形上距离圆心最近的点，检查是否小于等于半径
    double closestX = std::max(rect.min.x, std::min(m_arc.center.x, rect.max.x));
    double closestY = std::max(rect.min.y, std::min(m_arc.center.y, rect.max.y));
    double dx = m_arc.center.x - closestX;
    double dy = m_arc.center.y - closestY;
    double distSq = dx * dx + dy * dy;
    if (distSq > m_arc.radius * m_arc.radius) {
        return false;
    }
    // 还需要检查最近点是否在圆弧的角度范围内
    // 简化实现：使用包围盒相交
    return boundingBox().intersects(rect);
}

std::unique_ptr<DbObject> DbArc::clone() const {
    return std::make_unique<DbArc>(*this);
}

void DbArc::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbEntity::writeFields(writer);
    
    writer.Key("center");
    DbJsonUtils::writeVectorPoint3d(writer, m_arc.center);
    
    writer.Key("radius");
    DbJsonUtils::writeDouble(writer, m_arc.radius);
    
    writer.Key("startAngle");
    DbJsonUtils::writeDouble(writer, m_arc.startAngle);
    
    writer.Key("endAngle");
    DbJsonUtils::writeDouble(writer, m_arc.endAngle);
}

bool DbArc::readFields(const rapidjson::Value& value) {
    if (!DbEntity::readFields(value)) {
        return false;
    }
    
    Geometry::Point center;
    double radius = 0.0;
    double startAngle = 0.0;
    double endAngle = 0.0;
    
    if (value.HasMember("center")) {
        DbJsonUtils::readVectorPoint3d(value["center"], center);
    }
    if (value.HasMember("radius")) {
        DbJsonUtils::readDouble(value["radius"], radius);
    }
    if (value.HasMember("startAngle")) {
        DbJsonUtils::readDouble(value["startAngle"], startAngle);
    }
    if (value.HasMember("endAngle")) {
        DbJsonUtils::readDouble(value["endAngle"], endAngle);
    }
    
    m_arc = Geometry::Arc(center, Geometry::Vector(0, 0, 1), radius, startAngle, endAngle);
    return true;
}

} // namespace tch
