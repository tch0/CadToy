#pragma once

// C++ 标准库

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

// 项目头文件
#include "DbEntity.h"
#include "Geometry.h"


namespace tch {

class DbEllipse : public DbEntity {
public:
    // RTTI
    static constexpr Type staticType() { return Type::kEllipse; }
    Type type() const override { return staticType(); }
    const char* typeName() const override { return "DbEllipse"; }
    
    bool isType(Type t) const override {
        if (t == kEllipse) { return true; }
        return DbEntity::isType(t);
    }
    
    DbEllipse() = default;
    DbEllipse(const Geometry::Point& center, double radiusX, double radiusY, double rotation);
    DbEllipse(const Geometry::Point& center, double radiusX, double radiusY,
              double rotation, double startParam, double endParam);
    
    const Geometry::Point& center() const { return m_ellipse.center; }
    void setCenter(const Geometry::Point& c);
    
    double radiusX() const { return m_ellipse.radiusX; }
    void setRadiusX(double rx);
    
    double radiusY() const { return m_ellipse.radiusY; }
    void setRadiusY(double ry);
    
    double rotation() const { return m_ellipse.rotation; }
    void setRotation(double rot);
    
    double startParam() const { return m_ellipse.startParam; }
    void setStartParam(double param);
    
    double endParam() const { return m_ellipse.endParam; }
    void setEndParam(double param);
    
    const Geometry::Ellipse& ellipse() const { return m_ellipse; }
    
    Geometry::AABB computeBoundingBox() const override;

    std::unique_ptr<DbObject> clone() const override;
    
protected:
    void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
    bool readFields(const rapidjson::Value& value) override;
    
private:
    Geometry::Ellipse m_ellipse;
};

} // namespace tch