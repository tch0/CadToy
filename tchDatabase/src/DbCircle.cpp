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
    DbJsonUtils::writeVectorPoint3d(writer, m_circle.center);
    
    writer.Key("radius");
    DbJsonUtils::writeDouble(writer, m_circle.radius);
}

bool DbCircle::readFields(const rapidjson::Value& value) {
    if (!DbEntity::readFields(value)) {
        return false;
    }
    
    Geometry::Point center;
    double radius = 0.0;
    
    if (value.HasMember("center")) {
        DbJsonUtils::readVectorPoint3d(value["center"], center);
    }
    if (value.HasMember("radius")) {
        DbJsonUtils::readDouble(value["radius"], radius);
    }
    
    m_circle = Geometry::Circle::xyPlane(center, radius);
    return true;
}

} // namespace tch
