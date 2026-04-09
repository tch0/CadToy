// 对应头文件
#include "DbLine.h"

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {

DbLine::DbLine(const Geometry::Point& start, const Geometry::Point& end)
    : m_segment(start, end) {}

Geometry::AABB DbLine::boundingBox() const {
    return Geometry::AABB(m_segment.start, m_segment.end);
}

std::unique_ptr<DbObject> DbLine::clone() const {
    return std::make_unique<DbLine>(*this);
}

void DbLine::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbEntity::writeFields(writer);
    
    writer.Key("start");
    DbJsonUtils::writeVectorPoint3d(writer, m_segment.start);
    
    writer.Key("end");
    DbJsonUtils::writeVectorPoint3d(writer, m_segment.end);
}

bool DbLine::readFields(const rapidjson::Value& value) {
    if (!DbEntity::readFields(value)) {
        return false;
    }
    
    Geometry::Point start, end;
    
    if (value.HasMember("start")) {
        DbJsonUtils::readVectorPoint3d(value["start"], start);
    }
    if (value.HasMember("end")) {
        DbJsonUtils::readVectorPoint3d(value["end"], end);
    }
    
    m_segment = Geometry::Segment(start, end);
    return true;
}

} // namespace tch
