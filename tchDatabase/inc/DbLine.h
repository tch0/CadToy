#pragma once

// C++ 标准库

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

// 项目头文件
#include "DbEntity.h"
#include "Geometry.h"


namespace tch {

class DbLine : public DbEntity {
public:
    static Type staticType() { return Type::kLine; }
    Type type() const override { return staticType(); }
    const char* typeName() const override { return "Line"; }
    
    DbLine() = default;
    DbLine(const Geometry::Point& start, const Geometry::Point& end);
    
    const Geometry::Point& start() const { return m_segment.start; }
    void setStart(const Geometry::Point& p) { m_segment.start = p; }
    
    const Geometry::Point& end() const { return m_segment.end; }
    void setEnd(const Geometry::Point& p) { m_segment.end = p; }
    
    const Geometry::Segment& segment() const { return m_segment; }
    
    Geometry::AABB boundingBox() const override;
    std::unique_ptr<DbObject> clone() const override;
    
protected:
    void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
    bool readFields(const rapidjson::Value& value) override;
    
private:
    Geometry::Segment m_segment;
};

} // namespace tch