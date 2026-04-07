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

std::unique_ptr<DbObject> DbArc::clone() const {
    return std::make_unique<DbArc>(*this);
}

void DbArc::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbEntity::writeFields(writer);
    
    writer.Key("center");
    writer.StartObject();
    writer.Key("x");
    writer.Double(m_arc.center.x);
    writer.Key("y");
    writer.Double(m_arc.center.y);
    writer.Key("z");
    writer.Double(m_arc.center.z);
    writer.EndObject();
    
    writer.Key("radius");
    writer.Double(m_arc.radius);
    
    writer.Key("startAngle");
    writer.Double(m_arc.startAngle);
    
    writer.Key("endAngle");
    writer.Double(m_arc.endAngle);
}

bool DbArc::readFields(const rapidjson::Value& value) {
    if (!DbEntity::readFields(value)) { return false; }
    
    Geometry::Point center(0, 0, 0);
    double radius = 0.0;
    double startAngle = 0.0;
    double endAngle = 0.0;
    
    if (value.HasMember("center") && value["center"].IsObject()) {
        const auto& centerVal = value["center"];
        if (centerVal.HasMember("x") && centerVal["x"].IsDouble()) { center.x = centerVal["x"].GetDouble(); }
        if (centerVal.HasMember("y") && centerVal["y"].IsDouble()) { center.y = centerVal["y"].GetDouble(); }
        if (centerVal.HasMember("z") && centerVal["z"].IsDouble()) { center.z = centerVal["z"].GetDouble(); }
    }
    
    if (value.HasMember("radius") && value["radius"].IsDouble()) {
        radius = value["radius"].GetDouble();
    }
    
    if (value.HasMember("startAngle") && value["startAngle"].IsDouble()) {
        startAngle = value["startAngle"].GetDouble();
    }
    
    if (value.HasMember("endAngle") && value["endAngle"].IsDouble()) {
        endAngle = value["endAngle"].GetDouble();
    }
    
    m_arc = Geometry::Arc(center, Geometry::Vector(0, 0, 1), radius, startAngle, endAngle);
    return true;
}

} // namespace tch