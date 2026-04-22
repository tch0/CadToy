// 对应头文件
#include "DbEntity.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "Database.h"


namespace tch {

void DbEntity::setLayerId(ObjectId id) {
    if (m_layerId == id) {
        return;
    }

    // 只有在数据库中且已分配ID时才触发moveEntityToLayer
    if (m_pDb && m_id != 0) {
        ObjectId actualLayerId = m_pDb->moveEntityToLayer(m_id, id);
        m_layerId = actualLayerId;
        notifyModified();
    } else {
        // 不在数据库中，直接设置
        m_layerId = id;
    }
}

void DbEntity::notifyModified() {
    if (m_pDb && m_id != 0) {
        m_pDb->onEntityModified(m_id);
    }
}

void DbEntity::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbObject::writeFields(writer);
    
    writer.Key("layerId");
    DbJsonUtils::writeUint64(writer, m_layerId);
    
    writer.Key("color");
    DbJsonUtils::writeColor(writer, m_color);
    
    writer.Key("linetype");
    DbJsonUtils::writeLinetype(writer, m_linetype);
    
    writer.Key("linetypeScale");
    DbJsonUtils::writeDouble(writer, m_linetypeScale);
    
    writer.Key("lineWeight");
    DbJsonUtils::writeLineWeight(writer, m_lineWeight);
    
    writer.Key("visible");
    DbJsonUtils::writeBool(writer, m_visible);
}

bool DbEntity::readFields(const rapidjson::Value& value) {
    if (!DbObject::readFields(value)) {
        return false;
    }
    
    if (value.HasMember("layerId")) {
        DbJsonUtils::readUint64(value["layerId"], m_layerId);
    }
    
    if (value.HasMember("color")) {
        DbJsonUtils::readColor(value["color"], m_color);
    }
    
    if (value.HasMember("linetype")) {
        DbJsonUtils::readLinetype(value["linetype"], m_linetype);
    }
    
    if (value.HasMember("linetypeScale")) {
        DbJsonUtils::readDouble(value["linetypeScale"], m_linetypeScale);
    }
    
    if (value.HasMember("lineWeight")) {
        DbJsonUtils::readLineWeight(value["lineWeight"], m_lineWeight);
    }
    
    if (value.HasMember("visible")) {
        DbJsonUtils::readBool(value["visible"], m_visible);
    }
    
    return true;
}

} // namespace tch
