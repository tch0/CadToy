#pragma once

// C++ 标准库

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

// 项目头文件
#include "DbEntity.h"
#include "Geometry.h"


namespace tch {

class DbCircle : public DbEntity {
public:
    // RTTI
    static constexpr Type staticType() { return Type::kCircle; }
    Type type() const override { return staticType(); }
    const char* typeName() const override { return "DbCircle"; }
    
    bool isType(Type t) const override {
        if (t == kCircle) { return true; }
        return DbEntity::isType(t);
    }
    
    DbCircle() = default;
    DbCircle(const Geometry::Point& center, double radius);
    
    const Geometry::Point& center() const { return m_circle.center; }
    void setCenter(const Geometry::Point& c);
    
    double radius() const { return m_circle.radius; }
    void setRadius(double r);
    
    const Geometry::Circle& circle() const { return m_circle; }
    
    Geometry::AABB boundingBox() const override;
    std::unique_ptr<DbObject> clone() const override;
    
protected:
    void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
    bool readFields(const rapidjson::Value& value) override;
    
private:
    Geometry::Circle m_circle = Geometry::Circle::xyPlane(Geometry::Point(0, 0, 0), 0);
};

} // namespace tch