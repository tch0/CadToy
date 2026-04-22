#pragma once

// C++ 标准库

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

// 项目头文件
#include "DbEntity.h"
#include "Geometry.h"


namespace tch {

class DbRay : public DbEntity {
public:
    // RTTI
    static constexpr Type staticType() { return Type::kRay; }
    Type type() const override { return staticType(); }
    const char* typeName() const override { return "DbRay"; }
    
    bool isType(Type t) const override {
        if (t == kRay) { return true; }
        return DbEntity::isType(t);
    }
    
    DbRay() = default;
    DbRay(const Geometry::Point& origin, const Geometry::Vector& direction);
    
    const Geometry::Point& origin() const { return m_ray.origin; }
    void setOrigin(const Geometry::Point& o);
    
    const Geometry::Vector& direction() const { return m_ray.direction; }
    void setDirection(const Geometry::Vector& d);
    
    const Geometry::Ray& ray() const { return m_ray; }
    
    Geometry::AABB boundingBox() const override;
    std::unique_ptr<DbObject> clone() const override;
    
protected:
    void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
    bool readFields(const rapidjson::Value& value) override;
    
private:
    Geometry::Ray m_ray;
};

} // namespace tch
