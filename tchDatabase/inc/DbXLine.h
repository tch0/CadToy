#pragma once

// C++ 标准库

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

// 项目头文件
#include "DbEntity.h"
#include "Geometry.h"


namespace tch {

class DbXLine : public DbEntity {
public:
    // RTTI
    static constexpr Type staticType() { return Type::kXLine; }
    Type type() const override { return staticType(); }
    const char* typeName() const override { return "DbXLine"; }
    
    bool isType(Type t) const override {
        if (t == kXLine) { return true; }
        return DbEntity::isType(t);
    }
    
    DbXLine() = default;
    DbXLine(const Geometry::Point& origin, const Geometry::Vector& direction);
    
    const Geometry::Point& origin() const { return m_line.origin; }
    void setOrigin(const Geometry::Point& o) { m_line.origin = o; }
    
    const Geometry::Vector& direction() const { return m_line.direction; }
    void setDirection(const Geometry::Vector& d) { m_line.direction = glm::normalize(d); }
    
    const Geometry::Line& line() const { return m_line; }
    
    Geometry::AABB boundingBox() const override;
    std::unique_ptr<DbObject> clone() const override;
    
protected:
    void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
    bool readFields(const rapidjson::Value& value) override;
    
private:
    Geometry::Line m_line;
};

} // namespace tch