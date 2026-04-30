// 对应头文件
#include "DbRay.h"

// C++ 标准库

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
    constexpr double inf = Geometry::INF;
    const double tol = Geometry::Tolerance::Default.absolute;
    
    double minX = o.x, maxX = o.x;
    double minY = o.y, maxY = o.y;
    double minZ = o.z, maxZ = o.z;
    
    auto extend = [tol](double dir, double origin, double& min, double& max) {
        if (dir > tol) {
            max = inf;                    // 向正方向延伸
        } else if (dir < -tol) {
            min = -inf;                   // 向负方向延伸
        } // else dir ≈ 0，min, max 保持 origin 不变
    };
    
    extend(m_ray.direction.x, o.x, minX, maxX);
    extend(m_ray.direction.y, o.y, minY, maxY);
    extend(m_ray.direction.z, o.z, minZ, maxZ);
    
    return Geometry::AABB(
        Geometry::Point(minX, minY, minZ),
        Geometry::Point(maxX, maxY, maxZ)
    );
}

// 实体是否完全位于给定的轴对齐包围盒内
bool DbRay::isInside(const Geometry::AABB&) const {
    // 射线是无限延伸的，不可能完全在矩形内
    return false;
}

// 实体是否与给定轴对齐包围盒相交（包括完全包含在内）
bool DbRay::intersects(const Geometry::AABB& rect) const {
    // 1. 快速接受：起点在矩形内
    if (rect.contains(m_ray.origin)) { return true; }
    
    const Geometry::Point& O = m_ray.origin;
    const Geometry::Vector& D = m_ray.direction;
    const double tol = Geometry::Tolerance::Default.absolute;   // 几何精度，不是选择容差
    
    double tMin = 0.0;                      // 射线 t 下限
    double tMax = Geometry::INF; // 上限
    
    // 在每个维度上裁剪参数区间
    auto clip = [&](double p, double q) -> bool {
        if (std::abs(p) < tol) {
            // 射线方向在该维度分量为零，平行于该边界
            if (q < 0) { return false; }       // 起点在边界外部 → 不相交
            return true;
        }
        double r = q / p;
        if (p < 0) {
            // 进入该维度的一侧边界
            if (r > tMax) { return false; }
            if (r > tMin) { tMin = r; }
        } else {
            // 离开该维度的一侧边界
            if (r < tMin) { return false; }
            if (r < tMax) { tMax = r; }
        }
        return true;
    };
    
    // X 维度的两个平面
    if (!clip(-D.x, O.x - rect.min.x)) { return false; }
    if (!clip( D.x, rect.max.x - O.x)) { return false; }
    // Y 维度的两个平面
    if (!clip(-D.y, O.y - rect.min.y)) { return false; }
    if (!clip( D.y, rect.max.y - O.y)) { return false; }
    // Z 维度的两个平面（2D CAD 通常忽略，但保留以保持三维兼容性）
    if (!clip(-D.z, O.z - rect.min.z)) { return false; }
    if (!clip( D.z, rect.max.z - O.z)) { return false; }
    
    // 存在有效的交点参数 t，且 t >= 0（已由 tMin=0 保证）
    return tMin <= tMax;
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
