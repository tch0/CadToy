// 对应头文件
#include "DbXLine.h"

// C++ 标准库
#include <limits>

// 第三方库

// 项目头文件


namespace tch {

DbXLine::DbXLine(const Geometry::Point& origin, const Geometry::Vector& direction)
    : m_line(origin, direction) {}

Geometry::AABB DbXLine::boundingBox() const {
    const Geometry::Point& o = m_line.origin;
    constexpr double inf = std::numeric_limits<double>::infinity();
    const double tol = Geometry::Tolerance::Default.absolute;
    
    double minX = o.x, maxX = o.x;
    double minY = o.y, maxY = o.y;
    double minZ = o.z, maxZ = o.z;
    
    if (std::abs(m_line.direction.x) > tol) {
        minX = -inf;
        maxX = inf;
    }
    if (std::abs(m_line.direction.y) > tol) {
        minY = -inf;
        maxY = inf;
    }
    if (std::abs(m_line.direction.z) > tol) {
        minZ = -inf;
        maxZ = inf;
    }
    
    return Geometry::AABB(
        Geometry::Point(minX, minY, minZ),
        Geometry::Point(maxX, maxY, maxZ)
    );
}

std::unique_ptr<DbObject> DbXLine::clone() const {
    return std::make_unique<DbXLine>(*this);
}

void DbXLine::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbEntity::writeFields(writer);
    
    writer.Key("origin");
    writer.StartObject();
    writer.Key("x");
    writer.Double(m_line.origin.x);
    writer.Key("y");
    writer.Double(m_line.origin.y);
    writer.Key("z");
    writer.Double(m_line.origin.z);
    writer.EndObject();
    
    writer.Key("direction");
    writer.StartObject();
    writer.Key("x");
    writer.Double(m_line.direction.x);
    writer.Key("y");
    writer.Double(m_line.direction.y);
    writer.Key("z");
    writer.Double(m_line.direction.z);
    writer.EndObject();
}

bool DbXLine::readFields(const rapidjson::Value& value) {
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
    
    m_line = Geometry::Line(origin, direction);
    return true;
}

} // namespace tch
