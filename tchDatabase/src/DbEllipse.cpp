// 对应头文件
#include "DbEllipse.h"

// C++ 标准库
#include <cmath>

// 第三方库

// 项目头文件


namespace tch {

DbEllipse::DbEllipse(const Geometry::Point& center, double radiusX, double radiusY, double rotation)
    : m_ellipse(center, Geometry::Vector(0, 0, 1), radiusX, radiusY, rotation) {}

DbEllipse::DbEllipse(const Geometry::Point& center, double radiusX, double radiusY,
                     double rotation, double startParam, double endParam)
    : m_ellipse(center, Geometry::Vector(0, 0, 1), radiusX, radiusY, rotation, startParam, endParam) {}

void DbEllipse::setCenter(const Geometry::Point& c) {
    m_ellipse.center = c;
    notifyModified();
}

void DbEllipse::setRadiusX(double rx) {
    m_ellipse.radiusX = rx;
    notifyModified();
}

void DbEllipse::setRadiusY(double ry) {
    m_ellipse.radiusY = ry;
    notifyModified();
}

void DbEllipse::setRotation(double rot) {
    m_ellipse.rotation = rot;
    notifyModified();
}

void DbEllipse::setStartParam(double param) {
    m_ellipse.startParam = param;
    notifyModified();
}

void DbEllipse::setEndParam(double param) {
    m_ellipse.endParam = param;
    notifyModified();
}

// TODO: 这个包围盒计算并不准确，没有考虑椭圆弧，椭圆弧也返回整个椭圆的包围盒，计算椭圆弧的包围盒最好像圆弧一样离散之后计算比较好
// 因为现在已经实现了使用实体缓存数据计算包围盒的机制，可以暂时不用实现管
Geometry::AABB DbEllipse::computeBoundingBox() const {
    double rx = m_ellipse.radiusX;
    double ry = m_ellipse.radiusY;
    double cosR = std::cos(m_ellipse.rotation);
    double sinR = std::sin(m_ellipse.rotation);
    
    // 旋转椭圆的轴对齐包围盒计算：
    // 椭圆参数方程：x = rx*cos(θ)*cos(φ) - ry*sin(θ)*sin(φ)
    //              y = rx*cos(θ)*sin(φ) + ry*sin(θ)*cos(φ)
    // 其中φ为旋转角度，θ为椭圆参数。
    // 对θ求导并令其为0，可得极值点：
    //   |x_max| = sqrt(rx²*cos²φ + ry²*sin²φ)
    //   |y_max| = sqrt(rx²*sin²φ + ry²*cos²φ)
    double halfW = std::sqrt(rx * rx * cosR * cosR + ry * ry * sinR * sinR);
    double halfH = std::sqrt(rx * rx * sinR * sinR + ry * ry * cosR * cosR);
    
    return Geometry::AABB(
        Geometry::Point(m_ellipse.center.x - halfW, m_ellipse.center.y - halfH, m_ellipse.center.z),
        Geometry::Point(m_ellipse.center.x + halfW, m_ellipse.center.y + halfH, m_ellipse.center.z)
    );
}

std::unique_ptr<DbObject> DbEllipse::clone() const {
    return std::make_unique<DbEllipse>(*this);
}

void DbEllipse::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbEntity::writeFields(writer);
    
    writer.Key("center");
    DbJsonUtils::writeVectorPoint3d(writer, m_ellipse.center);
    
    writer.Key("radiusX");
    DbJsonUtils::writeDouble(writer, m_ellipse.radiusX);
    
    writer.Key("radiusY");
    DbJsonUtils::writeDouble(writer, m_ellipse.radiusY);
    
    writer.Key("rotation");
    DbJsonUtils::writeDouble(writer, m_ellipse.rotation);
    
    writer.Key("startParam");
    DbJsonUtils::writeDouble(writer, m_ellipse.startParam);
    
    writer.Key("endParam");
    DbJsonUtils::writeDouble(writer, m_ellipse.endParam);
}

bool DbEllipse::readFields(const rapidjson::Value& value) {
    if (!DbEntity::readFields(value)) {
        return false;
    }
    
    Geometry::Point center;
    double radiusX = 0.0;
    double radiusY = 0.0;
    double rotation = 0.0;
    double startParam = 0.0;
    double endParam = 2.0 * Geometry::PI;
    
    if (value.HasMember("center")) {
        DbJsonUtils::readVectorPoint3d(value["center"], center);
    }
    if (value.HasMember("radiusX")) {
        DbJsonUtils::readDouble(value["radiusX"], radiusX);
    }
    if (value.HasMember("radiusY")) {
        DbJsonUtils::readDouble(value["radiusY"], radiusY);
    }
    if (value.HasMember("rotation")) {
        DbJsonUtils::readDouble(value["rotation"], rotation);
    }
    if (value.HasMember("startParam")) {
        DbJsonUtils::readDouble(value["startParam"], startParam);
    }
    if (value.HasMember("endParam")) {
        DbJsonUtils::readDouble(value["endParam"], endParam);
    }
    
    m_ellipse = Geometry::Ellipse(center, Geometry::Vector(0, 0, 1), radiusX, radiusY, rotation, startParam, endParam);
    return true;
}

} // namespace tch
