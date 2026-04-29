// 对应头文件
#include "DbCircle.h"

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {

DbCircle::DbCircle(const Geometry::Point& center, double radius)
    : m_circle(Geometry::Circle::xyPlane(center, radius)) {}

void DbCircle::setCenter(const Geometry::Point& c) {
    m_circle.center = c;
    notifyModified();
}

void DbCircle::setRadius(double r) {
    m_circle.radius = r;
    notifyModified();
}

Geometry::AABB DbCircle::boundingBox() const {
    return Geometry::AABB(
        Geometry::Point(m_circle.center.x - m_circle.radius, m_circle.center.y - m_circle.radius, m_circle.center.z),
        Geometry::Point(m_circle.center.x + m_circle.radius, m_circle.center.y + m_circle.radius, m_circle.center.z)
    );
}

// 实体是否完全位于给定的轴对齐包围盒内
bool DbCircle::isInside(const Geometry::AABB& rect) const {
    // 圆完全在矩形内：圆心在矩形内且圆不超出矩形边界
    if (!rect.contains(m_circle.center)) {
        return false;
    }
    // 检查圆的四个极值点是否都在矩形内
    return rect.contains(Geometry::Point(m_circle.center.x - m_circle.radius, m_circle.center.y, m_circle.center.z)) &&
           rect.contains(Geometry::Point(m_circle.center.x + m_circle.radius, m_circle.center.y, m_circle.center.z)) &&
           rect.contains(Geometry::Point(m_circle.center.x, m_circle.center.y - m_circle.radius, m_circle.center.z)) &&
           rect.contains(Geometry::Point(m_circle.center.x, m_circle.center.y + m_circle.radius, m_circle.center.z));
}

// 实体是否与给定轴对齐包围盒相交（包括完全包含在内）
bool DbCircle::intersects(const Geometry::AABB& rect) const {
    // 圆与矩形相交：找到矩形上距离圆心最近的点，检查是否小于等于半径
    double closestX = std::max(rect.min.x, std::min(m_circle.center.x, rect.max.x));
    double closestY = std::max(rect.min.y, std::min(m_circle.center.y, rect.max.y));
    double dx = m_circle.center.x - closestX;
    double dy = m_circle.center.y - closestY;
    return (dx * dx + dy * dy) <= (m_circle.radius * m_circle.radius);
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
