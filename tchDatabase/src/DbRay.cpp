// 对应头文件
#include "DbRay.h"

// C++ 标准库
#include <limits>

// 第三方库

// 项目头文件


namespace tch {

DbRay::DbRay(const Geometry::Point& origin, const Geometry::Vector& direction)
    : m_ray(origin, direction) {}

void DbRay::setOrigin(const Geometry::Point& o) {
    m_ray.origin = o;
    notifyModified();
}

void DbRay::setDirection(const Geometry::Vector& d) {
    m_ray.direction = glm::normalize(d);
    notifyModified();
}

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

// 实体是否完全位于给定的轴对齐包围盒内
bool DbRay::isInside(const Geometry::AABB& rect) const {
    // 射线是无限延伸的，不可能完全在矩形内
    return false;
}

// 实体是否与给定轴对齐包围盒相交（包括完全包含在内）
bool DbRay::intersects(const Geometry::AABB& rect) const {
    // 射线与矩形相交：检查起点是否在矩形内，或射线与矩形相交
    if (rect.contains(m_ray.origin)) {
        return true;
    }
    // 简化实现：使用包围盒相交
    return boundingBox().intersects(rect);
}

std::unique_ptr<DbObject> DbRay::clone() const {
    return std::make_unique<DbRay>(*this);
}

void DbRay::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbEntity::writeFields(writer);
    
    writer.Key("origin");
    DbJsonUtils::writeVectorPoint3d(writer, m_ray.origin);
    
    writer.Key("direction");
    DbJsonUtils::writeVectorPoint3d(writer, m_ray.direction);
}

bool DbRay::readFields(const rapidjson::Value& value) {
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
    
    m_ray = Geometry::Ray(origin, direction);
    return true;
}

} // namespace tch
