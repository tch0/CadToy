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

Geometry::AABB DbArc::computeBoundingBox() const {
    double sx = m_arc.center.x + m_arc.radius * std::cos(m_arc.startAngle);
    double sy = m_arc.center.y + m_arc.radius * std::sin(m_arc.startAngle);
    double ex = m_arc.center.x + m_arc.radius * std::cos(m_arc.endAngle);
    double ey = m_arc.center.y + m_arc.radius * std::sin(m_arc.endAngle);
    
    double minX = std::min(sx, ex);
    double maxX = std::max(sx, ex);
    double minY = std::min(sy, ey);
    double maxY = std::max(sy, ey);
    
    double start = Geometry::normalizeAngle(m_arc.startAngle);
    double end = Geometry::normalizeAngle(m_arc.endAngle);
    
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
    if (inRange(Geometry::HALF_PI)) {
        maxY = m_arc.center.y + m_arc.radius;
    }
    if (inRange(Geometry::PI)) {
        minX = m_arc.center.x - m_arc.radius;
    }
    if (inRange(3 * Geometry::HALF_PI)) {
        minY = m_arc.center.y - m_arc.radius;
    }
    
    return Geometry::AABB(
        Geometry::Point(minX, minY, m_arc.center.z),
        Geometry::Point(maxX, maxY, m_arc.center.z)
    );
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
