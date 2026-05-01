// 对应头文件
#include "DbLine.h"

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {

DbLine::DbLine(const Geometry::Point& start, const Geometry::Point& end)
    : m_segment(start, end) {}

void DbLine::setStart(const Geometry::Point& p) {
    m_segment.start = p;
    notifyModified();
}

void DbLine::setEnd(const Geometry::Point& p) {
    m_segment.end = p;
    notifyModified();
}

Geometry::AABB DbLine::computeBoundingBox() const {
    return Geometry::AABB(m_segment.start, m_segment.end);
}

// 实体是否完全位于给定的轴对齐包围盒内
bool DbLine::isInside(const Geometry::AABB& rect) const {
    // 线段完全在矩形内 ⇔ 两个端点都在矩形内
    return rect.contains(m_segment.start) && rect.contains(m_segment.end);
}

// 实体是否与给定轴对齐包围盒相交（包括完全包含在内）
bool DbLine::intersects(const Geometry::AABB& rect) const {
    // 线段与矩形相交测试
    return rect.intersectsSegment(m_segment.start, m_segment.end);
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
