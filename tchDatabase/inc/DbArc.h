#pragma once

// C++ 标准库

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

// 项目头文件
#include "DbEntity.h"
#include "Geometry.h"


namespace tch {

class DbArc : public DbEntity {
public:
    static constexpr Type staticType() { return Type::kArc; }
    Type type() const override { return staticType(); }
    const char* typeName() const override { return "DbArc"; }
    
    bool isType(Type t) const override {
        if (t == kArc) { return true; }
        return DbEntity::isType(t);
    }
    
    DbArc() = default;
    DbArc(const Geometry::Point& center, double radius,
          double startAngle, double endAngle);
    
    const Geometry::Point& center() const { return m_arc.center; }
    void setCenter(const Geometry::Point& c) { m_arc.center = c; }
    
    double radius() const { return m_arc.radius; }
    void setRadius(double r) { m_arc.radius = r; }
    
    double startAngle() const { return m_arc.startAngle; }
    void setStartAngle(double angle) { m_arc.startAngle = angle; }
    
    double endAngle() const { return m_arc.endAngle; }
    void setEndAngle(double angle) { m_arc.endAngle = angle; }
    
    const Geometry::Arc& arc() const { return m_arc; }
    
    Geometry::AABB boundingBox() const override;
    std::unique_ptr<DbObject> clone() const override;
    
protected:
    void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
    bool readFields(const rapidjson::Value& value) override;
    
private:
    Geometry::Arc m_arc;
};

} // namespace tch