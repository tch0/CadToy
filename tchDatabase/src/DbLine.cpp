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
    writer.StartObject();
    writer.Key("x"); writer.Double(m_segment.start.x);
    writer.Key("y"); writer.Double(m_segment.start.y);
    writer.Key("z"); writer.Double(m_segment.start.z);
    writer.EndObject();
    
    writer.Key("end");
    writer.StartObject();
    writer.Key("x"); writer.Double(m_segment.end.x);
    writer.Key("y"); writer.Double(m_segment.end.y);
    writer.Key("z"); writer.Double(m_segment.end.z);
    writer.EndObject();
}

bool DbLine::readFields(const rapidjson::Value& value) {
    if (!DbEntity::readFields(value)) { return false; }
    
    Geometry::Point start, end;
    
    if (value.HasMember("start") && value["start"].IsObject()) {
        const auto& startVal = value["start"];
        if (startVal.HasMember("x") && startVal["x"].IsDouble()) { start.x = startVal["x"].GetDouble(); }
        if (startVal.HasMember("y") && startVal["y"].IsDouble()) { start.y = startVal["y"].GetDouble(); }
        if (startVal.HasMember("z") && startVal["z"].IsDouble()) { start.z = startVal["z"].GetDouble(); }
    }
    
    if (value.HasMember("end") && value["end"].IsObject()) {
        const auto& endVal = value["end"];
        if (endVal.HasMember("x") && endVal["x"].IsDouble()) { end.x = endVal["x"].GetDouble(); }
        if (endVal.HasMember("y") && endVal["y"].IsDouble()) { end.y = endVal["y"].GetDouble(); }
        if (endVal.HasMember("z") && endVal["z"].IsDouble()) { end.z = endVal["z"].GetDouble(); }
    }
    
    m_segment = Geometry::Segment(start, end);
    return true;
}

} // namespace tch