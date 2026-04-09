// 对应头文件
#include "DbLayer.h"

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {

std::unique_ptr<DbObject> DbLayer::clone() const {
    return std::make_unique<DbLayer>(*this);
}

void DbLayer::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbObject::writeFields(writer);
    
    writer.Key("name");
    DbJsonUtils::writeString(writer, m_name);
    
    writer.Key("frozen");
    DbJsonUtils::writeBool(writer, m_frozen);
    
    writer.Key("locked");
    DbJsonUtils::writeBool(writer, m_locked);
    
    writer.Key("color");
    DbJsonUtils::writeColor(writer, m_color);
    
    writer.Key("linetype");
    DbJsonUtils::writeLinetype(writer, m_linetype);
    
    writer.Key("lineWeight");
    DbJsonUtils::writeLineWeight(writer, m_lineWeight);
    
    writer.Key("transparency");
    DbJsonUtils::writeDouble(writer, m_transparency);
    
    writer.Key("description");
    DbJsonUtils::writeString(writer, m_description);
}

bool DbLayer::readFields(const rapidjson::Value& value) {
    if (!DbObject::readFields(value)) {
        return false;
    }
    
    if (value.HasMember("name")) {
        DbJsonUtils::readString(value["name"], m_name);
    }
    
    if (value.HasMember("frozen")) {
        DbJsonUtils::readBool(value["frozen"], m_frozen);
    }
    
    if (value.HasMember("locked")) {
        DbJsonUtils::readBool(value["locked"], m_locked);
    }
    
    if (value.HasMember("color")) {
        DbJsonUtils::readColor(value["color"], m_color);
    }
    
    if (value.HasMember("linetype")) {
        DbJsonUtils::readLinetype(value["linetype"], m_linetype);
    }
    
    if (value.HasMember("lineWeight")) {
        DbJsonUtils::readLineWeight(value["lineWeight"], m_lineWeight);
    }
    
    if (value.HasMember("transparency")) {
        double t = 0.0;
        DbJsonUtils::readDouble(value["transparency"], t);
        setTransparency(static_cast<float>(t));
    }
    
    if (value.HasMember("description")) {
        DbJsonUtils::readString(value["description"], m_description);
    }
    
    return true;
}

} // namespace tch
