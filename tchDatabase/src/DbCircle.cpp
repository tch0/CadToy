// 对应头文件
#include "DbCircle.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "Geometry.h"

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

Geometry::AABB DbCircle::computeBoundingBox() const {
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

// 实体是否与给定轴对齐包围盒相交（圆周与矩形相交）
// 注意：检测的是圆周（边界），不是圆盘（填充区域）
// 使用包围盒内最近点和最远点距圆心距离检测
bool DbCircle::intersects(const Geometry::AABB& rect) const {
    double cx = m_circle.center.x;
    double cy = m_circle.center.y;
    double r = m_circle.radius;
    double tol = Geometry::Tolerance::Default.absolute;

    // 最近点（矩形内到圆心距离最小）
    double closestX = std::clamp(cx, rect.min.x, rect.max.x);
    double closestY = std::clamp(cy, rect.min.y, rect.max.y);
    double dxMin = cx - closestX;
    double dyMin = cy - closestY;
    double dMinSq = dxMin * dxMin + dyMin * dyMin;

    // 最远点（必定是四个角点之一）
    double dMaxSq = 0.0;
    auto updateMax = [&](double px, double py) {
        double dx = px - cx;
        double dy = py - cy;
        dMaxSq = std::max(dMaxSq, dx * dx + dy * dy);
    };
    updateMax(rect.min.x, rect.min.y);
    updateMax(rect.min.x, rect.max.y);
    updateMax(rect.max.x, rect.min.y);
    updateMax(rect.max.x, rect.max.y);

    double rSq = r * r;
    // 相交条件：最近点距离 <= r（有交点或圆心在矩形内）且最远点距离 >= r（圆周穿过矩形）
    return (dMinSq <= rSq + tol) && (dMaxSq >= rSq - tol);
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
