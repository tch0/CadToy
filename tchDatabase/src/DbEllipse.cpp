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

Geometry::AABB DbEllipse::boundingBox() const {
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
    writer.StartObject();
    writer.Key("x");
    writer.Double(m_ellipse.center.x);
    writer.Key("y");
    writer.Double(m_ellipse.center.y);
    writer.Key("z");
    writer.Double(m_ellipse.center.z);
    writer.EndObject();
    
    writer.Key("radiusX");
    writer.Double(m_ellipse.radiusX);
    
    writer.Key("radiusY");
    writer.Double(m_ellipse.radiusY);
    
    writer.Key("rotation");
    writer.Double(m_ellipse.rotation);
    
    writer.Key("startParam");
    writer.Double(m_ellipse.startParam);
    
    writer.Key("endParam");
    writer.Double(m_ellipse.endParam);
}

bool DbEllipse::readFields(const rapidjson::Value& value) {
    if (!DbEntity::readFields(value)) { return false; }
    
    Geometry::Point center(0, 0, 0);
    double radiusX = 0.0;
    double radiusY = 0.0;
    double rotation = 0.0;
    double startParam = 0.0;
    double endParam = 2.0 * Geometry::PI;
    
    if (value.HasMember("center") && value["center"].IsObject()) {
        const auto& centerVal = value["center"];
        if (centerVal.HasMember("x") && centerVal["x"].IsDouble()) { center.x = centerVal["x"].GetDouble(); }
        if (centerVal.HasMember("y") && centerVal["y"].IsDouble()) { center.y = centerVal["y"].GetDouble(); }
        if (centerVal.HasMember("z") && centerVal["z"].IsDouble()) { center.z = centerVal["z"].GetDouble(); }
    }
    
    if (value.HasMember("radiusX") && value["radiusX"].IsDouble()) {
        radiusX = value["radiusX"].GetDouble();
    }
    
    if (value.HasMember("radiusY") && value["radiusY"].IsDouble()) {
        radiusY = value["radiusY"].GetDouble();
    }
    
    if (value.HasMember("rotation") && value["rotation"].IsDouble()) {
        rotation = value["rotation"].GetDouble();
    }
    
    if (value.HasMember("startParam") && value["startParam"].IsDouble()) {
        startParam = value["startParam"].GetDouble();
    }
    
    if (value.HasMember("endParam") && value["endParam"].IsDouble()) {
        endParam = value["endParam"].GetDouble();
    }
    
    m_ellipse = Geometry::Ellipse(center, Geometry::Vector(0, 0, 1),
                                   radiusX, radiusY, rotation,
                                   startParam, endParam);
    return true;
}

} // namespace tch