// 对应头文件
#include "DbObject.h"

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {

void DbObject::toJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    writer.StartObject();
    
    writer.Key("type");
    writer.Uint(static_cast<uint16_t>(type()));
    
    writer.Key("typeString");
    writer.String(typeName());
    
    writeFields(writer);
    
    writer.EndObject();
}

bool DbObject::fromJson(const rapidjson::Value& value) {
    if (!value.HasMember("type") || !value["type"].IsUint()) { return false; }
    // typeString 只用于调试，不读取
    return readFields(value);
}

void DbObject::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    writer.Key("id");
    writer.Uint64(m_id);
}

bool DbObject::readFields(const rapidjson::Value& value) {
    if (value.HasMember("id") && value["id"].IsUint64()) {
        m_id = value["id"].GetUint64();
    }
    return true;
}

} // namespace tch