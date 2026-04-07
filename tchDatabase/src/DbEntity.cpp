// 对应头文件
#include "DbEntity.h"

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {

void DbEntity::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbObject::writeFields(writer);  // 先调用基类写入 id
    
    writer.Key("layerId");
    writer.Uint64(m_layerId);
    
    writer.Key("color");
    writer.StartObject();
    
    writer.Key("type");
    writer.Int(static_cast<int>(m_color.type()));
    
    if (m_color.type() == DbColor::kColor) {
        writer.Key("rgb");
        writer.Uint(m_color.rgb());
    }
    
    writer.EndObject();
    
    writer.Key("linetype");
    writer.StartObject();
    
    writer.Key("type");
    writer.Int(static_cast<int>(m_linetype.type()));
    
    if (m_linetype.type() == DbLinetypeRef::kLinetype) {
        writer.Key("id");
        writer.Uint64(m_linetype.linetypeId());
    }
    
    writer.EndObject();
    
    writer.Key("linetypeScale");
    writer.Double(m_linetypeScale);
    
    writer.Key("lineWeight");
    writer.Int(static_cast<int>(m_lineWeight));
    
    writer.Key("visible");
    writer.Bool(m_visible);
}

bool DbEntity::readFields(const rapidjson::Value& value) {
    if (!DbObject::readFields(value)) { return false; }  // 先调用基类读取 id
    
    if (value.HasMember("layerId") && value["layerId"].IsUint64()) {
        m_layerId = value["layerId"].GetUint64();
    }
    
    // 颜色
    if (value.HasMember("color") && value["color"].IsObject()) {
        const auto& colorVal = value["color"];
        if (colorVal.HasMember("type") && colorVal["type"].IsInt()) {
            int type = colorVal["type"].GetInt();
            if (type == DbColor::kByLayer) {
                m_color = DbColor::byLayer();
            } else if (type == DbColor::kByBlock) {
                m_color = DbColor::byBlock();
            } else if (type == DbColor::kColor && colorVal.HasMember("rgb") && colorVal["rgb"].IsUint()) {
                m_color = DbColor::rgb(colorVal["rgb"].GetUint());
            }
        }
    }
    
    // 线型
    if (value.HasMember("linetype") && value["linetype"].IsObject()) {
        const auto& ltVal = value["linetype"];
        if (ltVal.HasMember("type") && ltVal["type"].IsInt()) {
            int type = ltVal["type"].GetInt();
            if (type == DbLinetypeRef::kByLayer) {
                m_linetype = DbLinetypeRef::byLayer();
            } else if (type == DbLinetypeRef::kByBlock) {
                m_linetype = DbLinetypeRef::byBlock();
            } else if (type == DbLinetypeRef::kContinuous) {
                m_linetype = DbLinetypeRef::continuous();
            } else if (type == DbLinetypeRef::kLinetype && ltVal.HasMember("id") && ltVal["id"].IsUint64()) {
                m_linetype = DbLinetypeRef::byId(ltVal["id"].GetUint64());
            }
        }
    }
    
    // 线型比例
    if (value.HasMember("linetypeScale") && value["linetypeScale"].IsDouble()) {
        m_linetypeScale = value["linetypeScale"].GetDouble();
    }
    
    // 线宽
    if (value.HasMember("lineWeight") && value["lineWeight"].IsInt()) {
        m_lineWeight = static_cast<DbLineWeight>(value["lineWeight"].GetInt());
    }
    
    // 可见性
    if (value.HasMember("visible") && value["visible"].IsBool()) {
        m_visible = value["visible"].GetBool();
    }
    
    return true;
}

} // namespace tch