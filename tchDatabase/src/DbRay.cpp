// 对应头文件
#include "DbRay.h"

// C++ 标准库
#include <limits>

// 第三方库

// 项目头文件


namespace tch {

DbRay::DbRay(const Geometry::Point& origin, const Geometry::Vector& direction)
    : m_ray(origin, direction) {}

Geometry::AABB DbRay::boundingBox() const {
    const Geometry::Point& o = m_ray.origin;
    constexpr double inf = std::numeric_limits<double>::infinity();
    const double tol = Geometry::Tolerance::Default.absolute;
    
    double minX = o.x, maxX = o.x;
    double minY = o.y, maxY = o.y;
    double minZ = o.z, maxZ = o.z;
    
    if (std::abs(m_ray.direction.x) > tol) {
        maxX = inf;
    }
    if (std::abs(m_ray.direction.y) > tol) {
        maxY = inf;
    }
    if (std::abs(m_ray.direction.z) > tol) {
        maxZ = inf;
    }
    
    return Geometry::AABB(
        Geometry::Point(minX, minY, minZ),
        Geometry::Point(maxX, maxY, maxZ)
    );
}

std::unique_ptr<DbObject> DbRay::clone() const {
    return std::make_unique<DbRay>(*this);
}

void DbRay::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbEntity::writeFields(writer);
    
    writer.Key("origin");
    writer.StartObject();
    writer.Key("x");
    writer.Double(m_ray.origin.x);
    writer.Key("y");
    writer.Double(m_ray.origin.y);
    writer.Key("z");
    writer.Double(m_ray.origin.z);
    writer.EndObject();
    
    writer.Key("direction");
    writer.StartObject();
    writer.Key("x");
    writer.Double(m_ray.direction.x);
    writer.Key("y");
    writer.Double(m_ray.direction.y);
    writer.Key("z");
    writer.Double(m_ray.direction.z);
    writer.EndObject();
}

bool DbRay::readFields(const rapidjson::Value& value) {
    if (!DbEntity::readFields(value)) {
        return false;
    }
    
    Geometry::Point origin(0, 0, 0);
    Geometry::Vector direction(1, 0, 0);
    
    if (value.HasMember("origin") && value["origin"].IsObject()) {
        const auto& originVal = value["origin"];
        if (originVal.HasMember("x") && originVal["x"].IsDouble()) {
            origin.x = originVal["x"].GetDouble();
        }
        if (originVal.HasMember("y") && originVal["y"].IsDouble()) {
            origin.y = originVal["y"].GetDouble();
        }
        if (originVal.HasMember("z") && originVal["z"].IsDouble()) {
            origin.z = originVal["z"].GetDouble();
        }
    }
    
    if (value.HasMember("direction") && value["direction"].IsObject()) {
        const auto& dirVal = value["direction"];
        if (dirVal.HasMember("x") && dirVal["x"].IsDouble()) {
            direction.x = dirVal["x"].GetDouble();
        }
        if (dirVal.HasMember("y") && dirVal["y"].IsDouble()) {
            direction.y = dirVal["y"].GetDouble();
        }
        if (dirVal.HasMember("z") && dirVal["z"].IsDouble()) {
            direction.z = dirVal["z"].GetDouble();
        }
    }
    
    m_ray = Geometry::Ray(origin, direction);
    return true;
}

} // namespace tch
