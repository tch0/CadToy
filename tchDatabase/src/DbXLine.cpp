// 对应头文件
#include "DbXLine.h"

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {

DbXLine::DbXLine(const Geometry::Point& origin, const Geometry::Vector& direction)
    : m_line(origin, direction) {}

void DbXLine::setOrigin(const Geometry::Point& o) {
    m_line.origin = o;
    notifyModified();
}

void DbXLine::setDirection(const Geometry::Vector& d) {
    m_line.direction = glm::normalize(d);
    notifyModified();
}

// 重写包围盒计算，同样实现缓存机制，但不使用图形缓存顶点
Geometry::AABB DbXLine::boundingBox() const {
    if (!m_bboxDirty) { 
        return m_cachedBBox; 
    }
    m_cachedBBox = computeBoundingBox();
    m_bboxDirty = false;
    return m_cachedBBox;
}

Geometry::AABB DbXLine::computeBoundingBox() const {
    const Geometry::Point& o = m_line.origin;
    constexpr double inf = Geometry::INF;
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

// 实体是否完全位于给定的轴对齐包围盒内
bool DbXLine::isInside(const Geometry::AABB&) const {
    // 构造线是无限延伸的，不可能完全在矩形内
    return false;
}

// 实体是否与给定轴对齐包围盒相交（包括完全包含在内）
bool DbXLine::intersects(const Geometry::AABB& rect) const {
    const Geometry::Point& O = m_line.origin;
    const Geometry::Point& D = m_line.direction;
    const double tol = Geometry::Tolerance::Default.absolute;

    // 采用 Slab 方法，t 范围 (-∞, ∞)
    double tMin = -Geometry::INF;
    double tMax =  Geometry::INF;

    auto clip = [&](double p, double q) -> bool {
        if (std::abs(p) < tol) {
            // 方向在该维度上的分量为零，直线平行于该边界
            if (q < 0) { return false; }   // 在边界外侧，永不相交
            return true;
        }
        double r = q / p;
        if (p < 0) {
            if (r > tMax) { return false; }
            if (r > tMin) { tMin = r; }
        } else {
            if (r < tMin) { return false; }
            if (r < tMax) { tMax = r; }
        }
        return true;
    };

    // X 维度
    if (!clip(-D.x, O.x - rect.min.x)) { return false; }
    if (!clip( D.x, rect.max.x - O.x)) { return false; }
    // Y 维度
    if (!clip(-D.y, O.y - rect.min.y)) { return false; }
    if (!clip( D.y, rect.max.y - O.y)) { return false; }
    // Z 维度（若为 2D，通常通过）
    if (!clip(-D.z, O.z - rect.min.z)) { return false; }
    if (!clip( D.z, rect.max.z - O.z)) { return false; }

    // 存在合法的 t（任何实数）即表示相交
    return tMin <= tMax;
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
