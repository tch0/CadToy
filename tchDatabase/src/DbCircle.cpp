// 对应头文件
#include "DbCircle.h"

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {

DbCircle::DbCircle(const Geometry::Point& center, double radius)
    : m_circle(Geometry::Circle::xyPlane(center, radius)) {}

Geometry::AABB DbCircle::boundingBox() const {
    return Geometry::AABB(
        Geometry::Point(m_circle.center.x - m_circle.radius, m_circle.center.y - m_circle.radius, m_circle.center.z),
        Geometry::Point(m_circle.center.x + m_circle.radius, m_circle.center.y + m_circle.radius, m_circle.center.z)
    );
}

std::unique_ptr<DbObject> DbCircle::clone() const {
    return std::make_unique<DbCircle>(*this);
}

void DbCircle::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbEntity::writeFields(writer);
    
    writer.Key("center");
    writer.StartObject();
    writer.Key("x"); writer.Double(m_circle.center.x);
    writer.Key("y"); writer.Double(m_circle.center.y);
    writer.Key("z"); writer.Double(m_circle.center.z);
    writer.EndObject();
    
    writer.Key("radius");
    writer.Double(m_circle.radius);
}

bool DbCircle::readFields(const rapidjson::Value& value) {
    if (!DbEntity::readFields(value)) { return false; }
    
    Geometry::Point center(0, 0, 0);
    double radius = 0.0;
    
    if (value.HasMember("center") && value["center"].IsObject()) {
        const auto& centerVal = value["center"];
        if (centerVal.HasMember("x") && centerVal["x"].IsDouble()) { center.x = centerVal["x"].GetDouble(); }
        if (centerVal.HasMember("y") && centerVal["y"].IsDouble()) { center.y = centerVal["y"].GetDouble(); }
        if (centerVal.HasMember("z") && centerVal["z"].IsDouble()) { center.z = centerVal["z"].GetDouble(); }
    }
    
    if (value.HasMember("radius") && value["radius"].IsDouble()) {
        radius = value["radius"].GetDouble();
    }
    
    m_circle = Geometry::Circle::xyPlane(center, radius);
    return true;
}

} // namespace tch