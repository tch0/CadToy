// 对应头文件
#include "DbLayer.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "Database.h"


namespace tch {

std::unique_ptr<DbObject> DbLayer::clone() const {
    return std::make_unique<DbLayer>(*this);
}

void DbLayer::notifyModified() {
    if (m_pDb && m_id != 0) {
        m_pDb->onLayerModified(m_id);
    }
}

void DbLayer::setFrozen(bool frozen) {
    if (m_frozen != frozen) {
        m_frozen = frozen;
        notifyModified();
    }
}

void DbLayer::setLocked(bool locked) {
    if (m_locked != locked) {
        m_locked = locked;
        notifyModified();
    }
}

void DbLayer::setColor(const DbColor& color) {
    if (m_color != color) {
        m_color = color;
        notifyModified();
    }
}

void DbLayer::setLinetype(const DbLinetypeRef& linetype) {
    if (m_linetype != linetype) {
        m_linetype = linetype;
        notifyModified();
    }
}

void DbLayer::setLineWeight(DbLineWeight lw) {
    if (m_lineWeight != lw) {
        m_lineWeight = lw;
        notifyModified();
    }
}

void DbLayer::setTransparency(float t) {
    float clamped = std::max(0.0f, std::min(1.0f, t));
    if (m_transparency != clamped) {
        m_transparency = clamped;
        notifyModified();
    }
}

void DbLayer::setName(const std::string& name) {
    if (m_name != name) {
        std::string oldName = m_name;
        m_name = name;
        // 名称会导致数据库更新图层名称索引，需要执行特殊通知逻辑
        if (m_pDb && m_id != 0) {
            m_pDb->onLayerNameModified(m_id, oldName); // 传递旧名称
        }
    }
}

void DbLayer::setDescription(const std::string& desc) {
    if (m_description != desc) {
        m_description = desc;
        notifyModified();
    }
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
