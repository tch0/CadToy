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
    DbJsonUtils::writeVectorPoint3d(writer, m_line.origin);
    
    writer.Key("direction");
    DbJsonUtils::writeVectorPoint3d(writer, m_line.direction);
}

bool DbXLine::readFields(const rapidjson::Value& value) {
    if (!DbEntity::readFields(value)) {
        return false;
    }
    
    Geometry::Point origin;
    Geometry::Vector direction(1, 0, 0);
    
    if (value.HasMember("origin")) {
        DbJsonUtils::readVectorPoint3d(value["origin"], origin);
    }
    if (value.HasMember("direction")) {
        DbJsonUtils::readVectorPoint3d(value["direction"], direction);
    }
    
    m_line = Geometry::Line(origin, direction);
    return true;
}

} // namespace tch
