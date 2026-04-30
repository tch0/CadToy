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

Geometry::AABB DbArc::boundingBox() const {
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

// 实体是否完全位于给定的轴对齐包围盒内
bool DbArc::isInside(const Geometry::AABB& rect) const {
    // 圆弧完全在矩形内：使用圆弧自身的包围盒检查
    Geometry::AABB bbox = boundingBox();
    return rect.contains(bbox.min) && rect.contains(bbox.max);
}

// 实体是否与给定轴对齐包围盒相交（包括完全包含在内）
bool DbArc::intersects(const Geometry::AABB& rect) const {
    // 1. 快速排斥：包围盒不相交
    if (!boundingBox().intersects(rect)) {
        return false;
    }
    
    // 2. 快速排斥：矩形完全在圆外（圆心到矩形最近距离 > 半径）
    double cx = m_arc.center.x, cy = m_arc.center.y, r = m_arc.radius;
    double closestX = std::max(rect.min.x, std::min(cx, rect.max.x));
    double closestY = std::max(rect.min.y, std::min(cy, rect.max.y));
    double dx = cx - closestX;
    double dy = cy - closestY;
    if (dx * dx + dy * dy > r * r) {
        return false;
    }
    
    // 3. 特殊情况：半径极小，视为点
    if (r <= Geometry::Tolerance::Default.absolute) {
        Geometry::Point arcPoint(cx + r * std::cos(m_arc.startAngle),
                                 cy + r * std::sin(m_arc.startAngle),
                                 m_arc.center.z);
        return rect.contains(arcPoint);
    }
    
    // 4. 将弧按弦高误差自适应离散为线段序列，逐段测试
    double start = Geometry::normalizeAngle(m_arc.startAngle);
    double end   = Geometry::normalizeAngle(m_arc.endAngle);
    double sweep = end - start;
    if (sweep < 0) { sweep += 2.0 * Geometry::PI; }  // 保证逆时针正角度

    double maxStep = 2.0 * std::acos(1.0 - Geometry::Tolerance::Selection.absolute / r);  // 弦高误差控制步长
    int steps = std::max(1, static_cast<int>(std::ceil(sweep / maxStep)));
    double thetaStep = sweep / steps;
    
    // 生成端点的 lambda
    auto pointAtAngle = [cx, cy, r, z = m_arc.center.z](double angle) -> Geometry::Point {
        return Geometry::Point(cx + r * std::cos(angle),
                               cy + r * std::sin(angle),
                               z);
    };
    
    Geometry::Point prev = pointAtAngle(start);
    for (int i = 1; i <= steps; ++i) {
        Geometry::Point curr = pointAtAngle(start + i * thetaStep);
        if (rect.intersectsSegment(prev, curr)) {
            return true;
        }
        prev = curr;
    }
    return false;
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
